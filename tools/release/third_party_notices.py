#!/usr/bin/env python3
"""Writes THIRD_PARTY_NOTICES.txt: every third-party component mikosu ships or links, with its licence text.

The texts come from the dependency sources a build unpacked (build/build/deps/<name>/), the cached archives in
build-aux/cache/ and the vendored code in libraries/, so run it after `tools/build.sh linux`. Long standard
licences (GPL-2.0, LGPL-2.1, Apache-2.0) are printed once at the end and referred to.

usage: third_party_notices.py [--build build] [--out THIRD_PARTY_NOTICES.txt]
"""

import argparse
import re
import sys
import tarfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "build-aux" / "cache"


def version(prefix):
    """the version (or short commit) of a dependency, from its archive name in build-aux/cache"""
    pat = re.compile(re.escape(prefix) + r"-(.+?)(?:\.tar\.\w+|\.tgz|\.zip)$")
    dirpat = re.compile(re.escape(prefix) + r"-([^.]+)$")  # unpacked into a folder (BASS)
    for p in sorted(CACHE.iterdir()):
        m = pat.match(p.name) or (dirpat.match(p.name) if p.is_dir() else None)
        if m:
            v = m.group(1)
            return v[:12] if re.fullmatch(r"[0-9a-f]{40}", v) else v.lstrip("v")
    return "?"


def leading_comment(path):
    """the comment block a source file starts with (its licence header): a whole /* ... */ block, or the run of //
    lines (blank lines inside it included)"""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    while lines and not lines[0].strip():
        lines.pop(0)
    if lines and lines[0].lstrip().startswith("/*"):
        out = []
        for line in lines:
            out.append(line)
            if "*/" in line:
                break
        return "\n".join(out).strip()
    out = []
    for line in lines:
        if line.lstrip().startswith("//") or (out and not line.strip()):
            out.append(line)
        else:
            break
    return "\n".join(out).strip()


