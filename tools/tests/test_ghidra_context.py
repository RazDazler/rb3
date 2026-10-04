"""B8 exports must use exact named function addresses, never retail/data labels."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).parents[1]))
from ghidra_context import symbol_addresses, requests


class ContextTests(unittest.TestCase):
    def test_only_function_rows_are_exported(self):
        addresses = symbol_addresses('foo__Fv = .text:0x80001234; // type:function size:0x20\n'
                                     'data = .data:0x80005678; // type:object size:0x4\n')
        self.assertEqual(addresses, {'foo__Fv': '0x80001234'})
        self.assertEqual(requests(['foo__Fv', 'foo__Fv'], addresses), '0x80001234\tfoo__Fv\n')

    def test_missing_or_injected_name_is_rejected(self):
        with self.assertRaises(ValueError):
            requests(['foo__Fv\n0x80004000\tmemcpy'], {'foo__Fv': '0x80001234'})


if __name__ == '__main__':
    unittest.main()
