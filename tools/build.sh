#!/usr/bin/env bash
# One-command build for mikosu.
#
#   tools/build.sh [linux|windows] [--dev] [--debug] [-j N] [-- extra configure args]
#   (MIKOSU_JOBS=N also sets the job count; on CI, where $CI is set, it defaults to all cores)
#
#   linux    (default) native Linux x86_64 build -> build/dist/bin-x86_64/
#   windows  Windows x64 cross-build with llvm-mingw -> build-win64/dist/bin-x86_64/
#   --dev    also build the in-binary tests (-testapp); uses build-dev/ or build-win64-dev/
#   --debug  debug build (implies --dev, separate *-debug build dir)
#
# Prerequisites (Ubuntu/Mint package names) are listed in CLAUDE.md. Dependencies are downloaded once
# into build-aux/cache/ and built inside the build dir, so the first build takes ~10 minutes.
# Heavy steps run through tools/dev/guarded (memory/task caps), see CLAUDE.md.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# perl (automake) intermittently segfaults at exit with some locales (seen with LC_NUMERIC=agr_PE),
# which breaks autoreconf for us and for dependencies like mpg123; build with a neutral locale
export LC_ALL=C.UTF-8
TOOLS_HOME="${MIKOSU_DEV_HOME:-$HOME/.local/share/mikosu-dev}"

# pinned toolchain for Windows cross-builds (same as upstream CI)
LLVM_MINGW_VERSION=20260908
LLVM_MINGW_SHA256=2258c745e3155870c80793f3e8c80b28fbde11b9ff73c4c78783635b3440b092

target=linux
dev=0
debug=0
jobs=""
extra=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        linux|windows) target="$1"; shift ;;
        --dev) dev=1; shift ;;
        --debug) debug=1; dev=1; shift ;;
        -j) jobs="$2"; shift 2 ;;
        -j*) jobs="${1#-j}"; shift ;;
        --) shift; extra=("$@"); break ;;
        -h|--help) sed -n '2,13p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done

jobs="${jobs:-${MIKOSU_JOBS:-}}"
if [[ -z "$jobs" ]]; then
    n=$(nproc 2>/dev/null || echo 4)
    if [[ -n "${CI:-}" ]]; then
        jobs=$n  # dedicated CI runner: use every core
    else
        jobs=$(( n > 12 ? 10 : (n > 2 ? n - 2 : 1) ))  # leave the desktop some room
    fi
fi

log() { printf '\033[1m[build]\033[0m %s\n' "$*"; }

# --- autotools: regenerate configure/Makefile.in when the inputs or the source list changed ---
needs_autogen() {
    [[ -f "$ROOT/configure" && -f "$ROOT/Makefile.in" && -f "$ROOT/src/Makefile.sources" ]] || return 0
    # autoreconf only rewrites the outputs whose inputs changed, so compare each input with its own output
    [[ "$ROOT/configure.ac" -nt "$ROOT/configure" ]] && return 0
    [[ "$ROOT/Makefile.am" -nt "$ROOT/Makefile.in" ]] && return 0
    [[ "$ROOT/autogen.sh" -nt "$ROOT/src/Makefile.sources" ]] && return 0
    # a .cpp/.c added or removed since the last autogen
    local listed actual
    listed=$(grep -oE '(src|libraries)/[^ \\]+\.(cpp|c)' "$ROOT/src/Makefile.sources" | LC_ALL=C sort -u)
    actual=$(cd "$ROOT" && find src libraries -not -path '*/[@.]*' -type f \( -name '*.cpp' -o -name '*.c' \) | LC_ALL=C sort -u)
    [[ "$listed" != "$actual" ]]
}
if needs_autogen; then
    log "running autogen.sh"
    (cd "$ROOT" && ./autogen.sh >/dev/null)
fi

# --- toolchain ---
configure_args=(--with-audio=bass,soloud)
case "$target" in
    linux)
        # configure.ac uses plain gcc/g++; GCC >= 14 is required (<print>). Use a PATH shim when the
        # default compiler is older but gcc-14 is installed. Only compiler drivers go in the shim:
        # never ar/nm/ranlib (gcc-ar finds "ar" on PATH and would exec itself forever).
        if [[ "$(g++ -dumpversion 2>/dev/null | cut -d. -f1)" -lt 14 ]]; then
            if command -v g++-14 >/dev/null; then
                shim="$TOOLS_HOME/gcc14-shim/bin"
                mkdir -p "$shim"
                for pair in gcc:gcc-14 g++:g++-14 cc:gcc-14 c++:g++-14 cpp:cpp-14 gcov:gcov-14; do
                    ln -sfn "/usr/bin/${pair#*:}" "$shim/${pair%%:*}"
                done
                export PATH="$shim:$PATH"
                log "using $(g++ --version | head -1) via $shim"
            else
                echo "error: GCC 14 or newer is required (install g++-14)" >&2
                exit 1
            fi
        fi
        configure_args+=(--with-renderer=opengl,sdlgpu)
        builddir="$ROOT/build"
        ;;
    windows)
        tc="$TOOLS_HOME/toolchains/llvm-mingw-$LLVM_MINGW_VERSION"
        if [[ ! -x "$tc/bin/x86_64-w64-mingw32-clang++" ]]; then
            archive="llvm-mingw-$LLVM_MINGW_VERSION-ucrt-ubuntu-22.04-x86_64.tar.xz"
            log "downloading $archive"
            mkdir -p "$tc"
            tmp="$(mktemp -d)"
            curl -fsSL -o "$tmp/$archive" \
                "https://github.com/mstorsjo/llvm-mingw/releases/download/$LLVM_MINGW_VERSION/$archive"
            echo "$LLVM_MINGW_SHA256  $tmp/$archive" | sha256sum -c --quiet
            tar xf "$tmp/$archive" -C "$tc" --strip-components=1
            rm -rf "$tmp"
        fi
        export PATH="$tc/bin:$PATH"
        configure_args+=(--host=x86_64-w64-mingw32 --enable-clang --with-renderer=opengl,dx11,sdlgpu)
        builddir="$ROOT/build-win64"
        ;;
esac

if (( debug )); then
    configure_args+=(--enable-debug)
    builddir="$builddir-debug"
elif (( dev )); then
    builddir="$builddir-dev"
fi
(( dev )) || configure_args+=(--disable-tests)
configure_args+=("${extra[@]}")

mkdir -p "$builddir"
cd "$builddir"

# (re)configure when the build dir is new, configure changed, or the arguments changed
args_file="$builddir/.mikosu-configure-args"
if [[ ! -f Makefile || "$ROOT/configure" -nt Makefile || "$(cat "$args_file" 2>/dev/null)" != "${configure_args[*]}" ]]; then
    log "configuring $builddir: ${configure_args[*]}"
    "$ROOT/configure" "${configure_args[@]}" > configure.log 2>&1 || { tail -40 configure.log; exit 1; }
    printf '%s' "${configure_args[*]}" > "$args_file"
fi

log "building with -j$jobs (log: $builddir/make.log)"
guard=()
[[ -x "$ROOT/tools/dev/guarded" ]] && guard=("$ROOT/tools/dev/guarded" --mem "${GUARDED_MEM:-16G}" --)
if ! "${guard[@]}" make -j"$jobs" install > make.log 2>&1; then
    grep -nE "error:|\*\*\*" make.log | head -20
    echo "build failed, see $builddir/make.log" >&2
    exit 1
fi
log "done: $(ls -d "$builddir"/dist/bin-*/ | head -1)"
