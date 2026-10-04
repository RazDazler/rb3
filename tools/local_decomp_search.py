"""Model-free bounded source search for explicitly understood B8 expressions.

Generate a manifest once, or run its variants with the transactional verifier.
Only independent float loads are reordered; the arithmetic expression/operation
chain and dependency order remain unchanged. No algebraic reassociation.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from decomp_runner import ROOT, Runner, exclusive_lock, write_json
from decomp_variants import run_variants
from local_decomp_agent import body_span


def project_z_variants(text):
    start, end = body_span(text, 'u32 WiiCam::ProjectZ(float z)')
    old = text[start:end]
    rows = [
        'float nearPlane = mNearPlane;',
        'float farPlane = mFarPlane;',
        'float minZ = mZRange.x;',
        'float distance = farPlane - nearPlane;',
        'float maxZ = mZRange.y;',
    ]
    # Refuse to experiment when the reviewed template has changed.
    expected = '{\n    ' + '\n    '.join(rows) + '''
    float range = maxZ - minZ;
    float farScale = farPlane / distance;
    float projected = farScale * nearPlane;
    projected = z * farScale - projected;
    z = projected / z;
    z = z * range + minZ;
    return 16777215.0f * z;
}'''
    if old.replace('\r\n', '\n') != expected:
        raise ValueError('ProjectZ no longer matches the semantically reviewed search template')
    suffix = expected[expected.index('    float range'):]
    variants = []
    for order in itertools.permutations(range(5)):
        if order.index(3) < max(order.index(0), order.index(1)) or order == tuple(range(5)):
            continue
        body = '{\n    ' + '\n    '.join(rows[i] for i in order) + '\n' + suffix
        variants.append({'name': 'load-order-' + ''.join(map(str, order)),
                         'replacements': [{'old': old, 'new': body}]})
    return {'unit': 'main/system/rndwii/Cam', 'symbol': 'ProjectZ__6WiiCamFf', 'variants': variants}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['generate', 'run'])
    parser.add_argument('--minutes', type=float, default=30)
    parser.add_argument('--timeout', type=float, default=120)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--limit', type=int, default=40)
    args = parser.parse_args()
    if min(args.minutes, args.timeout, args.jobs, args.limit) <= 0:
        parser.error('Limits must be positive')
    state = ROOT / 'build/local-agent/search'
    state.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'src/system/rndwii/Cam.cpp'
    manifest = project_z_variants(source.read_text(encoding='utf-8'))
    fingerprint = hashlib.sha256(source.read_bytes() + Path(__file__).read_bytes()).hexdigest()
    destination = state / (fingerprint + '-manifest.json')
    write_json(destination, manifest)
    print(f'{len(manifest["variants"])} reviewed variants saved to {destination}', flush=True)
    if args.mode == 'generate':
        return
    result_path = state / (fingerprint + '-result.json')
    if result_path.exists():
        old = json.loads(result_path.read_text())
        tested = {item['name'] for item in old['attempts']}
        manifest['variants'] = [item for item in manifest['variants'] if item['name'] not in tested]
        if not manifest['variants']:
            print('All variants were previously tested; no repeated work.')
            return
    else:
        old = {'attempts': []}
    args.version, args.quota_file, args.allow_new_helper = 'SZBE69_B8', None, []
    args.stop_file = ROOT / 'build/local-agent/STOP'
    runner = Runner(args)
    with exclusive_lock(runner.lock_path):
        result = run_variants(runner, manifest)
    result['attempts'] = old['attempts'] + result['attempts']
    result['semantic_basis'] = 'Only independent nonvolatile member loads reordered; distance follows near/far reads; all arithmetic chain expressions unchanged.'
    write_json(result_path, result)
    print(json.dumps({k: v for k, v in result.items() if k != 'attempts'}, indent=2))


if __name__ == '__main__':
    main()
