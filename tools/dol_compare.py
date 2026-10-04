"""Compare actual DOL section bytes by virtual address, independently of symbol names."""
from pathlib import Path
import argparse
import json
import re
import struct


def sections(data):
    if len(data) < 0x100:
        raise ValueError("Truncated DOL header")
    result = []
    for kind, count, offsets, addresses, sizes in (("text", 7, 0, 0x48, 0x90), ("data", 11, 0x1C, 0x64, 0xAC)):
        for index in range(count):
            offset, address, size = (struct.unpack_from(">I", data, base + 4 * index)[0] for base in (offsets, addresses, sizes))
            if size:
                if offset < 0x100 or offset + size > len(data):
                    raise ValueError("DOL section lies outside file")
                result.append({"name": f"{kind}{index}", "address": address, "size": size,
                               "offset": offset, "bytes": data[offset:offset + size]})
    return result


def differences(left, right, base, limit=16):
    if left == right:
        return []
    ranges, index = [], 0
    while index < max(len(left), len(right)) and len(ranges) < limit:
        if index < min(len(left), len(right)) and left[index] == right[index]:
            index += 1
            continue
        start = index
        while index < max(len(left), len(right)) and (index >= min(len(left), len(right)) or left[index] != right[index]):
            index += 1
        ranges.append({"address": f"0x{base + start:08X}", "bytes": index - start,
                       "original_hex": left[start:min(index, start + 16)].hex(),
                       "rebuilt_hex": right[start:min(index, start + 16)].hex()})
    return ranges


def split_ranges(path, source):
    text = Path(path).read_text()
    match = re.search(r"(?m)^" + re.escape(source) + r":\r?\n((?:[ \t].*\r?\n)+)", text)
    if not match:
        return []
    return [{"section": m[1], "start": int(m[2], 16), "end": int(m[3], 16)}
            for m in re.finditer(r"(\.\w+)\s+start:0x([0-9A-Fa-f]+)\s+end:0x([0-9A-Fa-f]+)", match[1])]


def compare_dols(original, rebuilt, focus=()):
    left, right = Path(original).read_bytes(), Path(rebuilt).read_bytes()
    a, b = sections(left), sections(right)
    header = lambda data: dict(zip(("bss_address", "bss_size", "entry_point"), struct.unpack_from(">III", data, 0xD8)))
    result = {"identical": left == right, "original_header": header(left), "rebuilt_header": header(right),
              "added_sections": [s["name"] for s in b if s["name"] not in {x["name"] for x in a}],
              "sections": [], "focus": []}
    for section in a:
        peer = next((s for s in b if s["name"] == section["name"]), None)
        row = {k: v for k, v in section.items() if k != "bytes"}
        if peer:
            row["rebuilt_address"], row["rebuilt_size"] = peer["address"], peer["size"]
            row["differences"] = differences(section["bytes"], peer["bytes"], section["address"], 1) if peer["address"] == section["address"] else []
            row["same_address"] = peer["address"] == section["address"]
        else:
            row["missing"] = True
        result["sections"].append(row)
    for item in focus:
        start, end = item["start"], item["end"]
        def extract(items):
            section = next((s for s in items if s["address"] <= start and end <= s["address"] + s["size"]), None)
            return section["bytes"][start - section["address"]:end - section["address"]] if section else None
        x, y = extract(a), extract(b)
        row = dict(item)
        row["differences"] = differences(x, y, start) if x is not None and y is not None else []
        row["available_in_both"] = x is not None and y is not None
        result["focus"].append(row)
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("rebuilt", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = compare_dols(args.original, args.rebuilt)
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text)
