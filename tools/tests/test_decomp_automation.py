"""Regression checks for configuration preservation and unattended failure handling."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
import sys

sys.path.insert(0, str(Path(__file__).parents[1]))


def load(name):
    spec = importlib.util.spec_from_file_location(name, Path(__file__).parents[1] / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


runner = load("decomp_runner")
audit = load("library_audit")
variants = load("decomp_variants")
fetcher = load("library_fetch")
dol = load("dol_compare")
inspector = load("decomp_inspect")
source_trial = load("decomp_source_trial")
literals = load("decomp_literals")


class AutomationTests(unittest.TestCase):
    def test_literal_proposal_preserves_c_escapes_and_rejects_code(self):
        assembly = '.obj "@stringBase0", local\n\t.string "file.cpp"\n\t.string "line\\n\\\"quoted\\\""\n.endobj "@stringBase0"\n'
        self.assertEqual(literals.pool_literals(assembly)[1], '"line\\n\\\"quoted\\\""')
        for extra in ('\t.4byte 0x0\n', '\t.string "x"; unexpected();\n'):
            with self.assertRaises(ValueError):
                literals.pool_literals(assembly.replace('.endobj', extra + '.endobj'))
        with self.assertRaises(ValueError):
            literals.pool_literals(assembly + assembly)

    def test_literal_manifest_refuses_mixed_version_and_preserves_source(self):
        import json
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            source = root / 'src' / 'sample.cpp'
            source.write_bytes(b'#include "sample.h"\r\n')
            (root / 'objdiff.json').write_text(json.dumps({'units': [{
                'name': 'main/sample', 'target_path': 'build/SZBE69/obj/sample.o',
                'metadata': {'source_path': 'src/sample.cpp'}}]}))
            with self.assertRaisesRegex(ValueError, 'different version'):
                literals.prepare_manifest('main/sample', 'SZBE69_B8', 'sample__Fv', root)
            self.assertEqual(source.read_bytes(), b'#include "sample.h"\r\n')

    def test_coordinated_trial_validates_all_sources_before_writing(self):
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            path = root / 'src' / 'a.cpp'
            path.write_bytes(b'original\r\n')
            edits = {'path': 'src/a.cpp', 'replacements': [{'old': 'original', 'new': 'trial'}]}
            with patch.object(source_trial, 'ROOT', root):
                with self.assertRaisesRegex(ValueError, 'more than once'):
                    source_trial.prepare_sources([edits, edits])
                with self.assertRaisesRegex(ValueError, 'under src'):
                    source_trial.prepare_sources([{'path': '../escape.cpp', 'replacements': []}])
            self.assertEqual(path.read_bytes(), b'original\r\n')

    def test_equal_function_score_only_allowed_for_layout_adoption(self):
        rows = [{'name': 'target', 'match_percent': 100, 'differences': {}}]
        self.assertFalse(variants.improvement(rows, rows, 'target')[0])
        self.assertTrue(variants.improvement(rows, rows, 'target',
                                            require_target_improvement=False)[0])
        worse = [{'name': 'target', 'match_percent': 99, 'differences': {}}]
        self.assertFalse(variants.improvement(rows, worse, 'target',
                                             require_target_improvement=False)[0])

    def test_inspection_accepts_omitted_zero_pair_indexes(self):
        original = {'symbol': {'name': 'first'}, 'target': {},
                    'match_percent': 100, 'instructions': []}
        compiled = {'symbol': {'name': 'first'}, 'instructions': []}
        diff = {'left': {'sections': [{'kind': 'SECTION_TEXT', 'symbols': [original]}]},
                'right': {'sections': [{'symbols': [compiled]}]}}
        self.assertEqual(inspector.inspect_function(diff, 'first')['match_percent'], 100)

    def test_auto_candidates_do_not_treat_omitted_zero_data_score_as_matching(self):
        report = {'units': [
            {'name': 'main/a', 'metadata': {'source_path': 'src/a.cpp'},
             'measures': {'total_code': '16', 'matched_code_percent': 100, 'total_data': '4'}},
            {'name': 'main/b', 'metadata': {'source_path': 'src/b.cpp'},
             'measures': {'total_code': '16', 'matched_code_percent': 100}}]}
        entries = {'a.cpp': 'NonMatching', 'b.cpp': 'NonMatching'}
        self.assertEqual([row['unit'] for row in runner.candidates(report, entries)], ['main/b'])
        self.assertEqual(len(runner.candidates(report, entries, 'main/a')), 1)

    def test_compare_refuses_other_version_before_invoking_objdiff(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'objdiff.json').write_text('{"units":[{"name":"main/test",'
                '"target_path":"build/SZBE69/obj/test.o","base_path":"build/SZBE69/src/test.o"}]}')
            instance = runner.Runner.__new__(runner.Runner)
            instance.args = SimpleNamespace(version='SZBE69_B8')
            instance.command = lambda *a: self.fail('Must not invoke objdiff with another version')
            with patch.object(runner, 'ROOT', root), self.assertRaises(RuntimeError):
                instance.compare('main/test')

    def test_versions_share_lock_and_debug_uses_pinned_build_manifest(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for version in ('SZBE69', 'SZBE69_B8'):
                config = root / 'config' / version; config.mkdir(parents=True)
                settings = f'object: orig/{version}/sys/main.dol\n'
                if version == 'SZBE69': settings += 'hash: ' + 'a' * 40 + '\n'
                (config / 'config.yml').write_text(settings)
                (config / 'build.sha1').write_text('b' * 40 + f'  build/{version}/main.dol\n')
            args = lambda version: SimpleNamespace(version=version, minutes=1)
            with patch.object(runner, 'ROOT', root):
                retail, debug = runner.Runner(args('SZBE69')), runner.Runner(args('SZBE69_B8'))
                self.assertEqual(retail.expected, 'a' * 40)
                self.assertEqual(debug.expected, 'b' * 40)
                self.assertEqual(retail.lock_path, debug.lock_path)
                manifest = root / 'config/SZBE69_B8/build.sha1'
                manifest.write_text(manifest.read_text() * 2)
                with self.assertRaises(ValueError): runner.Runner(args('SZBE69_B8'))

    def test_inspection_uses_explicit_pair_and_separates_relocations(self):
        original = {"symbol": {"name": "Load"}, "target": {"section_index": 0, "symbol_index": 1},
                    "instructions": [{"diff_kind": "DIFF_ARG_MISMATCH", "instruction": {"formatted": "bl 0x0"}},
                                     {"diff_kind": "DIFF_INSERT"}]}
        compiled = {"symbol": {"name": "mapped_Load"}, "instructions": [
            {"instruction": {"formatted": "bl 0x0"}}, {"instruction": {"formatted": "mr r3, r31"}}]}
        diff = {"left": {"sections": [{"kind": "SECTION_TEXT", "symbols": [original,
            {"symbol": {"name": "PreLoad"}}]}]}, "right": {"sections": [{"symbols": [{}, compiled]}]}}
        result = inspector.inspect_function(diff, "Load")
        self.assertEqual(result["compiled_symbol"], "mapped_Load")
        self.assertEqual(len(result["instruction_differences"]), 1)
        self.assertEqual(len(result["same_instruction_relocation_differences"]), 1)

    def test_cached_sources_refuse_modified_missing_and_extra_files(self):
        import io, tarfile
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); archive = root / "source.tar.gz"; tree = root / "tree"
            with tarfile.open(archive, "w:gz") as tar:
                entry = tarfile.TarInfo("release/file.c"); entry.size = 8
                tar.addfile(entry, io.BytesIO(b"original"))
            fetcher.extract_source(archive, tree)
            self.assertEqual(fetcher.verify_source(archive, tree), 1)
            file = tree / "release/file.c"; file.write_bytes(b"modified")
            with self.assertRaises(ValueError): fetcher.verify_source(archive, tree)
            file.unlink()
            with self.assertRaises(ValueError): fetcher.verify_source(archive, tree)
            file.write_bytes(b"original"); (tree / "extra").write_bytes(b"extra")
            with self.assertRaises(ValueError): fetcher.verify_source(archive, tree)

    def test_each_download_redirect_must_remain_https(self):
        with self.assertRaises(ValueError):
            fetcher.HTTPSRedirects().redirect_request(None, None, 302, "", {}, "http://example.com/source")

    def test_changed_macro_helper_diagnostic_preserves_actual_name(self):
        baseline = [{"name": "target", "match_percent": 90},
                    {"name": "FORCEACTIVEExample20__Fv", "bytes": 24, "side": "compiled", "unpaired": True}]
        candidate = [{"name": "target", "match_percent": 100},
                     {"name": "FORCEACTIVEExample35__Fv", "bytes": 32, "side": "compiled", "unpaired": True}]
        accepted, reason = variants.improvement(baseline, candidate, "target")
        self.assertFalse(accepted)
        self.assertIn("FORCEACTIVEExample35__Fv", reason)

    def test_dol_focus_uses_virtual_address_despite_different_file_offsets(self):
        import struct
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            def make(offset, payload):
                data = bytearray(offset + len(payload))
                struct.pack_into('>I', data, 0, offset)
                struct.pack_into('>I', data, 0x48, 0x80001000)
                struct.pack_into('>I', data, 0x90, len(payload))
                data[offset:] = payload
                return data
            a, b = root/'a.dol', root/'b.dol'
            a.write_bytes(make(0x100, b'abcd'))
            b.write_bytes(make(0x120, b'abXd'))
            result = dol.compare_dols(a, b, [{'start':0x80001000, 'end':0x80001004}])
            self.assertEqual(result['focus'][0]['differences'][0]['address'], '0x80001002')
            self.assertEqual(result['focus'][0]['differences'][0]['original_hex'], '63')
            self.assertEqual(result['focus'][0]['differences'][0]['rebuilt_hex'], '58')
        with self.assertRaises(ValueError):
            dol.sections(b'too short')

    def test_source_extraction_rejects_traversal_and_links_before_writing(self):
        import tarfile
        import io
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for filename, link in (("../escape.c", False), ("link.c", True)):
                archive = root / "source.tar.gz"
                with tarfile.open(archive, "w:gz") as tar:
                    member = tarfile.TarInfo(filename)
                    if link:
                        member.type, member.linkname = tarfile.SYMTYPE, "../escape.c"
                    else:
                        member.size = 1
                    tar.addfile(member, None if link else io.BytesIO(b"x"))
                with self.assertRaises(ValueError):
                    fetcher.extract_source(archive, root / "extract")
                self.assertFalse((root / "escape.c").exists())

    def test_variant_replacements_reject_ambiguous_edits(self):
        with self.assertRaises(ValueError):
            variants.apply_replacements(b"twice twice", [{"old": "twice", "new": "one"}])
        self.assertEqual(variants.apply_replacements(b"a\r\nb\r\n", [{"old": "a\nb", "new": "c\nd"}]), b"c\r\nd\r\n")

    def test_variant_mixed_newlines_preserve_unedited_bytes(self):
        original = b"prefix\r\na\nb\r\nsuffix\n"
        actual = variants.apply_replacements(original, [{"old": "a\nb", "new": "c\nd"}])
        self.assertEqual(actual, b"prefix\r\nc\r\nd\r\nsuffix\n")

    def test_variant_mixed_newline_matches_must_be_unique(self):
        with self.assertRaises(ValueError):
            variants.apply_replacements(b"a\nb a\r\nb", [{"old": "a\nb", "new": "c"}])

    def test_variant_requires_improvement_without_other_regressions(self):
        baseline = [{"name": "target", "match_percent": 90}, {"name": "other", "match_percent": 100}]
        improved = [{"name": "target", "match_percent": 95}, {"name": "other", "match_percent": 100}]
        self.assertTrue(variants.improvement(baseline, improved, "target")[0])
        improved[1]["match_percent"] = 99
        self.assertFalse(variants.improvement(baseline, improved, "target")[0])
        self.assertFalse(variants.improvement(baseline, baseline, "target")[0])

    def test_variant_does_not_accept_extra_compiler_helpers(self):
        baseline = [{"name": "target", "match_percent": 90}]
        candidate = [{"name": "target", "match_percent": 100}, {"name": "extra", "side": "compiled", "unpaired": True}]
        self.assertFalse(variants.improvement(baseline, candidate, "target")[0])

    def test_line_number_macro_renames_preserve_helper_count_and_size(self):
        baseline = [{"name":"target", "match_percent":90}, {"name":"FORCEACTIVEExample20__Fv", "bytes":24, "side":"compiled", "unpaired":True}]
        candidate = [{"name":"target", "match_percent":100}, {"name":"FORCEACTIVEExample30__Fv", "bytes":24, "side":"compiled", "unpaired":True}]
        self.assertTrue(variants.improvement(baseline, candidate, "target")[0])
        candidate.append(dict(candidate[-1], name="FORCEACTIVEExample40__Fv"))
        self.assertFalse(variants.improvement(baseline, candidate, "target")[0])

    def test_source_recovery_refuses_external_edit_before_config_restore(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            instance = runner.Runner.__new__(runner.Runner)
            instance.config, instance.journal = root / "objects.json", root / "pending.json"
            source = root / "source.cpp"
            instance.config.write_bytes(b"current config")
            source.write_bytes(b"external source edit")
            runner.write_json(instance.journal, {"trial_config_sha256": runner.digest(instance.config),
                "original_config_sha256": "original", "sources": [{"path": str(source), "allowed_sha256": ["trial"]}]})
            with self.assertRaises(RuntimeError):
                instance.recover()
            self.assertEqual(instance.config.read_bytes(), b"current config")
            self.assertEqual(source.read_bytes(), b"external source edit")

    def test_edit_preserves_neighbors_and_crlf(self):
        import json
        raw = b'{\r\n    "one.cpp": "NonMatching",\r\n    "two.cpp": {"status": "Matching", "note": "keep"}\r\n}\r\n'
        changed = runner.replace_entry(raw, "one.cpp", {"status": "Matching"})
        self.assertEqual(json.loads(changed)["two.cpp"], json.loads(raw)["two.cpp"])
        self.assertTrue(changed.endswith(b'    "two.cpp": {"status": "Matching", "note": "keep"}\r\n}\r\n'))
        self.assertNotIn(b"\n", changed.replace(b"\r\n", b""))
        with self.assertRaises(ValueError):
            runner.replace_entry(raw, "absent.cpp", "Matching")

    def test_original_entry_not_mutated(self):
        original = {"status": "NonMatching", "extra_cflags": ["-O4"]}
        result = runner.matching_entry(original, ["-ipa off"])
        self.assertEqual(original["extra_cflags"], ["-O4"])
        self.assertEqual(result["extra_cflags"], ["-O4", "-ipa off"])

    def test_quota_stops_at_smallest_remaining_window(self):
        self.assertEqual(runner.remaining_usage({"rateLimits": {"primary": {"usedPercent": 99}, "secondary": {"usedPercent": 14}}}), 1)
        self.assertEqual(runner.remaining_usage({"remaining_percent": 0}), 0)
        with self.assertRaises(ValueError):
            runner.remaining_usage({})

    def test_checksum_failure_is_diagnosed_even_after_build_failure(self):
        self.assertEqual(runner.failure_summary("main.dol: FAILED\ncomputed checksum wrong")["kind"], "executable_mismatch")
        self.assertEqual(runner.failure_summary("multiply-defined: 'pool'")["symbols"], ["pool"])

    def test_relocations_and_unpaired_helpers_not_declared_exact(self):
        data = {"left": {"sections": [{"symbols": [{"symbol": {"name": "call", "size": "4"}, "match_percent": 100,
            "instructions": [{"diff_kind": "DIFF_ARG_MISMATCH", "instruction": {"relocation": {"target": {"symbol": {"name": "alias"}}}}}]}]}]},
            "right": {"sections": [{"symbols": [{"symbol": {"name": "extra", "size": "4"}, "instructions": [{}]}]}]}}
        result = runner.summarize_diff(data)
        self.assertFalse(result[0]["exact_comparison"])
        self.assertEqual(result[0]["target_relocations"], ["alias"])
        self.assertTrue(result[1]["unpaired"])

    def test_failed_or_timed_out_restore_preserves_dol(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            backup, output = root / "backup", root / "output"
            backup.write_bytes(b"verified executable")
            instance = runner.Runner.__new__(runner.Runner)
            instance.output = output
            instance.expected = runner.sha1(backup)
            def timeout(label):
                output.write_bytes(b"broken")
                raise subprocess.TimeoutExpired("compiler", 1)
            instance.build = timeout
            with self.assertRaises(subprocess.TimeoutExpired):
                instance.restore_build(backup, "restore")
            self.assertEqual(output.read_bytes(), backup.read_bytes())

    def test_recovery_refuses_external_configuration_edit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            instance = runner.Runner.__new__(runner.Runner)
            instance.config, instance.journal = root / "objects.json", root / "pending.json"
            instance.config.write_bytes(b"external edit")
            runner.write_json(instance.journal, {"trial_config_sha256": "trial", "original_config_sha256": "original"})
            with self.assertRaises(RuntimeError):
                instance.recover()
            self.assertEqual(instance.config.read_bytes(), b"external edit")

    def test_lock_excludes_second_runner_then_releases(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "lock"
            with runner.exclusive_lock(path):
                with self.assertRaises(OSError):
                    with runner.exclusive_lock(path):
                        pass
            with runner.exclusive_lock(path):
                pass

    def test_journal_recovery_rebuilds_original_configuration(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            instance = runner.Runner.__new__(runner.Runner)
            instance.config, instance.journal = root / "objects.json", root / "pending.json"
            backup, dol = root / "backup.json", root / "verified.dol"
            backup.write_bytes(b'{"original": true}')
            dol.write_bytes(b"verified")
            instance.expected = runner.sha1(dol)
            instance.config.write_bytes(b'{"trial": true}')
            runner.write_json(instance.journal, {"trial_config_sha256": runner.digest(instance.config),
                "original_config_sha256": runner.digest(backup), "config_backup": str(backup), "dol_backup": str(dol)})
            observed = []
            instance.build = lambda label: (observed.append(instance.config.read_bytes()) or True, root / "build.log")
            instance.refresh = lambda: observed.append(b"refreshed")
            instance.recover()
            self.assertEqual(observed, [backup.read_bytes(), b"refreshed"])
            self.assertFalse(instance.journal.exists())

    def test_corrupt_recovery_backups_do_not_mutate_any_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            instance = runner.Runner.__new__(runner.Runner)
            instance.config, instance.journal = root/'config', root/'journal'
            config_backup, dol_backup, source_backup, source = (root/n for n in ['config-backup', 'dol-backup', 'source-backup', 'source'])
            instance.config.write_bytes(b'trial config')
            config_backup.write_bytes(b'original config')
            dol_backup.write_bytes(b'verified executable')
            source_backup.write_bytes(b'original source')
            source.write_bytes(b'trial source')
            instance.expected = runner.sha1(dol_backup)
            pending = {'trial_config_sha256': runner.digest(instance.config),
                'original_config_sha256': runner.digest(config_backup), 'config_backup': str(config_backup),
                'dol_backup': str(dol_backup), 'sources': [{'path':str(source), 'backup':str(source_backup),
                'original_sha256':runner.digest(source_backup), 'allowed_sha256':[runner.digest(source_backup),runner.digest(source)]}]}
            runner.write_json(instance.journal, pending)
            for backup in (config_backup, dol_backup, source_backup):
                saved = backup.read_bytes()
                backup.write_bytes(b'corrupt')
                with self.assertRaises(RuntimeError):
                    instance.recover()
                self.assertEqual(instance.config.read_bytes(), b'trial config')
                self.assertEqual(source.read_bytes(), b'trial source')
                self.assertTrue(instance.journal.exists())
                backup.write_bytes(saved)

    def test_corrupt_dol_fallback_is_rejected_before_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            backup = root/'backup'
            backup.write_bytes(b'corrupt')
            instance = runner.Runner.__new__(runner.Runner)
            instance.expected = 'expected retail hash'
            instance.build = lambda label: self.fail('Build must not start with a corrupt fallback')
            with self.assertRaises(RuntimeError):
                instance.restore_build(backup, 'restore')

    def test_declared_library_version_has_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "version.h").write_text('// header\n#define VERSION "1.2.1"\n')
            evidence = audit.version_evidence(root, "version.h", r'VERSION\s+"([^"]+)"')
            self.assertEqual(evidence["value"], "1.2.1")
            self.assertEqual(evidence["line"], 2)
            self.assertEqual(len(evidence["sha256"]), 64)

    def test_reference_comparison_distinguishes_line_endings_from_changes(self):
        with tempfile.TemporaryDirectory() as tmp:
            a, b = Path(tmp) / "a", Path(tmp) / "b"
            a.mkdir(); b.mkdir()
            (a / "same.c").write_bytes(b"x\r\n")
            (b / "same.c").write_bytes(b"x\n")
            (a / "change.c").write_bytes(b"one")
            (b / "change.c").write_bytes(b"two")
            (a / "new.c").write_bytes(b"new")
            result = audit.compare_trees(a, b)
            self.assertEqual(result["counts"]["modified"], 1)
            self.assertEqual(result["counts"]["line_endings_only"], 1)
            self.assertEqual(result["only_bundled"], ["new.c"])


if __name__ == "__main__":
    unittest.main()
