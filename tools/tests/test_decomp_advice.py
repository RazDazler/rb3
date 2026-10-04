import sys
from pathlib import Path
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).parents[1]))
import decomp_advice as advice


class AdviceTests(unittest.TestCase):
    def test_citations_must_refer_to_original_instructions(self):
        rows = [{'row': 0, 'original': 'blr'}, {'row': 1, 'original': None}]
        valid = {'observations': [{'rows': [0], 'claim': 'Returns via link register.'}], 'questions': []}
        self.assertEqual(advice.validated_notes(valid, rows), valid)
        for citations in ([1], [2], [True], [], ['0']):
            bad = {'observations': [{'rows': citations, 'claim': 'Unsupported'}], 'questions': []}
            with self.assertRaises(ValueError):
                advice.validated_notes(bad, rows)

    def test_unexpected_code_fields_rejected(self):
        with self.assertRaises(ValueError):
            advice.validated_notes({'body': '{}', 'observations': [], 'questions': []}, [])

    def test_endpoint_cannot_send_source_to_remote_or_embedded_credentials(self):
        for base in ('https://example.com', 'http://example.com', 'http://user:pass@localhost',
                     'http://localhost/remote', 'http://localhost?redirect=remote'):
            with patch.object(advice, 'build_opener') as opener:
                with self.assertRaises(ValueError):
                    advice.api(base, '/api/tags')
                opener.assert_not_called()


if __name__ == '__main__':
    unittest.main()
