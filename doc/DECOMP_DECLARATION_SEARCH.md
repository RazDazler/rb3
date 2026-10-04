# Local declaration-order search

`tools/decomp_declaration_order.py` generates a finite candidate manifest from a
reviewed C++ template. It uses no model, downloads or cloud calls and does not edit
source. The normal transactional runner checks each candidate and retains only
an improvement that passes its existing guards.

The first experiment matched `VocalPart::AddPhrasePoints` after 15 candidates,
improving its strict score from 90.16393% to 100% (244 bytes). Assignment and
floating-point operation order stayed fixed; only local declarations moved.
Several other declaration-order experiments did not improve their targets.

Save a JSON spec under ignored `build/decomp` with these fields:

- `unit`: configured objdiff unit name, such as `main/band3/game/VocalPart`.
- `symbol`: exact original function symbol.
- `old`: a unique source block to replace, including its whitespace.
- `template`: the reviewed replacement block with one `{declarations}` marker.
- `declarations`: an ordered list of `{"name":"next","type":"float"}` items.

The generator supports 2-6 primitive locals and emits up to 120 candidates by
default. Preserve all reads, assignments, operation order and control flow in the
template. Do not move initialized declarations across operations that can change
their values or introduce reads before assignment. Templates require human
semantic review; compilation and matching percentages cannot establish this.

```powershell
python tools/decomp_declaration_order.py build/decomp/order-spec.json --output build/decomp/order-trials.json --limit 24
python tools/decomp_session_trial.py variants build/decomp/order-trials.json --ledger doc/DECOMP_B8_GHIDRA_SESSION_2026-10-04.json --note "Reviewed local declaration-order search"
```

Use the currently active session ledger. The native lock prevents competing
builds; the runner observes its quota/STOP files, stops at an exact target match,
and verifies the full B8 executable before retaining its best candidate. Increase
the search limit only when the disassembly gives a reason to do so. Keep generated
manifests and analysis output private in `build/decomp`.
