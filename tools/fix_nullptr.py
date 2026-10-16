#!/usr/bin/env python3
"""Replace `nullptr` with 0 on lines where clang reports nullptr used as a non-pointer (CodeWarrior allowed it).
usage: fix_nullptr.py build/errs"""
import re, sys, glob, collections
pat = re.compile(r"^(\S+?):(\d+):\d+: error: .*(nullptr_t|incompatible with nullptr|from .std::nullptr_t.)")
todo = collections.defaultdict(set)
for f in glob.glob(sys.argv[1] + '/*.txt'):
    for l in open(f):
        m = pat.match(l)
        if m:
            todo[m.group(1)].add(int(m.group(2)))
for path, lines in todo.items():
    src = open(path, encoding='utf-8', errors='surrogateescape').read().split('\n')
    for n in lines:
        src[n - 1] = re.sub(r'\bnullptr\b', '0', src[n - 1])
    open(path, 'w', encoding='utf-8', errors='surrogateescape').write('\n'.join(src))
    print(path, sorted(lines))
