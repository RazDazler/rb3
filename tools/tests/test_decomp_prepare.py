"""Preparation must be read-only, version-bound and backed by exact evidence."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).parents[1]))
import decomp_prepare as prep


class PreparationTests(unittest.TestCase):
    def test_version_and_path_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'src/a.cpp').write_text('original')
            unit = {'name': 'main/a', 'target_path': 'build/SZBE69_B8/obj/a.o',
                    'base_path': 'build/SZBE69_B8/src/a.o', 'metadata': {'source_path': 'src/a.cpp'}}
            self.assertIn('main/a', prep.checked_project({'units': [unit]}, root))
            for path in ('build/SZBE69/obj/a.o', '../escape.o', 'src/a.cpp'):
                with self.assertRaises(ValueError):
                    prep.checked_project({'units': [unit | {'target_path': path}]}, root)

    def test_source_guard_detects_addition_deletion_and_changes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            p = root / 'src/a.cpp'
            p.write_bytes(b'old\r\n')
            before = prep.protected_snapshot(root)
            p.write_bytes(b'new\r\n')
            self.assertNotEqual(before, prep.protected_snapshot(root))
            p.unlink()
            self.assertNotEqual(before, prep.protected_snapshot(root))
            (root / 'src/new.h').write_text('new')
            self.assertIn('src/new.h', prep.protected_snapshot(root))

    def test_classifications_do_not_conflate_relocations_and_registers(self):
        def inspection(original, compiled, relocation=None):
            row = {'original': original, 'compiled': compiled,
                   'original_relocation': relocation, 'compiled_relocation': relocation}
            return {'match_percent': 99, 'instruction_differences': [row] if original != compiled else [],
                    'same_instruction_relocation_differences': [row] if original == compiled else []}
        pool = {'target': {'symbol': {'name': '@stringBase0'}}}
        self.assertEqual(prep.classification(inspection('lis r3, 0x0', 'lis r4, 0x0', pool)), 'string_metadata')
        self.assertEqual(prep.classification(inspection('add r3, r4, r5', 'add r4, r3, r5')), 'register_allocation_hint')
        self.assertEqual(prep.classification(inspection('addi r3, r4, 1', 'addi r3, r4, 2')), 'instruction_or_behavior')
        self.assertEqual(prep.classification(inspection('lwz r3, 0(r4)', 'lwz r3, 0(r4)')), 'relocations_or_symbol_ownership')

    def test_map_uses_exact_name_and_retains_line(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'original.map'
            path.write_text('  00000000 000004 80004000 000001e0  4 foo__Fv \toriginal.a foo.o\n'
                            '  00000004 000004 80004004 000001e4  4 notfoo__Fv \tother.a bar.o\n')
            result = prep.map_evidence(path, {'foo__Fv'})
            self.assertEqual(len(result['foo__Fv']), 1)
            self.assertEqual(result['foo__Fv'][0]['line'], 1)
            self.assertIn('foo.o', result['foo__Fv'][0]['text'])

    def test_small_zero_score_functions_are_opt_in(self):
        report = {'units': [{'name': 'main/a', 'functions': [
            {'name': 'near', 'size': '24', 'fuzzy_match_percent': 95},
            {'name': 'missing', 'size': '32', 'fuzzy_match_percent': 0},
            {'name': 'large', 'size': '800', 'fuzzy_match_percent': 95}]}]}
        rows = prep.ranked_units(report, {'main/a': {}}, 70, 256)
        self.assertEqual([f['name'] for f in rows[0]['functions']], ['near'])
        rows = prep.ranked_units(report, {'main/a': {}}, 70, 256, True)
        self.assertEqual([f['name'] for f in rows[0]['functions']], ['near', 'missing'])

    def test_isolated_assembly_does_not_include_neighbor_function(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'a.s'
            path.write_text('.fn foo__Fv, global\n/* 80000000 0000  4E 80 00 20 */ blr\n.endfn foo__Fv\n'
                            '.fn bar__Fv, global\nli r3, 5\n.endfn bar__Fv\n')
            result = prep.assembly_function(path, 'foo__Fv')
            self.assertIn('"foo__Fv":', result)
            self.assertNotIn('bar__Fv', result)
            self.assertNotIn('80000000', result)

    def test_pending_trial_refuses_recovery_or_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            state = root / 'build/prep'
            state.mkdir(parents=True)
            pending = root / 'pending.json'
            pending.write_text('{}')
            runner = SimpleNamespace(lock_path=root / 'native.lock', journal=pending)
            args = SimpleNamespace(minutes=1, timeout=5, jobs=1)
            with patch.object(prep, 'STATE', state), patch.object(prep, 'ROOT', root), patch.object(prep, 'Runner', return_value=runner):
                with self.assertRaisesRegex(RuntimeError, 'Pending source trial'):
                    prep.run(args)
            self.assertTrue(pending.is_file())
            self.assertEqual(json.loads((state / 'latest.json').read_text())['status'], 'failed')

    def test_stop_file_prevents_all_native_work(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'STOP').write_text('stop')
            runner = SimpleNamespace(lock_path=root / 'native.lock')
            with patch.object(prep, 'STATE', root), patch.object(prep, 'Runner', return_value=runner):
                result = prep.run(SimpleNamespace(minutes=1, timeout=5, jobs=1))
            self.assertEqual(result['status'], 'stop_file')
            self.assertEqual(result['units_compared'], 0)

    def test_m2c_has_no_context_cache_or_source_output_argument(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / 'packet.json'
            with patch.object(prep.subprocess, 'run', return_value=SimpleNamespace(returncode=0)) as run:
                result = prep.m2c_review(Path(tmp) / 'm2c.py', Path(tmp) / 'original.s', out, 3)
            argv = run.call_args.args[0]
            self.assertIn('--no-cache', argv)
            self.assertEqual(run.call_args.kwargs['cwd'], out.parent)
            self.assertEqual(result['status'], 'unreviewed_pseudocode')

    def test_repeat_uses_catalog_and_changed_object_invalidates_it(self):
        from unittest.mock import Mock
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for directory in ('src', 'config/SZBE69_B8', 'build/SZBE69_B8', 'build/native', 'tools'):
                (root / directory).mkdir(parents=True, exist_ok=True)
            (root / 'src/sample.cpp').write_text('original source')
            (root / 'config/SZBE69_B8/objects.json').write_text(json.dumps({'sample': {'objects': {'sample.cpp': 'NonMatching'}}}))
            (root / 'tools/decomp_rtti.py').write_text('tool')
            unit = {'name': 'main/sample', 'target_path': 'build/SZBE69_B8/target.o',
                    'base_path': 'build/SZBE69_B8/base.o', 'metadata': {'source_path': 'src/sample.cpp'}}
            (root / 'objdiff.json').write_text(json.dumps({'units': [unit]}))
            for name in ('target.o', 'base.o', 'original.dol', 'main.dol', 'objdiff.exe'):
                (root / 'build/SZBE69_B8' / name).write_bytes(b'original')
            function = {'name': 'foo__Fv', 'size': '4', 'fuzzy_match_percent': 95}
            report = {'units': [unit | {'functions': [function], 'measures': {'total_code': '4', 'matched_code_percent': 0}}],
                      'measures': {'matched_code': '0'}}
            diff = {'left': {'sections': [{'kind': 'SECTION_TEXT', 'symbols': [{
                'symbol': {'name': 'foo__Fv'}, 'target': {}, 'match_percent': 95,
                'instructions': [{'diff_kind': 'DIFF_ARG_MISMATCH', 'instruction': {'formatted': 'lwz r3, 0(r4)'}}]}]}]},
                'right': {'sections': [{'symbols': [{'symbol': {'name': 'foo__Fv'},
                    'instructions': [{'diff_kind': 'DIFF_ARG_MISMATCH', 'instruction': {'formatted': 'lwz r5, 0(r4)'}}]}]}]}}
            runner = SimpleNamespace(lock_path=root / 'build/native.lock', journal=root / 'build/pending.json',
                                     original=root / 'build/SZBE69_B8/original.dol', expected=prep.sha1(root / 'build/SZBE69_B8/main.dol'),
                                     output=root / 'build/SZBE69_B8/main.dol', config=root / 'config/SZBE69_B8/objects.json',
                                     directory=root / 'build/native', objdiff=str(root / 'build/SZBE69_B8/objdiff.exe'), ninja='unused')
            runner.command = Mock(return_value=(0, root / 'build/log.txt'))
            runner.refresh = Mock(return_value=report)
            def compare(_):
                (runner.directory / 'main_sample-diff.json').write_text(json.dumps(diff))
            runner.compare = Mock(side_effect=compare)
            args = SimpleNamespace(minutes=1, limit=10, timeout=5, jobs=1, min_score=70, max_size=256,
                                   include_unmatched=False, unit=[], m2c=None)
            state = root / 'build/preparation'
            with patch.object(prep, 'STATE', state), patch.object(prep, 'ROOT', root), patch.object(prep, 'Runner', return_value=runner):
                first = prep.run(args)
                second = prep.run(args)
                self.assertEqual(first['units_compared'], 1)
                self.assertEqual(second['units_compared'], 0)
                self.assertEqual(second['cache_hits'], 1)
                runner.compare.assert_called_once()
                (root / 'build/SZBE69_B8/base.o').write_bytes(b'changed object')
                third = prep.run(args)
                self.assertEqual(third['units_compared'], 1)
                self.assertEqual(runner.compare.call_count, 2)
                self.assertEqual(third['protected_files_changed'], [])


if __name__ == '__main__':
    unittest.main()
