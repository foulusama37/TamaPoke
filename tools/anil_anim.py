"""Decoder for Pokemon Anil's Data/PkmnAnimations.rxdata (Essentials v21 format).

Field indices / semantics verified against the canonical script
"BattleAnimationPlayer" (Scripts.rxdata #232, dumped to scripts/).
"""
import os, re, struct, sys, io

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import marshal_lite  # read-only reuse

ANIL = r"C:\Users\foulu\Downloads\Pokemon Anil V4.13"
RXDATA = os.path.join(ANIL, "Data", "PkmnAnimations.rxdata")
SHEETS = os.path.join(ANIL, "Graphics", "Animations")
MOVES_H = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "moves.h")

# AnimFrame indices
X, Y, ZOOMX, ANGLE, MIRROR, BLENDTYPE, VISIBLE, PATTERN, OPACITY = range(9)
ZOOMY = 11
COLORRED, COLORGREEN, COLORBLUE, COLORALPHA = 12, 13, 14, 15
TONERED, TONEGREEN, TONEBLUE, TONEGRAY = 16, 17, 18, 19
LOCKED = 20
FLASHRED, FLASHGREEN, FLASHBLUE, FLASHALPHA = 21, 22, 23, 24
PRIORITY, FOCUS = 25, 26

FOCUSUSER = (128, 224)
FOCUSTARGET = (384, 96)
SCREEN = (512, 384)
FPS = 20


def txt(v):
    return v.decode("utf-8", "replace") if isinstance(v, bytes) else ("" if v is None else str(v))


def color(v):
    """RGSS Color/Tone _dump = 4 little-endian doubles."""
    if isinstance(v, (bytes, bytearray)) and len(v) == 32:
        return tuple(round(c) for c in struct.unpack("<4d", v))
    return None


def g(cel, i, default=None):
    if cel is None or len(cel) <= i or cel[i] is None:
        return default
    return cel[i]


def load_all():
    root = marshal_lite.load(RXDATA)
    out = {}
    for a in root:
        if not a:
            continue
        name = txt(a.iv.get("name"))
        out.setdefault(name, a)
    return out


def moves_h_names():
    s = io.open(MOVES_H, encoding="utf-8", errors="replace").read()
    return re.findall(r'^\s*\{\s*"([A-Z0-9]+)",\s*T_', s, flags=re.M)


def timings(anim):
    res = []
    for t in (anim.iv.get("timing") or []):
        iv = t.iv
        res.append(dict(
            frame=iv.get("frame", 0), type=iv.get("timingType") or 0,
            name=txt(iv.get("name")), volume=iv.get("volume"), pitch=iv.get("pitch"),
            bgX=iv.get("bgX"), bgY=iv.get("bgY"), opacity=iv.get("opacity"),
            rgba=(iv.get("colorRed"), iv.get("colorGreen"), iv.get("colorBlue"), iv.get("colorAlpha")),
            duration=iv.get("duration") if iv.get("duration") is not None else 5,
            flashScope=iv.get("flashScope") or 0, flashColor=color(iv.get("flashColor")),
            flashDuration=iv.get("flashDuration"),
        ))
    return res


if __name__ == "__main__":
    allA = load_all()
    print(len(allA), "animations")
    nm = sys.argv[1] if len(sys.argv) > 1 else "Move:EXTREMESPEED"
    a = allA[nm]
    print({k: (v if not isinstance(v, list) else len(v)) for k, v in a.iv.items() if k != "array"})
    for fi, fr in enumerate(a):
        print("frame", fi, "ncel", len(fr))
        for ci, c in enumerate(fr):
            if c is None:
                print("   ", ci, None); continue
            print("   ", ci, "pat", g(c, PATTERN), "xy", g(c, X), g(c, Y), "z", g(c, ZOOMX), g(c, ZOOMY),
                  "ang", g(c, ANGLE), "mir", g(c, MIRROR), "op", g(c, OPACITY), "bl", g(c, BLENDTYPE),
                  "vis", g(c, VISIBLE), "pri", g(c, PRIORITY), "foc", g(c, FOCUS), "len", len(c),
                  "col", [g(c, i) for i in (12, 13, 14, 15)], "tone", [g(c, i) for i in (16, 17, 18, 19)])
        if fi > 3 and len(sys.argv) < 3:
            break
    for t in timings(a):
        print(t)
