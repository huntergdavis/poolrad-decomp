# U6F +0010 as a whole function: the roster text-file reader (Claude C1, PM GO i8483)

**Scope.** The complete executable span of GAME.OVR unit 0x6F + 0x0010 translated into
portable C at the level of its Pascal semantics, with every external call kept as an
explicit boundary. Inputs: `GAME.OVR` 206,105 bytes, sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; `START.EXE` 65,231
bytes, sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit 0x6F
starts at GAME.OVR offset 0x1DFE6 (9,189 code bytes); the function is public entry 1
(run-time stub `0x7C:0x25`). Ownership: assigned by the PM in inbox i8483 as this lane's
new primary after the +01B8 writer ([U6F_01B8_WRITE_LIST.md](U6F_01B8_WRITE_LIST.md)); no
other C of this span exists in the tree (searched for `0x7C:0x25`, `+0x0010` and the
unit-relative names under `docs`, `include`, `src`, `tests`, `tools`). The `deja` recall
hung and timed out (the index is damaged per i8257); the merged notes cited below are the
prior evidence. C: `src/recovered/overlay_u6f_0010_read_list.c`,
`include/poolrad/recovered/overlay_u6f_0010_read_list.h`,
`tests/recovered/test_overlay_u6f_0010_read_list.c`, built by
`tests/recovered/build_u6f_0010.sh` (no shared CMake edit without PM coordination).

## 1. Byte identity, independently derived

| item | value |
| --- | --- |
| executable span | `[0010,0197)`, GAME.OVR `[1DFF6,1E17D)`, 391 bytes, 147 instruction starts, sha256 `3dd22f1389f59ad3481634a8cb455521b7aa619a991fe1fb8b32d71171d7cb36` |
| outer return | `retf 4` at +0194 (one far argument, the var list head, at `[bp+6..8]`) |
| following bytes | `[0197,01B8)`, 33 bytes: the three private literals of the +01B8 writer with their length bytes (12 + 17 + 1 characters), excluded (sha256 `fb3e54559f7567e2ffde8d129525c2c85eddbede8c2eca93d5fb7bc39bab70c9`) |
| preceding bytes | `[0000,0010)`: the two private literals this function reads, CS:0x0000 (12 characters) and CS:0x000D (2 characters), with their length bytes; data, not code |
| frame | `sub sp, 0x20C`: Text record at `SS:BP-0x10C` (256 bytes), temporary string `SS:BP-0x20C`, current node `SS:BP-4`, tail `SS:BP-8`, party cursor `SS:BP-0xC` |
| calls | 18: one near (+0024 to +02AE), one resident far (`0x802:0x02EB`), sixteen SYSTEM far (`0xAF8:0x0B0B` x3, `0x0B98` x3, `0x04F7`, `0x0567`, `0x0A04`, `0x023F`, `0x16A8`, `0x08E1`, `0x081C`, `0x0C10`, `0x0B25`, `0x05C1`) |
| branches | +0048 `jne +004D` / +004A `jmp +0191`; +008A `je +008F` / +008C `jmp +0186`; +00B9 `jne +00DD`; +00DB `jmp +00FF`; +012B `je +0153`; +013C `je +0153`; +0151 `jmp +0125`; +0159 `je +0183`; +0183 `jmp +007D` |
| caller | unit 0x6C + 0x306D only (far, site +308A, fixup-confirmed): it passes `SS:BP-4` of its own frame and tests that head for nil after the return (+308F..+3097) |

Source and ownership map of what the span touches:

