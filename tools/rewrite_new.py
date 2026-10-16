#!/usr/bin/env python3
"""Rewrite C++ new-expressions in game code to carry a JKRHeapToken so they allocate from the
game's JKRHeap system on PC, while host libraries keep using the normal allocator.
  new T(...)          -> new (JKRHeapToken::Dummy) T(...)
  new (a, b) T(...)   -> new (JKRHeapToken::Dummy, a, b) T(...)
Idempotent; skips comments, strings and `operator new`."""
import re, sys, os

TOKEN = 'JKRHeapToken::Dummy'

def find_code_spans(s):
    """Yield (start, end) of regions that are code (not comments/strings/char literals)."""
    i, n, start = 0, len(s), 0
    while i < n:
        c = s[i]
        if c == '/' and i + 1 < n and s[i+1] == '/':
            yield start, i; j = s.find('\n', i); i = n if j < 0 else j; start = i
        elif c == '/' and i + 1 < n and s[i+1] == '*':
            yield start, i; j = s.find('*/', i + 2); i = n if j < 0 else j + 2; start = i
        elif c in '"\'':
            yield start, i; j = i + 1
            while j < n and s[j] != c:
                j += 2 if s[j] == '\\' else 1
            i = j + 1; start = i
        else:
            i += 1
    yield start, n

ident = re.compile(r'[A-Za-z_:]')

def rewrite(s):
    out, last, count = [], 0, 0
    for a, b in find_code_spans(s):
        for m in re.finditer(r'\bnew\b', s[a:b]):
            p = a + m.start()
            pre = s[max(0, p-40):p]
            if re.search(r'operator\s*$', pre) or re.search(r'#\s*define\s+\w+\s*$', pre):
                continue
            q = a + m.end()
            while q < len(s) and s[q] in ' \t\n': q += 1
            if q >= len(s): continue
            if s[q] == '(':
                depth, r = 0, q
                while r < len(s):
                    if s[r] == '(': depth += 1
                    elif s[r] == ')':
                        depth -= 1
                        if depth == 0: break
                    r += 1
                inner = s[q+1:r].strip()
                after = s[r+1:r+80].lstrip()
                if inner.startswith(TOKEN):
                    continue
                if after and ident.match(after[0]):
                    out.append(s[last:q+1]); out.append(TOKEN + (', ' if inner else ''))
                    last = q + 1; count += 1
                else:
                    out.append(s[last:p]); out.append('new (' + TOKEN + ') ')
                    last = a + m.end(); count += 1
            elif ident.match(s[q]):
                out.append(s[last:p]); out.append('new (' + TOKEN + ') ')
                last = q; count += 1
    out.append(s[last:])
    return ''.join(out), count

total = 0
for root in sys.argv[1:]:
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if not f.endswith(('.cpp', '.hpp', '.h', '.inc')): continue
            path = os.path.join(dp, f)
            s = open(path, encoding='utf-8', errors='surrogateescape').read()
            ns, c = rewrite(s)
            if c:
                open(path, 'w', encoding='utf-8', errors='surrogateescape').write(ns)
                total += c
print('rewrote', total)
