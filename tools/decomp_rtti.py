"""Pair anonymous CodeWarrior RTTI data through named RTTI relocations.

Only comparison symbol mappings are returned. Object bytes, relocations,
linking status and executable validation are never changed.
"""
from pathlib import Path
import re
import struct


class Elf32:
    def __init__(self, path):
        self.raw = Path(path).read_bytes()
        data = self.raw
        if len(data) < 52 or data[:6] != b'\x7fELF\x01\x02':
            raise ValueError('Expected ELF32 big-endian object')
        if struct.unpack_from('>H', data, 18)[0] != 20:
            raise ValueError('Expected PowerPC object')
        offset = struct.unpack_from('>I', data, 32)[0]
        stride, count = struct.unpack_from('>HH', data, 46)
        if stride != 40 or not count or offset + stride * count > len(data):
            raise ValueError('Invalid section table')
        self.sections = [struct.unpack_from('>10I', data, offset + stride * i)
                         for i in range(count)]
        self.tables = {}
        for index, section in enumerate(self.sections):
            if section[1] != 2:
                continue
            if section[9] != 16 or section[5] % 16:
                raise ValueError('Invalid symbol table')
            strings = self.section_data(section[6])
            symbols = []
            for at in range(0, section[5], 16):
                name, value, size, info, other, owner = struct.unpack_from(
                    '>IIIBBH', self.section_data(index), at)
                if name >= len(strings):
                    raise ValueError('Invalid symbol name')
                end = strings.find(b'\0', name)
                if end < 0:
                    raise ValueError('Unterminated symbol name')
                symbols.append({'name': strings[name:end].decode('utf-8'),
                                'value': value, 'size': size,
                                'type': info & 15, 'section': owner})
            self.tables[index] = symbols
        self.symbols = [s for table in self.tables.values() for s in table]
        self.relocations = {}
        for index, section in enumerate(self.sections):
            if section[1] != 4:
                continue
            if section[9] != 12 or section[5] % 12:
                raise ValueError('Invalid RELA table')
            table = self.tables.get(section[6])
            if table is None:
                raise ValueError('Missing relocation symbol table')
            for at in range(0, section[5], 12):
                address, info, addend = struct.unpack_from(
                    '>IIi', self.section_data(index), at)
                symbol_index, kind = info >> 8, info & 255
                if symbol_index >= len(table):
                    raise ValueError('Invalid relocation target')
                key = (section[7], address)
                if key in self.relocations:
                    raise ValueError('Ambiguous relocation')
                self.relocations[key] = (kind, table[symbol_index], addend)

    def section_data(self, index):
        if not 0 <= index < len(self.sections):
            raise ValueError('Invalid section index')
        section = self.sections[index]
        offset, size = section[4:6]
        if section[1] == 8 or offset + size > len(self.raw):
            raise ValueError('Section data unavailable')
        return self.raw[offset:offset + size]

    def payload(self, symbol):
        data = self.section_data(symbol['section'])
        offset, size = symbol['value'], symbol['size']
        if offset + size > len(data):
            raise ValueError('Symbol outside section')
        return data[offset:offset + size]

    def pointed_symbol(self, anchor, offset):
        relocation = self.relocations.get((anchor['section'], anchor['value'] + offset))
        if not relocation or relocation[0] != 1:  # R_PPC_ADDR32
            return None
        _, target, addend = relocation
        address = target['value'] + addend
        matches = [s for s in self.symbols if s['section'] == target['section']
                   and s['value'] == address and s['size'] and s['type'] != 3]
        return matches[0] if len(matches) == 1 else None

    def chain_signature(self, symbol):
        section = self.sections[symbol['section']]
        if symbol['size'] < 12 or symbol['size'] % 8 != 4 or section[1] != 1 or section[2] & 4:
            return None
        payload = self.payload(symbol)
        if payload[-4:] != b'\0' * 4:
            return None
        pairs = []
        for offset in range(0, len(payload) - 4, 8):
            relocation = self.relocations.get((symbol['section'], symbol['value'] + offset))
            if not relocation or relocation[0] != 1 or not relocation[1]['name'].startswith('__RTTI__'):
                return None
            pairs.append((relocation[1]['name'], relocation[2],
                          struct.unpack_from('>I', payload, offset + 4)[0]))
        return tuple(pairs) if pairs else None


