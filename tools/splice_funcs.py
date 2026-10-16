#!/usr/bin/env python3
"""Replace function definitions in a target file with versions from a reference file.
usage: splice_funcs.py <target> <reference> name1 [name2 ...]"""
import re, sys

def find_func(src, name):
    m = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\s*\(', src, re.M)
    if not m:
        return None
    start = m.start()
    i = src.index('{', m.end())
    depth = 0
    j = i
    while j < len(src):
        c = src[j]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                return start, j + 1
        j += 1
    return None

target, ref = sys.argv[1], sys.argv[2]
t = open(target).read()
r = open(ref).read()
for name in sys.argv[3:]:
    a = find_func(t, name)
    b = find_func(r, name)
    if not a or not b:
        print('not found:', name, bool(a), bool(b))
        continue
    t = t[:a[0]] + r[b[0]:b[1]] + t[a[1]:]
    print('spliced', name)
open(target, 'w').write(t)
