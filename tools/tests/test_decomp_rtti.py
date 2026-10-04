"""Exercise RTTI pairing with real ELF32 tables, not compiler-specific labels."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).parents[1]))
from decomp_rtti import Elf32, rtti_mappings, comparison_mappings


def fixture(path, label, class_name=b'Renderer\0', chain_size=12, section_target=False,
            owner='__RTTI__8Renderer', base_offset=0):
    strings = b'\0' + owner.encode() + b'\0' + label.encode() + b'\0' + (label + '1').encode() + b'\0'
    name_index = len(owner) + 2
    chain_index = name_index + len(label) + 1
    base_index = len(strings)
    strings += b'__RTTI__6ObjRef\0'
    payload = b'\0' * 8 + class_name
    payload += b'\0' * (-len(payload) % 4)
    chain_offset = len(payload)
    payload += b'\0' * chain_size
    if chain_size >= 12:
        payload = bytearray(payload)
        struct.pack_into('>I', payload, chain_offset + 4, base_offset)
    symbols = b'\0' * 16
    symbols += struct.pack('>IIIBBH', 1, 0, 8, 17, 0, 1)
    symbols += struct.pack('>IIIBBH', name_index, 8, len(class_name), 1, 0, 1)
    symbols += struct.pack('>IIIBBH', chain_index, chain_offset, chain_size, 1, 0, 1)
    symbols += struct.pack('>IIIBBH', 0, 0, 0, 3, 0, 1)
    symbols += struct.pack('>IIIBBH', base_index, 0, 0, 16, 0, 0)
    if section_target:
        relocations = struct.pack('>IIiIIi', 0, (4 << 8) | 1, 8,
                                  4, (4 << 8) | 1, chain_offset)
    else:
        relocations = struct.pack('>IIiIIi', 0, (2 << 8) | 1, 0,
                                  4, (3 << 8) | 1, 0)
    for offset in range(0, chain_size - 4, 8):
        relocations += struct.pack('>IIi', chain_offset + offset, (5 << 8) | 1, 0)
    contents = [payload, strings, symbols, relocations]
    raw = bytearray(52)
    raw[:6] = b'\x7fELF\x01\x02'
    struct.pack_into('>HH', raw, 16, 1, 20)
    sections = [(0,) * 10]
    for index, content in enumerate(contents, 1):
        offset = len(raw)
        raw.extend(content)
        kind = {1: 1, 2: 3, 3: 2, 4: 4}[index]
        link = 2 if index == 3 else 3 if index == 4 else 0
        info = 1 if index == 4 else 0
        entry = 16 if index == 3 else 12 if index == 4 else 0
        sections.append((0, kind, 0, 0, offset, len(content), link, info, 4, entry))
    table_offset = len(raw)
    for section in sections:
        raw.extend(struct.pack('>10I', *section))
    struct.pack_into('>I', raw, 32, table_offset)
    struct.pack_into('>HH', raw, 46, 40, len(sections))
    path.write_bytes(raw)


class RttiTests(unittest.TestCase):
    def test_merged_generated_constant_is_paired_by_complete_unique_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '@STRING@SetType__8RendererF6Symbol@0',
                    class_name=b'types\0', owner='__RTTI__Left')
            fixture(b, '@STRING@SetType__6ObjectF6Symbol@0',
                    class_name=b'types\0', owner='__RTTI__Right')
            self.assertEqual(comparison_mappings(a, b), {
                '@STRING@SetType__8RendererF6Symbol@0':
                '@STRING@SetType__6ObjectF6Symbol@0'})

    def test_generated_constant_rejects_ambiguous_and_changed_strings(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '__FUNCTION__$1', owner='__RTTI__Left')
            fixture(b, '@900', owner='__RTTI__Right')
            self.assertEqual(comparison_mappings(a, b), {'__FUNCTION__$1': '@900'})
            left, right = Elf32(a), Elf32(b)
            duplicate = dict(next(s for s in right.symbols if s['name'] == '@900'))
            duplicate['name'] = '@901'
            right.symbols.append(duplicate)
            with patch('decomp_rtti.Elf32', side_effect=[left, right, left, right]):
                self.assertEqual(comparison_mappings(a, b), {})
            fixture(b, '@900', class_name=b'Other\0', owner='__RTTI__Right')
            self.assertEqual(comparison_mappings(a, b), {})

    def test_generated_constant_rejects_relocations_and_embedded_zeroes(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '__FUNCTION__$1', owner='__RTTI__Left')
            fixture(b, '@900', owner='__RTTI__Right')
            left, right = Elf32(a), Elf32(b)
            left.relocations[(1, 8)] = (1, right.symbols[1], 0)
            with patch('decomp_rtti.Elf32', side_effect=[left, right, left, right]):
                self.assertEqual(comparison_mappings(a, b), {})
            fixture(a, '__FUNCTION__$1', class_name=b'A\0B\0', owner='__RTTI__Left')
            fixture(b, '@900', class_name=b'A\0B\0', owner='__RTTI__Right')
            self.assertEqual(comparison_mappings(a, b), {})

    def test_orphan_chain_requires_unique_exact_relocations_and_offsets(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '@100', owner='__RTTI__Left')
            fixture(b, '@900', owner='__RTTI__Right')
            self.assertEqual(rtti_mappings(a, b), {'@1001': '@9001'})
            left, right = Elf32(a), Elf32(b)
            duplicate = dict(next(s for s in right.symbols if s['name'] == '@9001'))
            duplicate['name'] = '@9002'
            right.symbols.append(duplicate)
            with patch('decomp_rtti.Elf32', side_effect=[left, right]):
                self.assertEqual(rtti_mappings(a, b), {})
            fixture(b, '@900', owner='__RTTI__Right', base_offset=32)
            self.assertEqual(rtti_mappings(a, b), {})

    def test_uninitialized_data_cannot_be_an_orphan_chain(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'a.o'
            fixture(path, '@100')
            elf = Elf32(path)
            symbol = next(s for s in elf.symbols if s['name'] == '@1001')
            section = list(elf.sections[symbol['section']])
            section[1] = 8
            elf.sections[symbol['section']] = tuple(section)
            self.assertIsNone(elf.chain_signature(symbol))

    def test_labels_and_section_addends_pair_by_named_owner(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '@100')
            fixture(b, '@900', section_target=True)
            self.assertEqual(rtti_mappings(a, b), {'@100': '@900', '@1001': '@9001'})

    def test_different_chain_sizes_are_compared_instead_of_declared_equal(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '@100', chain_size=20)
            fixture(b, '@900', chain_size=12)
            self.assertEqual(rtti_mappings(a, b)['@1001'], '@9001')
            left, right = Elf32(a), Elf32(b)
            x = next(s for s in left.symbols if s['name'] == '@1001')
            y = next(s for s in right.symbols if s['name'] == '@9001')
            self.assertNotEqual(left.payload(x), right.payload(y))

    def test_changed_class_name_is_paired_for_real_data_comparison(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '@100')
            fixture(b, '@900', class_name=b'Other\0')
            self.assertEqual(rtti_mappings(a, b), {'@100': '@900', '@1001': '@9001'})
            left, right = Elf32(a), Elf32(b)
            x = next(s for s in left.symbols if s['name'] == '@100')
            y = next(s for s in right.symbols if s['name'] == '@900')
            self.assertNotEqual(left.payload(x), right.payload(y))

    def test_non_chain_payload_and_truncated_section_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            a, b = Path(directory) / 'a.o', Path(directory) / 'b.o'
            fixture(a, '@100')
            fixture(b, '@900', chain_size=16)
            self.assertEqual(rtti_mappings(a, b), {'@100': '@900'})
            b.write_bytes(b.read_bytes()[:-1])
            with self.assertRaises(ValueError):
                rtti_mappings(a, b)


if __name__ == '__main__':
    unittest.main()
