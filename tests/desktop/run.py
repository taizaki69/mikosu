#!/usr/bin/env python3
"""Linux desktop tests: a real (non-headless) mikosu window on a private X display, for what the headless tests can't
see: keyboard input through the system input method, dead keys, and alt-tab under a real window manager.

Everything runs on a private display (Xvfb), in a private D-Bus session (dbus-run-session) with private config and
cache folders, so the user's desktop, input method and settings are never touched. Keys come from xdotool.

usage:  run.py [--game build/dist/bin-x86_64/mikosu] [test ...]      (tests: menu_keys dead_keys alttab)
needs:  Xvfb, xdotool, dbus-run-session; ibus-daemon for menu_keys/dead_keys; muffin or metacity plus xclock for
        alttab. A test whose tools are missing is skipped.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INNER = "MIKOSU_DESKTOP_TEST_INNER"
NAV_KEYS = "Down Down Down Up Down Up Down Down Up Up Right Left Right Left Down Up Down Up Down Up".split()
LETTERS = "asdfghjklq"


def have(*tools):
    return all(shutil.which(t) for t in tools)


def free_display():
    for n in range(90, 100):
        if not Path(f"/tmp/.X11-unix/X{n}").exists() and not Path(f"/tmp/.X{n}-lock").exists():
            return f":{n}"
    sys.exit("no free X display number in :90-:99")


class Desktop:
    """a private X display with optional input method and window manager, and one game instance"""

    def __init__(self, game, work, size="1280x720"):
        self.game, self.work, self.procs = game, work, []
        self.display = free_display()
        self.env = dict(os.environ, DISPLAY=self.display, XDG_CONFIG_HOME=str(work / "xdgcfg"),
                        XDG_CACHE_HOME=str(work / "xdgcache"))
        self.env.pop("WAYLAND_DISPLAY", None)
        for sub in ("xdgcfg", "xdgcache"):
            (work / sub).mkdir(parents=True, exist_ok=True)
        self.spawn(["Xvfb", self.display, "-screen", "0", f"{size}x24", "-nolisten", "tcp"])
        time.sleep(1)

    def spawn(self, cmd, **kw):
        p = subprocess.Popen(cmd, env=kw.pop("env", self.env), stdout=kw.pop("stdout", subprocess.DEVNULL),
                             stderr=subprocess.STDOUT, **kw)
        self.procs.append(p)
        return p

    def run(self, *cmd):
        return subprocess.run(list(cmd), env=self.env, capture_output=True, text=True).stdout.strip()

    def start_game(self, cfg_lines, xmodifiers, log):
        data = self.work / "data"
        (data / "cfg").mkdir(parents=True)
        (data / "no-osu").mkdir()
        (data / "cfg" / "osu.cfg").write_text("\n".join([f"osu_folder {data}/no-osu", *cfg_lines]) + "\n")
        env = dict(self.env, XMODIFIERS=xmodifiers, SDL_EVENT_LOGGING="1")
        self.game_proc = self.spawn([str(self.game), "-console", "-multi", "-opengl", "-datadir", str(data)],
                                    env=env, stdin=subprocess.PIPE, stdout=open(log, "w"), text=True)
        time.sleep(7)
        self.game_win = self.run("xdotool", "search", "--pid", str(self.game_proc.pid)).split("\n")[0]

    def command(self, line):
        self.game_proc.stdin.write(line + "\n")
        self.game_proc.stdin.flush()

    def focus_game(self):
        self.run("xdotool", "windowactivate", "--sync", self.game_win)
        self.run("xdotool", "windowfocus", self.game_win)

    def key(self, *keys):
        for k in keys:
            self.run("xdotool", "key", "--clearmodifiers", k)
            time.sleep(0.15)

    def close(self):
        try:
            self.command("exit")
            self.game_proc.wait(timeout=10)
        except Exception:
            pass
        for p in reversed(self.procs):
            if p.poll() is None:
                p.terminate()
        for p in self.procs:
            try:
                p.wait(timeout=5)
            except subprocess.TimeoutExpired:
                p.kill()


def events(log, kind):
    return [l for l in Path(log).read_text(errors="replace").splitlines() if f"SDL_EVENT_{kind} " in l]


def texts(log):
    return [m.group(1) for l in events(log, "TEXT_INPUT") if (m := re.search(r"text='([^']*)'", l))]


def test_menu_keys(game, work):
    """menu key presses with IBus running as the X input method, like a stock Ubuntu/Mint desktop"""
    if not have("ibus-daemon"):
        return None, "ibus-daemon not installed"
    d = Desktop(game, work)
    try:
        d.spawn(["ibus-daemon", "--xim", "--panel=disable"])
        time.sleep(2)
        log = work / "game.log"
        d.start_game([], "@im=ibus", log)
        d.command("set_active_ui_screen songbrowser")
        time.sleep(3)
        d.focus_game()
        time.sleep(1)
        d.key(*NAV_KEYS)
        d.key(*LETTERS)
        time.sleep(1)
    finally:
        d.close()
    downs, chars = len(events(log, "KEY_DOWN")), "".join(texts(log))
    want = len(NAV_KEYS) + len(LETTERS)
    ok = downs == want and chars == LETTERS
    return ok, f"{downs}/{want} key presses, typed '{chars}'"


def test_dead_keys(game, work):
    """a dead key sequence on the Latin American layout: e, then ´ e (one é), then a"""
    if not have("ibus-daemon", "setxkbmap"):
        return None, "ibus-daemon or setxkbmap not installed"
    d = Desktop(game, work)
    try:
        d.run("setxkbmap", "-layout", "latam")
        d.spawn(["ibus-daemon", "--xim", "--panel=disable"])
        time.sleep(2)
        log = work / "game.log"
        d.start_game([], "@im=ibus", log)
        d.command("set_active_ui_screen songbrowser")
        time.sleep(3)
        d.focus_game()
        time.sleep(1)
        d.key("e", "dead_acute", "e", "a")
        time.sleep(1)
    finally:
        d.close()
    chars = "".join(texts(log))
    return chars == "eéa", f"typed '{chars}' (want 'eéa')"


def test_alttab(game, work):
    """fullscreen with a custom letterboxed resolution: alt-tab to another window and back, 3 times"""
    wm = next((w for w in ("muffin", "metacity") if have(w)), None)
    if not wm or not have("xclock", "xprop"):
        return None, "needs muffin or metacity, xclock and xprop"
    d = Desktop(game, work, size="1920x1080")
    problems = []
    try:
        d.spawn([wm, "--replace"])
        time.sleep(3)
        log = work / "game.log"
        d.start_game(["fullscreen 1", "letterboxing 1", "letterboxed_resolution 1280x960", "debug_osu 1"],
                     "@im=none", log)
        clock = d.spawn(["xclock"])
        time.sleep(2)
        clock_win = d.run("xdotool", "search", "--pid", str(clock.pid)).split("\n")[0]
        for i in range(1, 4):
            d.run("xdotool", "windowactivate", clock_win)
            time.sleep(1.5)
            stacking = d.run("xprop", "-root", "_NET_CLIENT_LIST_STACKING").split("#")[-1].split(",")
            if not stacking or int(stacking[-1].strip(), 16) != int(clock_win):
                problems.append(f"away {i}: the other window isn't on top")
            d.run("xdotool", "windowactivate", d.game_win)
            time.sleep(2)
            state = d.run("xprop", "-id", d.game_win, "_NET_WM_STATE")
            active = d.run("xdotool", "getactivewindow")
            if active != d.game_win or "FULLSCREEN" not in state:
                problems.append(f"back {i}: active={active} state={state.split('= ')[-1]}")
    finally:
        d.close()
    last = [m.group(0) for m in re.finditer(r"Actual \(\d+x\d+\): [A-Za-z ]+", Path(log).read_text(errors="replace"))]
    if not last or not last[-1].startswith("Actual (1280x960): FS letterboxed"):
        problems.append(f"resolution at the end: {last[-1] if last else 'none logged'}")
    return not problems, f"under {wm}: " + ("; ".join(problems) if problems else "fullscreen, focused, 1280x960 every time")


TESTS = {"menu_keys": test_menu_keys, "dead_keys": test_dead_keys, "alttab": test_alttab}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, default=ROOT / "build" / "dist" / "bin-x86_64" / "mikosu")
    ap.add_argument("tests", nargs="*", default=list(TESTS))
    args = ap.parse_args()
    if not sys.platform.startswith("linux"):
        sys.exit("Linux only")
    if not have("Xvfb", "xdotool", "dbus-run-session"):
        sys.exit("needs Xvfb, xdotool and dbus-run-session")

    if os.environ.get(INNER) != "1":
        # a private D-Bus session, so the private IBus and window manager never meet the user's
        os.execvpe("dbus-run-session", ["dbus-run-session", "--", sys.executable, __file__, "--game",
                                        str(args.game.resolve()), *args.tests], dict(os.environ, **{INNER: "1"}))

    failed = 0
    for name in args.tests:
        work = Path(tempfile.mkdtemp(prefix=f"mikosu-desktop-{name}-"))
        ok, detail = TESTS[name](args.game, work)
        status = "SKIP" if ok is None else ("PASS" if ok else "FAIL")
        print(f"{status} {name}: {detail}", flush=True)
        if ok is False:
            failed += 1
            print(f"  (kept {work})")
        else:
            shutil.rmtree(work, ignore_errors=True)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
