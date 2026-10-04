"""Cache local B8 Ghidra context under build/, never propose or edit source."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

from decomp_runner import ROOT, exclusive_lock, sha1, write_json

STATE = ROOT / 'build/decomp/ghidra/SZBE69_B8'
SYMBOLS = ROOT / 'config/SZBE69_B8/symbols.txt'
ORIGINAL = ROOT / 'orig/SZBE69_B8/sys/main.dol'
EXPECTED = 'e26b3daf41886f0d09670135910f2510cd093ae8'
SCRIPT = ROOT / 'tools/ghidra-scripts'


def symbol_addresses(text):
    return {m[1]: m[2] for m in re.finditer(
        r'^(\S+) = \S+:(0x[0-9A-Fa-f]+);[^\n]*type:function', text, re.M)}


def requests(names, addresses):
    missing = [name for name in names if name not in addresses]
    if missing:
        raise ValueError('Exact B8 function names required: ' + ', '.join(missing))
    return ''.join(addresses[name] + '\t' + name + '\n' for name in dict.fromkeys(names))


def run(args):
    home = args.ghidra_home.resolve()
    headless = home / 'support' / ('analyzeHeadless.bat' if os.name == 'nt' else 'analyzeHeadless')
    if not headless.is_file():
        raise ValueError('Select an installed Ghidra home containing support/analyzeHeadless')
    if sha1(ORIGINAL) != EXPECTED:
        raise ValueError('Original input must be the exact supported B8 DOL')
    STATE.mkdir(parents=True, exist_ok=True)
    identity = {'dol_sha1': EXPECTED, 'symbols_sha256': hashlib.sha256(SYMBOLS.read_bytes()).hexdigest(),
                'import_script_sha256': hashlib.sha256((SCRIPT / 'ImportB8Symbols.java').read_bytes()).hexdigest(),
                'ghidra_home': str(home)}
    stamp = STATE / 'project-inputs.json'
    project = STATE / 'project'
    project.mkdir(exist_ok=True)
    with exclusive_lock(STATE / 'context.lock'):
        if args.action == 'init':
            if stamp.exists():
                if json.loads(stamp.read_text()) != identity:
                    raise ValueError('Cached project inputs changed; retain it and select a new project before reimporting')
                print('Verified cached B8 Ghidra project; no reanalysis needed')
                return
            if (project / 'RB3_B8.gpr').exists():
                raise ValueError('Unverified existing project: inspect init.log before reuse')
            command = [str(headless), str(project), 'RB3_B8', '-import', str(ORIGINAL),
                       '-loader', 'GameCubeLoader', '-loader-autoloadMaps', 'false',
                       '-scriptPath', str(SCRIPT), '-preScript', 'ImportB8Symbols.java', str(SYMBOLS),
                       '-analysisTimeoutPerFile', '900', '-max-cpu', '4']
            log = STATE / 'init.log'
        else:
            if not stamp.exists() or json.loads(stamp.read_text()) != identity:
                raise ValueError('Initialize a verified project before exporting')
            if not args.symbol:
                raise ValueError('Supply at least one exact --symbol name')
            request_text = requests(args.symbol, symbol_addresses(SYMBOLS.read_text()))
            key = hashlib.sha256((request_text + identity['symbols_sha256'] +
                                 hashlib.sha256((SCRIPT / 'ExportB8Context.java').read_bytes()).hexdigest()).encode()).hexdigest()[:20]
            output = STATE / 'packets' / key
            if (output / 'index.tsv').exists():
                print('Cached Ghidra context:', output)
                return
            output.mkdir(parents=True, exist_ok=True)
            request_file = output / 'request.tsv'
            request_file.write_text(request_text, encoding='utf-8')
            command = [str(headless), str(project), 'RB3_B8', '-process', 'main.dol', '-readOnly',
                       '-noanalysis', '-scriptPath', str(SCRIPT), '-postScript', 'ExportB8Context.java',
                       str(request_file), str(output), '-max-cpu', '4']
            log = output / 'export.log'
        start = time.monotonic()
        environment = os.environ.copy()
        environment.setdefault('GHIDRA_HEADLESS_MAXMEM', '4G')
        with log.open('w', encoding='utf-8') as stream:
            result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=1800 if args.action == 'init' else 120 + 40 * len(args.symbol),
                                    env=environment)
        text = log.read_text(encoding='utf-8', errors='replace')
        if result.returncode or 'REPORT: Import succeeded' not in text and args.action == 'init':
            raise RuntimeError('Ghidra failed; review ' + str(log))
        if args.action == 'init':
            if 'B8 symbols:' not in text or 'REPORT: Analysis succeeded' not in text:
                raise RuntimeError('Import/analysis not confirmed; review ' + str(log))
            write_json(stamp, identity)
        elif not (output / 'index.tsv').is_file():
            raise RuntimeError('No context index exported; review ' + str(log))
        print(json.dumps({'action': args.action, 'elapsed_seconds': round(time.monotonic() - start, 2),
                          'log': str(log), 'source_edits': False}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['init', 'export'])
    parser.add_argument('--ghidra-home', type=Path,
                        default=ROOT / 'build/references/ghidra/ghidra_12.1.4_PUBLIC')
    parser.add_argument('--symbol', action='append', default=[])
    run(parser.parse_args())


if __name__ == '__main__':
    main()