| item | where it is established | owner |
| --- | --- | --- |
| +02AE, the directory-ensuring routine | [U6F_POSTINSERT_02AE.md](U6F_POSTINSERT_02AE.md), this lane's pending +02AE boundary note (branch `claude/u6f-02ae`, head 8915d63, unpushed) records this call site, +0024, among its six | merged behavioral C `pr_u6f_postinsert_02ae` (Codex); boundary here |
| resident `0x802:0x02EB`, the file probe returning a Boolean in AL | [RESIDENT_DAX_SELECTOR_02EB.md](RESIDENT_DAX_SELECTOR_02EB.md): copies at most 0x28 characters of the name, Assign/Reset/Close, IOResult, AL = 1 only on a zero IOResult and a non-empty name | bounded model `pr_rec_res_0802_02eb` (Codex); boundary here |
| `DS:0x537A`, the live path string | [U6F_01B8_WRITE_LIST.md](U6F_01B8_WRITE_LIST.md) (the writer builds the same path) | run-time data, caller-supplied here |
| `DS:0x5D96/0x5D98`, the party list head, and the record link at `+0x104/+0x106` | [RESIDENT_PARTY_COUNT.md](RESIDENT_PARTY_COUNT.md), [DOS_CHARACTER.md](DOS_CHARACTER.md) | caller-supplied party records here |
| `0xAF8:0x0C10` string equality | PM decode in the ledger, section 1.19 (length compare, `repe cmpsb`, length compare) | translated inline |
| `0xAF8:0x0B25` bounded store, `0x0B0B`, `0x0B98`, `0x16A8` | the same PM decode and [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) | translated inline |
| `0xAF8:0x0567` (input open, mode 0xD7B1), `0x0A04` (Eof: AL = 1 at 0x1A or buffer end), `0x08E1` (read at most `count` characters, stop at CR or 0x1A, store the length byte), `0x081C` (skip through CR and an optional LF), `0x04F7`, `0x023F`, `0x05C1` | decoded read-only for this note from the expanded resident image (image 0x0B4E7, 0x0B984, 0x0B861, 0x0B79C) | boundaries |

## 2. Semantics

```
procedure ReadList(var head: far pointer);              { entry 1, retf 4 }
  head := nil;                                          { +0017 }
  EnsureDirectory;                                      { +0023: near +02AE }
  temp := DS:0x537A + CS:0x0000;                        { +0027: live path + 12-character constant }
  if not Probe(temp) then exit;                         { +0041: 0x802:0x02EB, AL = 0 -> +0191, no Close }
  Assign(f, DS:0x537A + CS:0x0000); Reset(f);           { +004D..+0078: 0x04F7, 0x0567 input mode }
  while not Eof(f) do begin                             { +007D: 0x0A04 }
    node := GetMem(0x2E); FillChar(node^, 0x2E, 0);     { +008F..+00AA }
    if head = nil then begin head := node; tail := head end
    else begin tail^.next := node; tail := tail^.next end;   { +00AF..+00FC: next at +0x2A }
    ReadLn(f, tail^.name);                              { +00FF: 0x08E1 with count 0x28, then 0x081C }
    p := PartyHead;                                     { +0118: DS:0x5D96/0x5D98 }
    while (p <> nil) and (p^.name <> tail^.name) do     { +0125..+0151: 0x0C10; link at +0x104 }
      p := p^.link;
    if p <> nil then                                    { +0153 }
      tail^.name := Copy(CS:0x000D + tail^.name, 1, 0x28);   { +015B..+017E: 0x0B0B, 0x0B98, 0x0B25 }
  end;                                                  { +0183: jmp +007D }
  Close(f);                                             { +0186: 0x05C1 }
```

Facts the C preserves: the head is cleared before the directory routine runs, so a
caller sees nil on every early exit; the probe's AL = 0 leaves the file unopened and
unclosed; the path is composed twice from the same two strings (the probe does not change
them), so the model composes it once and passes it to both; every line, including an empty
one, makes a node, and the node is zero-filled before it is linked and read; the read keeps
at most 0x28 characters of a longer line; the party compare is exact (lengths and bytes);
the first party record whose name equals the line stops the walk; the marked copy keeps the
first 0x28 characters of marker + name, so a 39- or 40-character name loses its last
characters; the read loop and the party walk have no bound in the engine (a cycle in the
party list never terminates); GetMem's result is never tested; nothing is written to
`DS:0x537A`. The writer (+01B8) later skips the nodes whose first character is the marker's
first byte, which is why the party's own members do not come back into the roster file.

## 3. Boundaries, kept explicit

| source call | identity | C boundary |
| --- | --- | --- |
| +0024 near +02AE | the directory-ensuring routine (129 bytes, 51 starts) | `ensure_directory` |
| 0x802:0x02EB | temporary-file probe, Boolean in AL, `retf 4` | `select_file(path, &al)` |
| 0x04F7 | text-record initialization, `retf 8` | `text_assign(path)` |
| 0x0567 | shared open path, mode 0xD7B1 (input), `retf 4` | `text_reset()` |
| 0x0A04 | Eof on the buffered text record | `eof(&al)` |
| 0x023F | GetMem(0x2E) | `alloc_node(&node)`; the model zero-fills the node (0x16A8) |
| 0x08E1 + 0x081C | read at most `count` (0x28) characters, store the length byte, then end the line | `read_line(chars, 0x28, &count)`; the model stores the length byte |
| 0x0B0B, 0x0B98, 0x0B25, 0x0C10, 0x16A8 | string load, concatenate, bounded store, equality, fill | translated inline (Pascal strings capped at 255; the store capped at 0x28) |
| 0x05C1 | shared close/flush, `retf 4` | `text_close()` |

