#!/usr/bin/env python3
"""For 'cannot initialize a variable/return object of type T* with ... BE<T>*' errors, retype the variable or accessor."""
import re, sys, glob, collections
pat = re.compile(r"^(\S+?):(\d+):\d+: error: cannot initialize (a variable|return object) of type '([\w:]+) \*' (?:\(aka '[^']*'\) )?with an (?:rvalue|lvalue) of type 'BE<([\w:]+)> \*")
todo = collections.defaultdict(list)
for f in glob.glob(sys.argv[1] + '/*.txt'):
    for l in open(f):
        m = pat.match(l)
        if m: todo[m.group(1)].append((int(m.group(2)), m.group(2+1), m.group(4)))
names = {'Vec':'Vec','f32':'f32','u16':'u16','s16':'s16','u32':'u32'}
for path, items in todo.items():
    src = open(path).read().split('\n')
    for n, kind, typ in set(items):
        l = src[n-1]
        if kind == 'a variable':
            l2 = re.sub(r'\b%s\s*\*\s*(\w+)\s*=' % typ, r'BE(%s)* \1 =' % typ, l, count=1)
        else:
            # return statement: fix the enclosing function's return type (search upward)
            for k in range(n-1, max(0, n-6), -1):
                l3 = re.sub(r'\b%s\s*\*\s*(\w+)\(' % typ, r'BE(%s)* \1(' % typ, src[k-1], count=1)
                if l3 != src[k-1]:
                    src[k-1] = l3; break
            else:
                print('unfixed ret', path, n)
            continue
        if l2 == l: print('unfixed', path, n, l.strip())
        src[n-1] = l2
    open(path, 'w').write('\n'.join(src))
