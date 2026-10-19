#!/usr/bin/env python3
"""Make forward declarations use the same class-key (class/struct) as the type's definition.

MSVC-ABI name mangling encodes class vs struct, so a mismatch between a forward declaration and the
definition produces different symbol names in different translation units (unresolved externals on Windows).
"""
import os, re, sys
from collections import defaultdict
roots = ["include", "libs", "src", "pc"]
exts = (".h", ".hpp", ".c", ".cpp", ".inc")
files = []
for r in roots:
    for dp, _, fn in os.walk(r):
        for f in fn:
            if f.endswith(exts):
                files.append(os.path.join(dp, f))
defre = re.compile(r"^\s*(?:template\s*<[^;{]*>\s*)?(class|struct)\s+(?:ATTRIBUTE_ALIGN\([^)]*\)\s*|__attribute__\(\([^)]*\)\)\s*)?([A-Za-z_]\w*)\s*(?:final\s*)?(?:\{|:(?!:))", re.M)
defs = defaultdict(set)
text = {}
for f in files:
    try:
        s = open(f, encoding="utf-8", errors="surrogateescape").read()
    except Exception:
        continue
    text[f] = s
    for m in defre.finditer(s):
        defs[m.group(2)].add(m.group(1))
uniq = {n: next(iter(k)) for n, k in defs.items() if len(k) == 1}
fwd = re.compile(r"(?<![<,]\s)(?<![<,])\b(class|struct)(\s+)([A-Za-z_]\w*)(\s*[;*&),>])")
changed = 0
for f, s in text.items():
    def rep(m):
        n = m.group(3)
        want = uniq.get(n)
        if want and want != m.group(1):
            return want + m.group(2) + n + m.group(4)
        return m.group(0)
    ns = fwd.sub(rep, s)
    if ns != s:
        changed += 1
        open(f, "w", encoding="utf-8", errors="surrogateescape").write(ns)
        if "-v" in sys.argv:
            print(f)
print("files changed:", changed)
