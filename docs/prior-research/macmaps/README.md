# Research snapshot from poolrad-macmaps

These eleven documents and two Java reference classes were copied unchanged on
2026-10-10 from `../poolrad-macmaps/` at Git revision
`c56c0695ded8b31e5f847f3a2007fa37ba9143e3`. The selected source files had
no working-tree changes. [`SHA256SUMS`](SHA256SUMS) records the exact copies.
Their relative links may refer to documents or tools still located in the
original sibling repository.

| Documents | Why they are here |
| --- | --- |
| [RESEARCH.md](RESEARCH.md), [AREA_IDENTITY.md](AREA_IDENTITY.md), [MAP_MEMORY.md](MAP_MEMORY.md), [AREA_NAMES.md](AREA_NAMES.md) | DAX/GEO layout, map planes and IDs, area transitions, and evidence connecting areas to event scripts. |
| [PARTY.md](PARTY.md), [PARTY_CONDITIONS.md](PARTY_CONDITIONS.md), [COMBAT_MEMORY.md](COMBAT_MEMORY.md), [EQUIPMENT_REFERENCE.md](EQUIPMENT_REFERENCE.md) | Character, status, combat, and equipment behavior to investigate in the DOS build. |
| [SAVE_FORMAT.md](SAVE_FORMAT.md), [MESSAGE_MEMORY.md](MESSAGE_MEMORY.md), [WILDERNESS_MEMORY.md](WILDERNESS_MEMORY.md) | Save-state, text/event, and wilderness leads. |
| [DaxReader.java](code/DaxReader.java), [GeoMap.java](code/GeoMap.java) | Checked DAX/RLE reader and 16-by-16 GEO accessors, as translation references for C. These classes do not implement DOS game logic. |

The source project studied **Macintosh Pool of Radiance v1.1** inside a 68k
emulator. This repository targets the supplied **DOS v1.3** build. The DAX
container and map research provide starting hypotheses, but DOS records and
behavior must be checked independently. Macintosh A5 offsets, heap handles,
resource-fork offsets, window code, and save layout are not DOS addresses or
portable structures.

No ROM, game binary, extracted asset, RAM capture, screenshot, or save was
copied. `DaxReader.java` credits Gold Box Explorer for the adapted DAX/GEO
format reader; the copied [MIT notice](licenses/GoldBoxExplorer-MIT.txt) must
be retained with any future code adapted from it.
