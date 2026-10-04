"""Local-model boundary, stale edits and transactional review-only behavior."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).parents[1]))
import local_decomp_agent as agent
import decomp_source_trial as trial
import local_decomp_search as search


class Boundaries(unittest.TestCase):
    def test_cache_ignores_formatting_but_preserves_token_boundaries(self):
        self.assertEqual(agent.canonical_body('{return x+1;}'), agent.canonical_body('{ /* rationale */ return x + 1; }'))
        self.assertNotEqual(agent.canonical_body('{float x;}'), agent.canonical_body('{floatx;}'))
        self.assertNotEqual(agent.canonical_body('{return x + +y;}'), agent.canonical_body('{return x++ + y;}'))

    def test_deterministic_search_preserves_dependencies_and_arithmetic(self):
        rows = ['float nearPlane = mNearPlane;', 'float farPlane = mFarPlane;',
                'float minZ = mZRange.x;', 'float distance = farPlane - nearPlane;',
                'float maxZ = mZRange.y;']
        tail = '''    float range = maxZ - minZ;
    float farScale = farPlane / distance;
    float projected = farScale * nearPlane;
    projected = z * farScale - projected;
    z = projected / z;
    z = z * range + minZ;
    return 16777215.0f * z;
}'''
        source = 'u32 WiiCam::ProjectZ(float z) {\n    ' + '\n    '.join(rows) + '\n' + tail
        variants = search.project_z_variants(source)['variants']
        self.assertEqual(len(variants), 39)
        for item in variants:
            body = item['replacements'][0]['new']
            self.assertTrue(body.endswith(tail))
            for row in rows: self.assertEqual(body.count(row), 1)
            self.assertGreater(body.index(rows[3]), body.index(rows[0]))
            self.assertGreater(body.index(rows[3]), body.index(rows[1]))
        with self.assertRaises(ValueError):
            search.project_z_variants(source.replace('16777215.0f', '16777214.0f'))

    def test_local_endpoint_no_proxy_or_remote(self):
        self.assertEqual(agent.localhost_url('http://127.0.0.1:11435/'), 'http://127.0.0.1:11435')
        for value in ('https://example.com', 'http://127.0.0.1.evil.test:11435',
                      'http://user:password@localhost:11435', 'http://localhost/remote',
                      'http://localhost?url=cloud', 'http://localhost#remote'):
            with self.assertRaises(ValueError):
                agent.localhost_url(value)

    def test_comments_and_literals_do_not_move_edit_boundaries(self):
        text = 'int f() { const char *s = "}"; /* } */ if (a) { a++; } return 1; }\nint g() {}'
        start, end = agent.body_span(text, 'int f()')
        self.assertEqual(text[end:], '\nint g() {}')
        self.assertTrue(text[start:end].startswith('{ const'))
        with self.assertRaises(ValueError):
            agent.body_span(text, 'int missing()')

    def test_path_cannot_leave_source_tree(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'src').mkdir()
            (root / 'src/f.cpp').write_text('int f() {}')
            (root / 'config.cpp').write_text('int f() {}')
            self.assertEqual(agent.source_path('src/f.cpp', root), root / 'src/f.cpp')
            for path in ('src/../config.cpp', str(root / 'config.cpp'), 'src/missing.cpp'):
                with self.assertRaises(ValueError):
                    agent.source_path(path, root)

    def test_multiline_signature_preserves_windows_offsets(self):
        text = 'void A::f(\r\n    const B &b, float x\r\n) {\r\n    use(b, x);\r\n}\r\nvoid g() {}'
        start, end = agent.body_span(text, 'void A::f(\n    const B &b, float x\n)')
        self.assertEqual(text[start:end], '{\r\n    use(b, x);\r\n}')
        with self.assertRaises(ValueError):
            agent.body_span(text, 'void A::f(\n    const C &b, float x\n)')

    def test_model_cannot_choose_files_tools_directives_or_helpers(self):
        old = '{ float x = mNearPlane; return x * 16777215.0f; }'
        self.assertEqual(agent.validate_proposal({'body': '{ float y = mNearPlane; return y * 16777215.0f; }', 'reason': 'lifetime'}, old), '{ float y = mNearPlane; return y * 16777215.0f; }')
        bad = [
            {'body': old, 'reason': 'x', 'files': ['config/objects.json']},
            {'body': '{} void evil() {}', 'reason': 'x'},
            {'body': '{\n#include "hack.h"\nreturn 1;}', 'reason': 'x'},
            {'body': '{ asm("blr"); }', 'reason': 'x'},
            {'body': '{ volatile float x; return x; }', 'reason': 'x'},
            {'body': '{ return system("cmd"); }', 'reason': 'x'},
            {'body': '{ return (*(float *)0x81234567); }', 'reason': 'x'},
            {'body': '{ return invented_helper(mNearPlane); }', 'reason': 'x'},
            {'body': '{ static float x; return x; }', 'reason': 'x'},
            {'body': '{ return 1; } /* hide */ int global = 1;', 'reason': 'x'},
            {'body': '{ class Hidden {}; return 1; }', 'reason': 'x'},
        ]
        for proposed in bad:
            with self.subTest(proposed=proposed), self.assertRaises(ValueError):
                agent.validate_proposal(proposed, old)

    def test_explicit_b8_definition_selection(self):
        text = '#ifdef VERSION_SZBE69_B8\nint f() { return 1; }\n#else\nint f() { return 2; }\n#endif'
        start, end = agent.body_span(text, 'int f()', 0)
        self.assertEqual(text[start:end], '{ return 1; }')

    def test_existing_escaped_literals_are_data_not_directives(self):
        old = '{ warn("value #1\\n"); return x; }'
        self.assertEqual(agent.validate_proposal({'body': old, 'reason': 'same'}, old), old)
        for body in ('{ warn("new\\n"); return x; }', '{ return x; \\\nreturn x; }'):
            with self.assertRaises(ValueError):
                agent.validate_proposal({'body': body, 'reason': 'invalid'}, old)

    def test_new_constants_require_explicit_trusted_context(self):
        proposed = {'body': '{ return x >> 31; }', 'reason': 'reviewed sign extraction'}
        with self.assertRaises(ValueError):
            agent.validate_proposal(proposed, '{ return x; }')
        self.assertEqual(agent.validate_proposal(proposed, '{ return x; }', ['31']), proposed['body'])
        for context in (['31;system()'], [31], ['']):
            with self.assertRaises(ValueError):
                agent.validate_proposal(proposed, '{ return x; }', context)

    def test_ternaries_and_switch_labels_are_not_statement_labels(self):
        old = '{ return x; }'
        for body in ('{ return x < 0 ? -x : x; }',
                     '{ switch (x) { case Mode::Ready: return 1; default: return 0; } }'):
            self.assertEqual(agent.validate_proposal({'body': body, 'reason': 'ordinary control flow'}, old), body)
        with self.assertRaises(ValueError):
            agent.validate_proposal({'body': '{ hidden: return x; }', 'reason': 'label'}, old)

    def test_stale_source_and_config_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'f.cpp'
            config = Path(folder) / 'config.json'
            path.write_text('original')
            config.write_text('{}')
            runner = argparse.Namespace(config=config)
            snap = {'source_sha256': agent.digest(path), 'config_sha256': agent.digest(config)}
            with patch.object(agent, 'source_path', return_value=path):
                self.assertFalse(agent.stale(runner, {'source': 'f.cpp'}, snap))
                path.write_text('changed by another worker')
                self.assertTrue(agent.stale(runner, {'source': 'f.cpp'}, snap))
                path.write_text('original')
                config.write_text('{"changed":true}')
                self.assertTrue(agent.stale(runner, {'source': 'f.cpp'}, snap))

    def test_fingerprint_changes_with_model_source_and_policy(self):
        key = agent.fingerprint({'id': 'task'}, {'body': '{}'}, 'model1')
        self.assertNotEqual(key, agent.fingerprint({'id': 'task'}, {'body': '{return 1;}'}, 'model1'))
        self.assertNotEqual(key, agent.fingerprint({'id': 'task'}, {'body': '{}'}, 'model2'))


class ReviewTransaction(unittest.TestCase):
    def test_improvement_is_verified_then_restored(self):
        """Exercise the existing source trial, not a mirrored local implementation."""
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source, config, dol = (root / name for name in ('f.cpp', 'objects.json', 'main.dol'))
            source.write_bytes(b'int f() {return 1;}')
            config.write_bytes(b'{}')
            dol.write_bytes(b'pinned executable')
            originals = {p: p.read_bytes() for p in (source, config, dol)}
            class FakeRunner:
                args = argparse.Namespace(adopt_unit=False, review_only=True, allow_new_helper=[])
                directory, output, original = root, dol, dol
                journal = root / 'pending.json'
                expected = hashlib.sha1(dol.read_bytes()).hexdigest()
                config = root / 'objects.json'
                counter = 0
                def stop_reason(self): return None
                def build(self, label): return True, root / 'build.log'
                def refresh(self):
                    self.counter += 1
                    return {'measures': {'matched_code': 4 if self.counter == 1 else 8}}
                def compare(self, unit):
                    return [{'name': 'f', 'match_percent': 50 if self.counter == 1 else 100}]
                def recover(self):
                    if self.journal.exists():
                        for path, data in originals.items(): path.write_bytes(data)
                        self.journal.unlink()
            runner = FakeRunner()
            (root / 'unit-diff.json').write_text('{}')
            with patch.object(trial, 'prepare_sources', return_value=[(source, source.read_bytes(), b'int f() {return 2;}')]), patch.object(trial, 'inspect_function', side_effect=ValueError('no fake assembly')):
                result = trial.run_trial(runner, {'files': [{}], 'unit': 'unit', 'symbol': 'f'})
            self.assertTrue(result['accepted'])
            self.assertFalse(result['retained'])
            self.assertEqual(result['measure_gains']['matched_code'], 4)
            for path, original in originals.items(): self.assertEqual(path.read_bytes(), original)
            self.assertFalse(runner.journal.exists())


if __name__ == '__main__':
    unittest.main()
