#!/usr/bin/env python3
"""Carry Dusklight's PC-specific conditional blocks over to the matching Petari file.

Dusklight (CC0, a Twilight Princess PC port) and Petari share JSystem/nw4r code. Dusklight marks its
PC changes with `#if TARGET_PC` / `#if TARGET_LITTLE_ENDIAN` blocks. For each such block:
  * with an #else branch (the original code): if the original code's token sequence occurs exactly once
    in the Petari file, it is replaced by the whole conditional block;
  * without an #else (a pure addition): it is inserted after the uniquely matching preceding code.
Token comparison ignores whitespace/comments, so formatting differences don't matter.

usage: port_ifdefs.py <dusklight file> <petari file> [--dry] [-v]
"""
import re
import sys

COND_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
PC_COND = re.compile(r'TARGET_PC|TARGET_LITTLE_ENDIAN')
TOKEN_RE = re.compile(r'''
    (?P<ws>\s+) |
    (?P<lc>//[^\n]*) |
    (?P<bc>/\*.*?\*/) |
    (?P<str>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*') |
    (?P<num>\d[\w.]*) |
    (?P<id>[A-Za-z_]\w*) |
    (?P<pp>\#[^\n]*) |
    (?P<op>::|->|<<=|>>=|<=|>=|==|!=|&&|\|\||\+\+|--|[-+*/%&|^!~<>=?:;,.(){}\[\]])
''', re.S | re.X)


def tokenize(src):
    toks = []
    for m in TOKEN_RE.finditer(src):
        k = m.lastgroup
        if k in ('ws', 'lc', 'bc'):
            continue
        t = m.group(0)
        if k == 'id' and t in ('const',):  # keep const; it matters
            pass
        toks.append((t, m.start(), m.end()))
    return toks


def find_all(hay, needle):
    n = len(needle)
    if n == 0:
        return []
    first = needle[0]
    out = []
    for i in range(len(hay) - n + 1):
        if hay[i] == first and hay[i:i + n] == needle:
            out.append(i)
    return out


def parse_blocks(lines):
    """Yield (start, end, cond_line, pc_lines, orig_lines) for PC conditional groups (outermost only)."""
    i = 0
    n = len(lines)
    while i < n:
        m = COND_RE.match(lines[i])
        if m and PC_COND.search(m.group(2)) and '&&' not in m.group(2) and '||' not in m.group(2):
            kind, expr = m.group(1), m.group(2).strip()
            negated = kind == 'ifndef' or expr.startswith('!')
            depth = 0
            j = i
            branches = [[]]
            split_ok = True
            while j < n:
                l = lines[j]
                if re.match(r'^\s*#\s*if', l):
                    depth += 1
                    if depth > 1:
                        branches[-1].append(l)
                elif re.match(r'^\s*#\s*endif', l):
                    depth -= 1
                    if depth == 0:
                        break
                    branches[-1].append(l)
                elif re.match(r'^\s*#\s*elif', l) and depth == 1:
                    split_ok = False
                    branches[-1].append(l)
                elif re.match(r'^\s*#\s*else', l) and depth == 1:
                    branches.append([])
                elif j != i:
                    branches[-1].append(l)
                j += 1
            if split_ok:
                first = branches[0]
                second = branches[1] if len(branches) > 1 else []
                pc, orig = (second, first) if negated else (first, second)
                yield i, j, pc, orig
            i = j + 1
            continue
        i += 1


def main():
    ref, tgt = sys.argv[1], sys.argv[2]
    dry = '--dry' in sys.argv
    verbose = '-v' in sys.argv
    rlines = open(ref, encoding='utf-8', errors='surrogateescape').read().split('\n')
    src = open(tgt, encoding='utf-8', errors='surrogateescape').read()
    if 'TARGET_PC' in src and '--force' not in sys.argv:
        pass
    edits = []  # (start, end, text)
    stats = {'replaced': 0, 'inserted': 0, 'skipped': 0}
    ttoks = tokenize(src)
    tvals = [t[0] for t in ttoks]
    for start, end, pc, orig in parse_blocks(rlines):
        pc_text = '\n'.join(pc)
        orig_text = '\n'.join(orig)
        otoks = [t[0] for t in tokenize(orig_text)]
        if otoks:
            hits = find_all(tvals, otoks)
            if len(hits) != 1:
                # Disambiguate with up to 6 tokens of preceding context.
                ctx_lines = [l for l in rlines[max(0, start - 8):start] if l.strip() and not l.strip().startswith('#')]
                ctx = [t[0] for t in tokenize('\n'.join(ctx_lines))][-6:]
                if ctx and hits:
                    hits = [h for h in hits if tvals[max(0, h - len(ctx)):h] == ctx]
            if len(hits) == 1:
                h = hits[0]
                a = ttoks[h][1]
                b = ttoks[h + len(otoks) - 1][2]
                # expand to whole lines
                a = src.rfind('\n', 0, a) + 1
                e = src.find('\n', b)
                b = len(src) if e < 0 else e
                block = '#ifdef TARGET_PC\n' + pc_text + '\n#else\n' + src[a:b] + '\n#endif'
                edits.append((a, b, block))
                stats['replaced'] += 1
            else:
                stats['skipped'] += 1
                if verbose:
                    print(f'  skip (orig {len(hits)} hits) @{start + 1}: {orig_text.strip()[:80]!r}')
        else:
            ctx_lines = [l for l in rlines[max(0, start - 8):start] if l.strip() and not l.strip().startswith('#')]
            ctx = [t[0] for t in tokenize('\n'.join(ctx_lines))]
            hits = []
            for k in (12, 8, 5, 3):
                c = ctx[-k:]
                if len(c) < 2:
                    break
                hits = find_all(tvals, c)
                if len(hits) == 1:
                    ctx = c
                    break
            if len(hits) == 1 and pc_text.strip():
                h = hits[0] + len(ctx) - 1
                b = ttoks[h][2]
                e = src.find('\n', b)
                b = len(src) if e < 0 else e
                edits.append((b, b, '\n#ifdef TARGET_PC\n' + pc_text + '\n#endif'))
                stats['inserted'] += 1
            else:
                stats['skipped'] += 1
                if verbose:
                    print(f'  skip (insert, {len(hits)} anchors) @{start + 1}: {pc_text.strip()[:80]!r}')
    # apply from the end; drop overlapping edits
    edits.sort(key=lambda e: e[0], reverse=True)
    last = len(src) + 1
    for a, b, text in edits:
        if b > last:
            stats['skipped'] += 1
            continue
        src = src[:a] + text + src[b:]
        last = a
    print(f'{tgt}: {stats}')
    if not dry:
        open(tgt, 'w', encoding='utf-8', errors='surrogateescape').write(src)


if __name__ == '__main__':
    main()