def rtti_mappings(left, right):
    """Map each named RTTI's anonymous class name and base-class table."""
    a, b = Elf32(left), Elf32(right)
    mappings = {}
    right_names = {}
    for symbol in b.symbols:
        right_names.setdefault(symbol['name'], []).append(symbol)
    for anchor in a.symbols:
        peers = right_names.get(anchor['name'], [])
        if (not anchor['name'].startswith('__RTTI__') or anchor['size'] != 8 or len(peers) != 1
                or a.sections[anchor['section']][1] != 1):
            continue
        peer = peers[0]
        if peer['size'] != 8 or b.sections[peer['section']][1] != 1:
            continue
        names = a.pointed_symbol(anchor, 0), b.pointed_symbol(peer, 0)
        if not all(names):
            continue
        original_name, compiled_name = names
        # The named owner establishes identity. Differing string bytes must
        # still be compared, rather than hidden by a coincident offset pairing.
        if not all(elf.payload(s).endswith(b'\0') for elf, s in
                   ((a, original_name), (b, compiled_name))):
            continue
        for offset in (0, 4):
            x, y = a.pointed_symbol(anchor, offset), b.pointed_symbol(peer, offset)
            if not x or not y or not all(re.fullmatch(r'@\d+', s['name']) for s in (x, y)):
                continue
            if offset == 4:
                # Base chains have pairs of pointers/offsets and a zero terminator.
                if any(s['size'] % 8 != 4 or elf.payload(s)[-4:] != b'\0' * 4
                       for elf, s in ((a, x), (b, y))):
                    continue
            previous = mappings.get(x['name'])
            if previous is not None and previous != y['name']:
                raise ValueError('Conflicting RTTI mapping')
            mappings[x['name']] = y['name']
    # DTK sometimes discards a weak RTTI owner but retains its anonymous base
    # chain. Pair only a unique exact relocation/offset signature in that case.
    right_chains = {}
    for symbol in b.symbols:
        if re.fullmatch(r'@\d+', symbol['name']) and symbol['size']:
            signature = b.chain_signature(symbol)
            if signature:
                right_chains.setdefault(signature, []).append(symbol)
    for symbol in a.symbols:
        if symbol['name'] in mappings or not re.fullmatch(r'@\d+', symbol['name']) or not symbol['size']:
            continue
        signature = a.chain_signature(symbol)
        peers = right_chains.get(signature, [])
        if signature and len(peers) == 1 and peers[0]['name'] not in mappings.values():
            mappings[symbol['name']] = peers[0]['name']
    if len(set(mappings.values())) != len(mappings):
        raise ValueError('RTTI mapping is not one-to-one')
    return mappings


def comparison_mappings(left, right):
    """Pair RTTI and unique compiler-generated immutable string constants.

    CodeWarrior can merge a generated SetType string with another class's
    identical constant. Offset guessing then pairs it with unrelated data.
    Only complete, unique, relocation-free C strings are eligible here.
    """
    mappings = rtti_mappings(left, right)
    a, b = Elf32(left), Elf32(right)

    def strings(elf, anonymous, excluded):
        grouped = {}
        for symbol in elf.symbols:
            name = symbol['name']
            if name in excluded:
                continue
            generated = name.startswith(('@STRING@', '__FUNCTION__$'))
            if not generated and not (anonymous and re.fullmatch(r'@\d+', name)):
                continue
            if not symbol['size'] or symbol['section'] >= len(elf.sections):
                continue
            section = elf.sections[symbol['section']]
            if section[1] != 1 or section[2] & 4:
                continue
            start, end = symbol['value'], symbol['value'] + symbol['size']
            if any(owner == symbol['section'] and start <= offset < end
                   for owner, offset in elf.relocations):
                continue
            payload = elf.payload(symbol)
            if (len(payload) < 2 or not payload.endswith(b'\0')
                    or payload.count(b'\0') != 1
                    or any(byte not in (9, 10, 13) and not 32 <= byte < 127
                           for byte in payload[:-1])):
                continue
            grouped.setdefault(payload, []).append(symbol['name'])
        return grouped

    originals = strings(a, False, mappings.keys())
    compiled = strings(b, True, mappings.values())
    for payload, names in originals.items():
        peers = compiled.get(payload, [])
        if len(names) != 1 or len(peers) != 1:
            continue
        original, peer = names[0], peers[0]
        if original in mappings or peer in mappings.values() or original == peer:
            continue
        mappings[original] = peer
    return mappings
