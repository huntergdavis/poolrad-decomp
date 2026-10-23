#!/usr/bin/env python3
"""Stage (b) save-walk driver (VP i20735): the keys of tools/oracle/capture_save_walk.sh (Enter, L, A, Up), each sent
only once the screen it is meant for is showing, instead of at fixed AUTOTYPE times. Fixed times drift with host load:
under the per-instruction observer core on a loaded host, run hybrid-stage-b-walk-a typed L and A into the code-word
prompt. Runs inside xvfb-run; launches tools/oracle/launch.sh itself.

Gates (each with its own budget; the whole route has one too):
  1. prompt line reads CODE           -> Return  (the code-word prompt)
  2. prompt line reads FUNCTION       -> l       (main menu: Load saved game)
  3. prompt line changed and is stable -> a       (save slot A)
  4. screen shows 15,4                 -> Up      (saved party position)
  5. screen shows 14,4                 -> stop    (the step north happened)
On success the emulator gets its shutdown key (Ctrl+F9, quit warning off) after SETTLE seconds, so it exits normally and
the observer writes its final report; SIGTERM, then SIGKILL, are logged fallbacks (a killed run has no final report); on any budget miss it is stopped and the stage is named.
With STOP=menu it stops at gate 2 instead (the cold start to the main menu, same keys up to there): the baseline that
"not on the cold start" is measured against.
Optional tail (route 2 and later): WALK_EXTRA="key key ..." (xdotool key names). After 14,4 each key is sent in turn,
then the driver waits until the prompt line reads the same on two polls (budget EXTRA_BUDGET_S per key, default 120)
and saves the frame as extraNN-<key>. The tail stops early, with the stage named, if a step's budget runs out, and with
exit 8 ("route diverged") if a random-encounter prompt (COMBAT) appears, or if WALK_EXPECT is set and a step settles on
a prompt containing none of its expected words: those runs are not the route.
Optional combat route (route 3, PM i21589): WALK_COMBAT=1. After 14,4 the party paces Down/Up until the encounter menu
(COMBAT/WAIT/FLEE) settles (budget SEEK_STEPS steps, default 60; exit 9 if none), presses c (Combat), then, until the area
menu (AREA/SEARCH/LOOK) settles again (budget COMBAT_BUDGET_S, default 1500): q (Quick: the computer fights for that
character) on every character's combat menu, Return on every PRESS ENTER/RETURN screen, e (Exit) then y on the treasure
menu. A settled prompt matching none of these stops with exit 10 and the frame saved, so the route never presses a key
for a screen it does not know. Every key is logged with the prompt it answered.
Usage: walk_driver.py RUN_DIR [STAGE_BUDGET_S=300] [TOTAL_BUDGET_S=1500] [STOP=walk|menu]
"""
import os, signal, subprocess, sys, time

RUN = sys.argv[1]
STAGE_BUDGET = int(sys.argv[2]) if len(sys.argv) > 2 else 300
TOTAL_BUDGET = int(sys.argv[3]) if len(sys.argv) > 3 else 1500
STOP = sys.argv[4] if len(sys.argv) > 4 else "walk"
assert STOP in ("walk", "menu"), STOP
SETTLE = 3
FR = os.path.join(RUN, "frames")
os.makedirs(FR, exist_ok=True)
T0 = time.time()
n = 0


def log(msg):
    print(f"t={time.time() - T0:6.1f}s {msg}", flush=True)


def grab(tag):
    global n
    n += 1
    p = os.path.join(FR, f"{n:04d}-{tag}.png")
    subprocess.run(["import", "-window", "root", p], check=False, stderr=subprocess.DEVNULL)
    return p


def ocr_prompt(p):
    c = p[:-4] + "-prompt.png"
    subprocess.run(["convert", p, "-crop", "660x26+190+562", "-scale", "200%", c], check=False)
    r = subprocess.run(["tesseract", c, "stdout", "--psm", "7"], capture_output=True, text=True)
    return " ".join(r.stdout.split())


def ocr_digits(p):
    r = subprocess.run(["tesseract", p, "stdout", "--psm", "11", "-c", "tessedit_char_whitelist=0123456789,NSWE:"],
                       capture_output=True, text=True)
    return " ".join(r.stdout.split())


def key(k):
    subprocess.run(["xdotool", "windowfocus", "--sync", WIN], check=False)
    subprocess.run(["xdotool", "key", "--delay", "120", k], check=False)
    log(f"KEY {k}")


emu = subprocess.Popen(["tools/oracle/launch.sh", RUN, "-set", "cpu core=normal", "-set", "dosbox quit warning=false", "-time-limit", str(TOTAL_BUDGET + 60)],
                       stdout=open(os.path.join(RUN, "launch.log"), "w"), stderr=subprocess.STDOUT)


