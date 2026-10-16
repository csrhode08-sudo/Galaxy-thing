#!/usr/bin/env python3
"""List `#ifdef __MWERKS__` blocks without an #else that sit directly at the start of a function body
(the function would silently do nothing on PC)."""
import re, sys, glob
for f in glob.glob('src/**/*.cpp', recursive=True) + glob.glob('libs/**/*.h*', recursive=True) + glob.glob('include/**/*.h*', recursive=True):
    L = open(f, encoding='utf-8', errors='replace').read().split('\n')
    i = 0
    while i < len(L):
        if re.match(r'\s*#\s*if(def)?\s+(defined\()?__MWERKS__\)?\s*$', L[i]):
            depth = 0; j = i; has_else = False
            while j < len(L):
                if re.match(r'\s*#\s*if', L[j]): depth += 1
                elif re.match(r'\s*#\s*endif', L[j]):
                    depth -= 1
                    if depth == 0: break
                elif re.match(r'\s*#\s*(else|elif)', L[j]) and depth == 1: has_else = True
                j += 1
            prev = L[i-1].strip() if i > 0 else ''
            nxt = L[j+1].strip() if j+1 < len(L) else ''
            if not has_else and prev.endswith('{') and '(' in prev:
                print(f'{f}:{i+1}: {prev[:90]}  || after: {nxt[:40]}')
            i = j
        i += 1
