#!/usr/bin/env python3
"""For 'jump bypasses variable initialization' notes inside switch statements, wrap the case body
that declares the variable in braces (CodeWarrior allowed jumping past initializations).
usage: fix_case_scopes.py <errs dir>"""
import re, glob, sys, collections
notes = collections.defaultdict(set)
for f in glob.glob(sys.argv[1] + '/*.txt'):
    txt = open(f).read()
    for m in re.finditer(r'^(\S+?):(\d+):\d+: error: cannot jump from switch statement to this case label\n(?:.*\n)*?^(\S+?):(\d+):\d+: note: jump bypasses variable initialization', txt, re.M):
        notes[m.group(3)].add(int(m.group(4)))
label = re.compile(r'^(\s*)(case\b.*|default\s*):\s*(\{)?\s*(//.*)?$')
for path, decls in notes.items():
    L = open(path).read().split('\n')
    edits = []
    for d in decls:
        i = d - 1
        while i >= 0 and not label.match(L[i]):
            i -= 1
        if i < 0 or label.match(L[i]).group(3):
            continue
        ind = label.match(L[i]).group(1)
        j = i + 1
        while j < len(L):
            m = label.match(L[j])
            if m and m.group(1) == ind:
                break
            if L[j].strip() == '}' and len(L[j]) - len(L[j].lstrip()) < len(ind):
                break
            j += 1
        edits.append((i, j, ind))
    for i, j, ind in sorted(set(edits), reverse=True):
        L.insert(j, ind + '}')
        L[i] = L[i].rstrip() + ' {'
    open(path, 'w').write('\n'.join(L))
    print(path, len(set(edits)))