Host spans and contracts: the live path span must hold the whole string (length + 1
bytes; this function never reads the raw byte after the length, unlike the writer); each
party record carries its supplied name span, checked before the compare (at least
length + 1 bytes) at that record, after the node for the line was already allocated,
linked and read; a read service that claims more than 0x28 characters or a NULL node from
the allocator is reported as `ARGUMENT` at the offset reached. The engine rereads `head^`
at +00AF for every line and trusts its own tail for a non-nil head; the model keeps the
head it last published and, at that reread, reports `ARGUMENT` (offset 0x00AF) when any
callback changed `*head` before or after the first link, so no node is ever appended
through a tail that no longer belongs to the published list (PM review i8549). A head
changed by a callback before an empty file's first Eof is not detected, because the engine
never rereads it on that path. Host guards (`max_nodes`, `max_party_steps`) bound the two engine
loops and report `LINE_LIMIT` or `PARTY_LIMIT`, statuses the engine cannot produce. A
missing callback at a reached boundary reports `NEED_HOOK` with the call instruction's
source offset in `next_offset`; a callback failure reports `HOST_FAILURE` with every
earlier effect kept, and the list built so far stays reachable from the head, as the
engine's heap would. The private CS literals are caller-supplied with their exact original
lengths (12 and 2); their bytes are not in the public tree. The C node is laid out as the
engine's (name at +0, next at +0x2A) but its size differs with the host pointer width.

## 4. Classification

Whole-function **path-complete model**: all 147 instruction starts are covered at the
semantics level and all eleven branches are represented; the 18 calls are boundaries or
inline string operations. It is not a strict full translation (the text-file services, the
allocator, the probe and +02AE are callbacks) and no native run was made (host heavy-native
hold). No strict credit is claimed.

## 5. Tests

Synthetic callbacks record the call order and serve lines from a fixed table; the node pool
is pre-filled with 0xCC so the zero-fill is observable. Probe false: head cleared, order
`E S`, nothing opened or closed, the probe received the composed path; empty file: order
`E S A R F C`, the Assign path equals the probe path; four lines with a two-record party:
order `E S A R (F G L)x4 F C`, the links in order, the second line marked with the
marker, the empty line making an empty node, a case-different line not matched, eight
party steps; a 43-character line read as 40 and a 40-character match marked and cut to 40,
an empty party name matching an empty line; each of the eight boundaries missing, in reach
order, with the stopped offset; close, probe, allocator and read failures keeping the
earlier effects and the reachable list; a NULL node and an over-long read refused; a file
that never ends hitting the line guard; a cyclic party hitting the party guard; path
overflow past 255 before the probe; argument checks (literal lengths, live-string span,
null hooks, null head, null output, no effects); a zero-length live path accepted; party
record spans refused at the record after the line's node stands, with an earlier line's
marking kept; hostile callbacks moving the head to another node or to nil after the first
linked line, and to another node before the first link, refused at the reread (+00AF) with
the new node allocated but not linked and the first node intact, plus the undetected
empty-file case recorded as such. Builds: strict (`-std=c11 -Wall -Wextra -Wpedantic -Werror`), the same under
`-fsanitize=address,undefined`, and `gcc -fanalyzer` on the model and the test (clean;
one finding during development, a tail link after a host-moved head, became the explicit
reread check above). `cppcheck` and `clang-tidy` are not installed on this host.

## 6. Proposed registration (for PM, not applied)

```cmake
# poolrad_recovered sources: + src/recovered/overlay_u6f_0010_read_list.c
add_executable(poolrad_overlay_u6f_0010_read_list_tests tests/recovered/test_overlay_u6f_0010_read_list.c)
target_link_libraries(poolrad_overlay_u6f_0010_read_list_tests PRIVATE poolrad_recovered)
add_test(NAME poolrad_overlay_u6f_0010_read_list_tests COMMAND poolrad_overlay_u6f_0010_read_list_tests)
```

## 7. Queue

Follow-ons in the assigned order (i8483): +22D5 (entry 11, the area-transition save
prompt; its documentation in ECL_CONTRACTS.md section 6 and the ledger section 4 calls it
the ECL loader and is to be corrected with it) and +1446 (entry 10, cold entry, local-only).
Neither is started.