def section(text, start, end=None):
    """the part of text from the line matching start up to (not including) the line matching end"""
    lines = text.replace("\r", "").splitlines()
    i = next(n for n, l in enumerate(lines) if re.match(start, l))
    j = next((n for n, l in enumerate(lines[i + 1 :], i + 1) if end and re.match(end, l)), len(lines))
    return "\n".join(lines[i:j]).strip()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", type=Path, default=ROOT / "build")
    ap.add_argument("--out", type=Path, default=ROOT / "THIRD_PARTY_NOTICES.txt")
    args = ap.parse_args()
    deps = args.build / "build" / "deps"
    if not deps.is_dir():
        sys.exit(f"{deps} not found: build first (tools/build.sh linux)")

    def dep(name, *files):
        return "\n\n".join((deps / name / f).read_text(encoding="utf-8", errors="replace").strip() for f in files)

    def copyright_line(name, path):
        for line in (deps / name / path).read_text(encoding="utf-8", errors="replace").splitlines():
            if "Copyright" in line:
                return line.strip(" */#").strip()
        return ""

    def tar_member(prefix, member_suffix):
        arc = next(p for p in CACHE.iterdir() if p.name.startswith(prefix + "-") and ".tar" in p.name)
        with tarfile.open(arc) as t:
            m = next(m for m in t.getmembers() if m.name.endswith(member_suffix))
            return t.extractfile(m).read().decode("utf-8", "replace").strip()

    def bass_terms():
        bass = next(p for p in CACHE.iterdir() if p.name.startswith("bass-") and p.is_dir())
        with zipfile.ZipFile(bass / "bass24-linux.zip") as z:
            txt = z.read("bass.txt").decode("latin-1")
        return section(txt, r"^Licence$", r"^Commercial licensing")

    lib = ROOT / "libraries"
    fonts = ROOT / "assets" / "fonts"

    # (name, version, homepage, licence, how mikosu uses it, licence text or "see <standard licence>")
    shipped = [
        ("FFmpeg", version("ffmpeg"), "https://ffmpeg.org", "GPL-2.0-or-later",
         "separate libraries (avcodec, avformat, avutil, swresample, swscale), built with --enable-gpl",
         dep("ffmpeg", "LICENSE.md") + "\n\n(full text: GNU GPL 2.0, below)"),
        ("BASS, BASS_FX, BASSmix, BASSFLAC, BASSloud, BASSASIO, BASSWASAPI", version("bass"), "https://www.un4seen.com",
         "proprietary, free for non-commercial use", "separate libraries, loaded for the BASS audio backend",
         bass_terms()),
        ("mimalloc", version("mimalloc"), "https://github.com/microsoft/mimalloc", "MIT", "separate library",
         dep("mimalloc", "LICENSE")),
    ]
    linked = [
        ("SDL 3", version("SDL3"), "https://libsdl.org", "Zlib", "linked", dep("SDL3", "LICENSE.txt")),
        ("FreeType", version("freetype"), "https://freetype.org", "FTL", "linked",
         dep("freetype", "docs/FTL.TXT")),
        ("libpng", version("libpng"), "http://www.libpng.org", "libpng-2.0", "linked", dep("libpng", "LICENSE")),
        ("libjpeg-turbo", version("libjpeg"), "https://libjpeg-turbo.org", "IJG AND BSD-3-Clause AND Zlib", "linked",
         dep("libjpeg", "LICENSE.md")),
        ("zlib-ng", version("zlib"), "https://github.com/zlib-ng/zlib-ng", "Zlib", "linked", dep("zlib", "LICENSE.md")),
        ("Brotli", version("brotli"), "https://github.com/google/brotli", "MIT", "linked", dep("brotli", "LICENSE")),
        ("bzip2", version("bzip2"), "https://sourceware.org/bzip2", "bzip2-1.0.6", "linked", dep("bzip2", "LICENSE")),
        ("XZ Utils (liblzma)", version("liblzma"), "https://tukaani.org/xz", "0BSD", "linked",
         dep("liblzma", "COPYING.0BSD")),
        ("Zstandard", version("zstd"), "https://facebook.github.io/zstd", "BSD-3-Clause", "linked",
         dep("zstd", "LICENSE")),
        ("libarchive", version("libarchive"), "https://libarchive.org", "BSD-2-Clause", "linked",
         dep("libarchive", "COPYING")),
        ("curl", version("curl"), "https://curl.se", "curl", "linked", dep("curl", "COPYING")),
        ("OpenSSL", version("openssl"), "https://openssl.org", "Apache-2.0", "linked",
         copyright_line("openssl", "crypto/cryptlib.c") + "\nLicensed under the Apache License 2.0 (below)."),
        ("nghttp2", version("nghttp2"), "https://nghttp2.org", "MIT", "linked", dep("nghttp2", "COPYING")),
        ("mpg123", version("mpg123"), "https://mpg123.org", "LGPL-2.1-only", "linked",
         section(dep("mpg123", "COPYING"), r".*", r".*GNU LESSER GENERAL PUBLIC LICENSE")
         + "\n\n(full text: GNU LGPL 2.1, below)"),
        ("SoLoud (neoloud fork)", version("SoLoud"), "https://github.com/neomodnet/neoloud", "Zlib", "linked",
         dep("SoLoud", "LICENSE")
         + "\n\nSoLoud includes dr_flac, dr_mp3, dr_wav, miniaudio, stb_vorbis and stb_image_write (public domain or "
         "MIT No Attribution) and Signalsmith Stretch (below)."),
        ("Signalsmith Stretch", "", "https://signalsmith-audio.co.uk/code/stretch", "MIT", "linked (through SoLoud)",
         dep("SoLoud", "vendored/signalsmith-stretch/LICENSE.txt")),
        ("Signalsmith Linear", "", "https://signalsmith-audio.co.uk", "MIT", "linked (through Signalsmith Stretch)",
         dep("SoLoud", "vendored/signalsmith-stretch/signalsmith-linear/LICENSE.txt")),
        ("{fmt}", version("fmt"), "https://fmt.dev", "MIT", "linked", dep("fmt", "LICENSE")),
        ("spdlog", version("spdlog"), "https://github.com/gabime/spdlog", "MIT", "linked", dep("spdlog", "LICENSE")),
        ("GLM", version("glm"), "https://github.com/g-truc/glm", "MIT", "linked", dep("glm", "copying.txt")),
        ("RapidJSON", version("rapidjson"), "https://rapidjson.org", "MIT", "linked (through discord-rpc)",
         tar_member("rapidjson", "/license.txt")),
        ("discord-rpc", version("discord-rpc"), "https://github.com/discord/discord-rpc", "MIT", "linked",
         dep("discord-rpc", "LICENSE")),
        ("simdutf", version("simdutf"), "https://simdutf.github.io/simdutf", "MIT (dual Apache-2.0/MIT)", "linked",
         dep("simdutf", "LICENSE-MIT")),
        ("Compile time regular expressions (CTRE)", version("ctre"), "https://github.com/hanickadot/compile-time-regular-expressions",
         "Apache-2.0 WITH LLVM-exception", "linked", dep("ctre", "LICENSE")),
        ("libacl", version("libacl"), "https://savannah.nongnu.org/projects/acl", "LGPL-2.1-or-later", "linked (Linux)",
         "Copyright (C) 1999, 2000 Andreas Gruenbacher and the acl contributors. Licensed under the GNU LGPL v2.1 "
         "or later (below)."),
        ("GNU libiconv", version("libiconv"), "https://www.gnu.org/software/libiconv", "LGPL-2.1-or-later",
         "linked (Windows)",
         copyright_line("libiconv", "lib/iconv.c") + "\nLicensed under the GNU LGPL v2.1 or later (below)."),
        ("nsync", version("nsync"), "https://github.com/google/nsync", "Apache-2.0", "linked (Windows)",
         copyright_line("nsync", "public/nsync.h") + "\nLicensed under the Apache License 2.0 (below)."),
    ]
    vendored = [
        ("Boost (sort, subset)", "", "https://www.boost.org", "BSL-1.0", "vendored in libraries/boost",
         "Boost Software License - Version 1.0 - August 17th, 2003\n\n"
         "Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software "
         "and accompanying documentation covered by this license (the \"Software\") to use, reproduce, display, "
         "distribute, execute, and transmit the Software, and to prepare derivative works of the Software, and to "
         "permit third-parties to whom the Software is furnished to do so, all subject to the following:\n\n"
         "The copyright notices in the Software and this entire statement, including the above license grant, this "
         "restriction and the following disclaimer, must be included in all copies of the Software, in whole or in "
         "part, and all derivative works of the Software, unless such copies or derivative works are solely in the "
         "form of machine-executable object code generated by a source language processor.\n\n"
         "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT "
         "LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. "
         "IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR "
         "OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE "
         "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."),
        ("delegates (SA::delegate)", "", "", "MIT",
         "vendored in libraries/delegatesv2", leading_comment(next((lib / "delegatesv2").rglob("*.h")))),
        ("fast_float", "", "https://github.com/fastfloat/fast_float", "MIT (triple Apache-2.0/MIT/BSL-1.0)",
         "vendored in libraries/fast_float", leading_comment(next((lib / "fast_float").rglob("*.h")))),
        ("unordered_dense", "", "https://github.com/martinus/unordered_dense", "MIT",
         "vendored in libraries/unordered_dense", leading_comment(next((lib / "unordered_dense").rglob("*.h")))),
        ("glad (generated OpenGL loader, Khronos API headers)", "", "https://github.com/Dav1dde/glad",
         "Apache-2.0 (Khronos headers)", "vendored in libraries/glad",
         "Copyright (c) The Khronos Group Inc. SPDX-License-Identifier: Apache-2.0 (below)."),
        ("stb_image", "2.30", "https://github.com/nothings/stb", "MIT OR Unlicense", "vendored in libraries",
         "stb_image is in the public domain, or alternatively available under the MIT license "
         "(see the end of libraries/stb_image.h): Copyright (c) 2017 Sean Barrett."),
        ("RSA Data Security, Inc. MD5 Message-Digest Algorithm", "", "", "RSA-MD", "vendored in libraries/MD5.cpp",
         "mikosu's MD5 code is derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm.\n\n"
         + leading_comment(lib / "MD5.cpp")),
        ("sha256 (LekKit)", "", "https://github.com/LekKit", "MIT", "vendored in libraries/sha256.cpp",
         leading_comment(lib / "sha256.cpp")),
        ("demoji", "", "", "MIT", "vendored in libraries/demoji.c", leading_comment(lib / "demoji.c")),
    ]
    font_entries = [
        ("Outfit", "", "https://github.com/Outfitio/Outfit-Fonts", "OFL-1.1", "UI font (assets/fonts/outfit.ttf)",
         (fonts / "OutfitLicense.txt").read_text(encoding="utf-8", errors="replace").strip()),
        ("Blobmoji", "", "https://github.com/C1710/blobmoji", "Apache-2.0", "font (assets/fonts)",
         "Blobmoji, a fork of Google's Noto Emoji, by its contributors. Licensed under the Apache License 2.0 (below; "
         "also assets/fonts/Blobmoji-LICENSE.txt)."),
        ("Fork Awesome", "", "https://forkaweso.me", "OFL-1.1", "icon font (assets/fonts)",
         "Fork Awesome, by the Fork Awesome contributors, licensed under the SIL Open Font License 1.1 (the full "
         "licence text is under Outfit above)."),
    ]

    standard = {
        "GNU General Public License, version 2": dep("ffmpeg", "COPYING.GPLv2"),
        "GNU Lesser General Public License, version 2.1": dep("libiconv", "COPYING.LIB"),
        "Apache License, Version 2.0": dep("openssl", "LICENSE.txt"),
    }

    out = [
        "mikosu third-party notices",
        "==========================",
        "",
        "mikosu is free software under the GNU General Public License v3.0 (see LICENSE). It is based on neomod",
        "(kiwec, spectator and contributors) and McOsu (McKay); see CREDITS.md.",
        "",
        "It ships, links or includes the following third-party components, each under its own licence.",
        "Generated by tools/release/third_party_notices.py.",
        "",
    ]
    for title, entries in (("Shipped as separate libraries", shipped), ("Linked into the program", linked),
                           ("Included source code", vendored), ("Fonts", font_entries)):
        out += ["", "#" * 100, f"# {title}", "#" * 100]
        for name, ver, url, spdx, use, text in entries:
            head = f"{name} {ver}".strip()
            out += ["", "-" * 100, head, "-" * 100]
            if url:
                out.append(f"Homepage: {url}")
            out += [f"Licence: {spdx}", f"Used as: {use}", "", text.strip()]
    out += ["", "#" * 100, "# Standard licence texts referred to above", "#" * 100]
    for name, text in standard.items():
        out += ["", "-" * 100, name, "-" * 100, "", text.strip()]
    args.out.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {args.out} ({args.out.stat().st_size // 1024} KiB)")


if __name__ == "__main__":
    main()