def descendants(comm):
    rows = [r.split(None, 2) for r in subprocess.run(["ps", "-eo", "pid=,ppid=,comm="], capture_output=True,
                                                     text=True).stdout.splitlines()]
    kids, out, todo = {}, [], [emu.pid]
    for pid, ppid, c in rows:
        kids.setdefault(int(ppid), []).append((int(pid), c))
    for _ in range(64):            # bounded walk of the process tree
        if not todo:
            break
        for pid, c in kids.get(todo.pop(), []):
            todo.append(pid)
            if c == comm:
                out.append(pid)
    return out


def stop(rc, why):
    log(f"STOP {why}")
    if WIN:                        # the emulator's own shutdown key: a normal exit, so the observer's atexit report runs
        key("ctrl+F9")
        try:
            emu.wait(timeout=60)
            log(f"stopped by Ctrl+F9; launch_rc={emu.returncode}")
            sys.exit(rc)
        except subprocess.TimeoutExpired:
            log("Ctrl+F9 did not stop the emulator within 60 s")
    for p in descendants("dosbox-x") or [emu.pid]:
        log(f"SIGTERM {p}")
        os.kill(p, signal.SIGTERM)
    try:
        emu.wait(timeout=60)
    except subprocess.TimeoutExpired:
        emu.kill()
        log("emulator did not exit within 60 s of SIGTERM; killed")
    log(f"launch_rc={emu.returncode}")
    sys.exit(rc)


WIN = ""
for _ in range(60):
    w = subprocess.run(["xdotool", "search", "--name", "DOSBox"], capture_output=True, text=True).stdout.split()
    if w:
        WIN = w[-1]
        break
    time.sleep(1)
if not WIN:
    stop(3, "no DOSBox-X window within 60 s")
log(f"window {WIN}")


def wait_for(name, test, ocr):
    t = time.time()
    last = None
    while time.time() - t < STAGE_BUDGET:
        if time.time() - T0 > TOTAL_BUDGET:
            stop(4, f"total budget {TOTAL_BUDGET}s exhausted in gate {name}")
        if emu.poll() is not None:
            stop(5, f"emulator exited in gate {name}")
        time.sleep(1)
        p = grab(name)
        txt = ocr(p)
        log(f"{name} poll: {txt!r}")
        ok = test(txt, last)
        last = txt
        if ok:
            log(f"GATE {name} passed on {os.path.basename(p)}")
            return txt
    stop(6, f"gate {name} budget {STAGE_BUDGET}s exhausted")


wait_for("code", lambda s, l: "CODE" in s, ocr_prompt)
time.sleep(1); key("Return")
menu = wait_for("menu", lambda s, l: "FUNCTION" in s, ocr_prompt)
if STOP == "menu":
    time.sleep(SETTLE)
    grab("final")
    stop(0, "cold start complete: main menu seen")
time.sleep(1); key("l")
wait_for("slots", lambda s, l: "FUNCTION" not in s and s == l, ocr_prompt)
key("a")
wait_for("at15_4", lambda s, l: "15,4" in s, ocr_digits)
time.sleep(1); key("Up")
wait_for("at14_4", lambda s, l: "14,4" in s, ocr_digits)
extra = os.environ.get("WALK_EXTRA", "").split()
expect = os.environ.get("WALK_EXPECT", "").split()        # per step: "WORD|WORD" the settled prompt must contain one of
for k, name in enumerate(extra, 1):
    time.sleep(1); key(name)
    t = time.time(); last = None; done = False
    while time.time() - t < int(os.environ.get("EXTRA_BUDGET_S", "120")):
        if time.time() - T0 > TOTAL_BUDGET:
            stop(4, f"total budget exhausted in extra step {k} ({name})")
        time.sleep(2)
        p = grab(f"extra{k:02d}-{name}")
        txt = ocr_prompt(p)
        log(f"extra {k} {name} poll: {txt!r}")
        if "COMBAT" in txt:            # a random encounter: the route has diverged, so nothing after this is the route
            stop(8, f"route diverged: encounter prompt {txt!r} at extra step {k} ({name})")
        if txt == last:
            done = True
            break
        last = txt
    if not done:
        stop(7, f"extra step {k} ({name}) never settled")
    log(f"EXTRA {k} {name} settled: {last!r}")
    if k <= len(expect) and not any(w in (last or "") for w in expect[k - 1].split("|")):
        stop(8, f"route diverged: extra step {k} ({name}) settled on {last!r}, expected one of {expect[k - 1]}")


