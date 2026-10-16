#!/usr/bin/env python3
"""Syntax-check game sources in parallel and aggregate the most common errors.
usage: check.py <regex> [sample_every_n] [jobs]"""
import json, re, subprocess, sys, os, collections, concurrent.futures, shlex
build = os.path.join(os.path.dirname(__file__), '..', 'build')
cc = json.load(open(os.path.join(build, 'compile_commands.json')))
pat = re.compile(sys.argv[1] if len(sys.argv) > 1 else r'/src/(Game|JSystem|nw4r)/')
every = int(sys.argv[2]) if len(sys.argv) > 2 else 1
jobs = int(sys.argv[3]) if len(sys.argv) > 3 else 3
ents = [e for e in cc if pat.search(e['file'])][::every]
outdir = os.path.join(build, 'errs'); os.makedirs(outdir, exist_ok=True)
def run(e):
    args = shlex.split(e['command'])
    i = args.index('-o'); del args[i:i+2]
    args += ['-fsyntax-only', '-ferror-limit=0', '-w']
    p = subprocess.run(args, cwd=e['directory'], capture_output=True, text=True)
    return e['file'], p.returncode, p.stderr
msgs = collections.Counter(); locs = collections.Counter(); bad = 0
with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
    for f, rc, err in ex.map(run, ents):
        name = f.split('galaxy-pc/')[-1].replace('/', '_')
        if rc:
            bad += 1
            open(os.path.join(outdir, name + '.txt'), 'w').write(err)
        else:
            try: os.remove(os.path.join(outdir, name + '.txt'))
            except FileNotFoundError: pass
        for m in re.finditer(r'^(\S+?):(\d+):\d+: (?:fatal )?error: (.*)$', err, re.M):
            loc = m.group(1).split('galaxy-pc/')[-1] + ':' + m.group(2)
            msgs[re.sub(r"'[^']*'", "'X'", m.group(3))] += 1
            locs[loc + '  ' + m.group(3)[:110]] += 1
print(f'{len(ents)} files, {bad} failed')
print('--- top messages'); [print(f'{n:6} {m[:150]}') for m, n in msgs.most_common(40)]
print('--- top locations'); [print(f'{n:6} {m}') for m, n in locs.most_common(80)]
