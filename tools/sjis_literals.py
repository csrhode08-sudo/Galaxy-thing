#!/usr/bin/env python3
"""Rewrites non-ASCII characters in narrow string literals as Shift-JIS octal escapes.

The game was built with CodeWarrior in Shift-JIS, so its Japanese string literals (object, demo, camera and
animation names, ...) are compared against Shift-JIS strings and hashes from the disc. The decomp's sources are
UTF-8; clang would keep the literals in UTF-8 and every such lookup would fail. This runs before compiling.

usage: sjis_literals.py <in> <out>      (writes <out> only if its content changes)
       sjis_literals.py --check <file>  (exit 0 if the file has a non-ASCII narrow string literal)
"""
import sys


def encode_char(ch):
    for enc in ("cp932", "shift_jis"):
        try:
            return ch.encode(enc)
        except UnicodeEncodeError:
            pass
    return ch.encode("utf-8")


def convert(text):
    out = []
    i = 0
    n = len(text)
    changed = False
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(text[i:j])
            i = j
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(text[i:j])
            i = j
            continue
        if c == "'":
            j = i + 1
            while j < n and text[j] != "'" and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(text[i : j + 1])
            i = j + 1
            continue
        if c == '"':
            # Prefixed literals (L"", u"", U"", u8"") and raw strings keep their encoding.
            k = i - 1
            prefix = ""
            while k >= 0 and (text[k].isalnum() or text[k] == "_"):
                prefix = text[k] + prefix
                k -= 1
            j = i + 1
            while j < n and text[j] != '"' and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            body = text[i + 1 : j]
            if prefix in ("", ) and any(ord(ch) > 127 for ch in body):
                conv = []
                for ch in body:
                    if ord(ch) > 127:
                        conv.extend("\\%03o" % b for b in encode_char(ch))
                    else:
                        conv.append(ch)
                out.append('"' + "".join(conv) + '"')
                changed = True
            else:
                out.append(text[i : j + 1])
            i = j + 1
            continue
        out.append(c)
        i += 1
    return "".join(out), changed


def main():
    if sys.argv[1] == "--scan":
        # --scan <root> <dir>...: print (relative to root, ';'-separated) every source/header that needs converting.
        import os

        root = sys.argv[2]
        found = []
        for d in sys.argv[3:]:
            for dp, _, fs in os.walk(os.path.join(root, d)):
                for f in sorted(fs):
                    if not f.endswith((".c", ".cpp", ".h", ".hpp", ".inc")):
                        continue
                    p = os.path.join(dp, f)
                    text = open(p, encoding="utf-8", errors="surrogateescape").read()
                    if any(ord(ch) > 127 for ch in text) and convert(text)[1]:
                        found.append(os.path.relpath(p, root).replace(os.sep, "/"))
        sys.stdout.write(";".join(sorted(found)))
        return
    if sys.argv[1] == "--check":
        text = open(sys.argv[2], encoding="utf-8", errors="surrogateescape").read()
        sys.exit(0 if convert(text)[1] else 1)
    src, dst = sys.argv[1], sys.argv[2]
    text = open(src, encoding="utf-8", errors="surrogateescape").read()
    result, _ = convert(text)
    try:
        if open(dst, encoding="utf-8", errors="surrogateescape").read() == result:
            return
    except OSError:
        pass
    import os

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
        f.write(result)


if __name__ == "__main__":
    main()