def settle(tag, budget):
    """Poll the prompt until it reads the same twice; returns it (None if the budget ran out)."""
    t = time.time(); last = None
    while time.time() - t < budget:
        if time.time() - T0 > TOTAL_BUDGET:
            stop(4, f"total budget exhausted at {tag}")
        if emu.poll() is not None:
            stop(5, f"emulator exited at {tag}")
        time.sleep(2)
        txt = ocr_prompt(grab(tag))
        log(f"{tag} poll: {txt!r}")
        if txt and txt == last:
            return txt
        last = txt
    return None


def readable(txt):
    """A prompt with at least one known menu word; the empty prompt bar OCRs as noise."""
    return any(w in txt for w in ("AREA", "SEARCH", "LOOK", "COMBAT", "FLEE", "ENTER", "RETURN", "CONTINUE", "QUICK", "GUARD",
                                  "AIM", "MOVE", "OVE", "USE", "DONE", "DOE", "AIN", "TREASURE", "TAKE", "POOL", "SURE", "Y/N", "BATTLE", "CAMP", "VIEW", "LEFT", "ATTACH", "ATTACK", "DYING", "YES"))


def has(txt, *words):
    return any(w in txt for w in words)


ENCOUNTER = ("COMBAT", "FLEE")
AREA = ("AREA", "SEARCH", "LOOK")
CONTINUE = ("ENTER", "RETURN", "CONTINUE")
# The combat menu is MOVE VIEW AIM USE QUICK DONE; its two-colour hot letters OCR badly ('NHOVE WIEN AIN USE adich DOE'),
# so match the fragments that survive.
COMBAT_MENU = ("QUICK", "GUARD", "AIM", "MOVE", "OVE", "USE", "DONE", "DOE", "AIN")
if os.environ.get("WALK_COMBAT"):
    found = None
    for step in range(int(os.environ.get("SEEK_STEPS", "60"))):
        k = "Down" if step % 2 == 0 else "Up"
        key(k)
        while True:
            txt = settle(f"seek{step:02d}-{k}", 60) or ""
            if has(txt, *CONTINUE) and not has(txt, *ENCOUNTER):
                key("Return"); continue
            break
        if has(txt, *ENCOUNTER, *COMBAT_MENU, "BATTLE") or not readable(txt):
            found = txt; break             # the encounter menu, or an ambush: BATTLE BEGINS, then the combat screen
        if not has(txt, *AREA):
            stop(10, f"seek step {step}: unknown settled prompt {txt!r}")
    if not found:
        stop(9, "no encounter within the seek budget")
    log(f"ENCOUNTER {found!r}")
    if has(found, *ENCOUNTER):
        time.sleep(1); key("c")
    t = time.time(); quick = 0; unknown_since = None; after_exit = False
    while True:
        if time.time() - t > int(os.environ.get("COMBAT_BUDGET_S", "1500")):
            stop(7, "combat never returned to the area menu")
        txt = settle("combat", 120)
        if txt is None:
            continue                       # still animating: the computer is fighting
        if has(txt, *AREA) and not has(txt, *ENCOUNTER):
            log(f"COMBAT OVER: area menu {txt!r}; quick presses {quick}")
            break
        if after_exit and len(txt) <= 12 and has(txt, "NO", "MO", "YES"):
            key("n")                       # THERE IS STILL TREASURE LEFT. GO BACK AND CLAIM IT? YES NO (OCR 'Me MO'): leave it
            after_exit = False
        elif has(txt, "BATTLE") and has(txt, "YES", "HES", "NO"):
            key("n")                       # CONTINUE BATTLE: YES NO is asked when no enemy is left in reach (the party only
                                           # guards, measured in hybrid-r3-proto-e): No ends the fight, its natural end
        elif has(txt, "LEFT", "ATTACH", "ATTACK", "DYING"):
            continue                       # MOVE/ATTACK, MOVE LEFT = n: the computer is moving that character; a status line
        elif has(txt, *COMBAT_MENU):
            key("q"); quick += 1
        elif has(txt, "TREASURE", "TAKE", "POOL", "FOOL", "SHARE"):
            key("e"); after_exit = True
        elif has(txt, "SURE", "Y/N", "(Y"):
            key("y")
        elif has(txt, *CONTINUE):
            key("Return")
        elif has(txt, *ENCOUNTER):
            key("c")
        elif not readable(txt) or has(txt, "BATTLE"):
            # the prompt bar is empty while monsters act (OCR reads noise): wait, but not for ever on one screen
            unknown_since = unknown_since or time.time()
            if time.time() - unknown_since > 120:
                stop(10, f"combat: unreadable prompt unchanged for 120 s: {txt!r}")
            continue
        else:
            stop(10, f"combat: unknown settled prompt {txt!r}")
        unknown_since = None
time.sleep(SETTLE)
grab("final")
stop(0, "route complete: 15,4 -> 14,4 seen" + (f", then {len(extra)} extra keys" if extra else "") +
     (", then an encounter fought to the area menu" if os.environ.get("WALK_COMBAT") else ""))
