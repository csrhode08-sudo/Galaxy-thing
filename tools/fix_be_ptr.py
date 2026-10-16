#!/usr/bin/env python3
"""For 'incompatible pointer types assigning to BE<T>* from T*' errors, wrap the JSUConvertOffsetToPtr / cast type in BE().
usage: fix_be_ptr.py build/errs"""
import re, sys, glob, collections
pat = re.compile(r"^(\S+?):(\d+):\d+: error: (incompatible pointer types assigning to 'BE<|cannot initialize a member subobject of type 'BE<)")
todo = collections.defaultdict(set)
for f in glob.glob(sys.argv[1] + '/*.txt'):
    for l in open(f):
        m = pat.match(l)
        if m: todo[m.group(1)].add(int(m.group(2)))
for path, lines in todo.items():
    src = open(path).read().split('\n')
    for n in lines:
        l = src[n-1]
        l2 = re.sub(r'JSUConvertOffsetToPtr\s*<\s*([\w:]+)\s*>', r'JSUConvertOffsetToPtr< BE(\1) >', l, count=1)
        if l2 == l:
            l2 = re.sub(r'\((u16|s16|u32|s32|f32|Vec)\s*\*\)', r'(BE(\1)*)', l, count=1)
        if l2 == l: print('unfixed', path, n, l.strip())
        src[n-1] = l2
    open(path, 'w').write('\n'.join(src))
