# U6F +01B8 as a whole function: the roster text-file writer (Claude C1, handoff i8328)

**Scope.** The complete executable span of GAME.OVR unit 0x6F + 0x01B8 translated into
portable C at the level of its Pascal semantics, with every external call kept as an
explicit boundary. Inputs: `GAME.OVR` 206,105 bytes, sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; `START.EXE` 65,231
bytes, sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit 0x6F
starts at GAME.OVR offset 0x1DFE6 (9,189 code bytes); the function is public entry 2
(run-time stub `0x7C:0x2A`). Ownership: pN released U6F +01B8/+02AE/+1446 C ownership to this
lane in inbox i8328 with the exact static packet bundle
`u6f-claude-p2-handoff-i8301` (manifest sha256
`48527f1b55c85b550ef0e8ec2f72096a6ef8ffafdd2fddf810c330e0fd2cd9d0`), read read-only. The
`deja` recall for this function hung and timed out twice (the index is damaged per i8257);
the retained packets and the merged notes are the cited prior evidence. C:
`src/recovered/overlay_u6f_01b8_write_list.c`,
`include/poolrad/recovered/overlay_u6f_01b8_write_list.h`,
`tests/recovered/test_overlay_u6f_01b8_write_list.c`, built by
`tests/recovered/build_u6f_01b8.sh` (no shared CMake edit without PM coordination).

## 1. Byte identity, independently re-derived

| item | value |
| --- | --- |
| executable span | `[01B8,029A)`, GAME.OVR `[1E19E,1E280)`, 226 bytes, 84 instruction starts, sha256 `25e7c2fe579b2c381ae74378f74155e051bb94c99132d2cc2ff2799e87d9fa26` (equal to the packet's) |
| outer return | `retf 4` at +0297 (one far argument, the list head, at `[bp+6..8]`) |
| following bytes | `[029A,02AE)`, 20 bytes of UI data preceding the next prologue at +02AE, excluded (sha256 `cd1ec72ef5b6755647f8e1d7987bb952625f040b91f4dee52efe1eca9e1a9bc6`) |
| frame | `sub sp, 0x306`: Text record at `SS:BP-0x104` (256 bytes), IOResult word `SS:BP-0x106`, path string `SS:BP-0x206`, message string `SS:BP-0x306`, current node far pointer `SS:BP-4` |
| calls | 14: one near (+01C0 to +02AE), twelve SYSTEM far (`0xAF8:0x0B0B` x2, `0x0B98` x3, `0x04F7`, `0x056C`, `0x04A2`, `0x0C3B`, `0x0919`, `0x084A`, `0x05C1`), one resident far (`0x709:0x0BF5`) |
| branches | +0201 `je +0239`, +023E `jne +01C3`, +0252 `je +0289`, +025C `je +0276`, +0287 `jmp +024C` |
| callers | unit 0x6C + 0x306D (far, site +327E, the party menu's key-0x41 path) and unit 0x6F + 0x069E (near, site +0779), both fixup- or decode-confirmed; the classification row `0x6f:0x01b8` records one fixup-confirmed far caller |

The packet's figures (226 B, 84 starts, 14 calls, RETF 4, callers +0779 and U6C +327E) were
reproduced from the bytes before use.

## 2. Semantics

```
procedure WriteList(list: far pointer);                 { entry 2, retf 4 }
  EnsureDirectory;                                      { +01BF: near +02AE, no arguments }
  repeat                                                { +01C3 }
    path := DS:0x537A + CS:0x0197;                      { live path string + 12-character constant }
    Assign(f, path); Rewrite(f); ior := IOResult;       { 0x04F7, 0x056C (output mode), 0x04A2 }
    if ior <> 0 then                                    { +01FC }
      Prompt(0x0E, CS:0x01A4 + Chr(DS:0x537B) + CS:0x01B6);   { 17 + 1 + 1 characters, 0x709:0x0BF5 }
  until ior = 0;                                        { +0239: the live word is re-read }
  node := list;                                         { +0240 }
  while node <> nil do begin                            { +024C }
    if node^[1] <> #$2A then WriteLn(f, node^.string);  { +0254..+0271: 0x0919 width 0, 0x084A }
    node := node^.next;                                 { +0276..+0287: far pointer at node + 0x2A }
  end;
  Close(f);                                             { +0289: 0x05C1, also for a nil list }
```

Facts the C preserves: the message color word has AL = 0x0E (AH is whatever the caller
left; the model passes the byte); the first character of the live path (`DS:0x537B`) is the
byte after its length byte; the node test reads the byte at node+1 whatever the length byte
says; the retry loop and the list walk have no bound in the engine (a cycle never
terminates); Close runs even for a nil list; no IOResult is read after Close, so the
function's AX on return carries no promise.

## 3. Boundaries, kept explicit

| source call | identity (pN packet, hash-locked spans) | C boundary |
| --- | --- | --- |
| +01C0 near +02AE | the directory-ensuring routine (129 bytes, 51 starts; DOS function 0x39 and the resident prompt inside it); behavioral C exists as `pr_u6f_postinsert_02ae` | `ensure_directory` |
| 0x0B0B, 0x0B98, 0x0C3B | StrLoad, StrCat, Chr (pure string operations) | translated inline (Pascal strings capped at 255) |
| 0x04F7 | text-record initialization, `retf 8` | `text_assign(path)` |
| 0x056C | shared open path, mode 0xD7B2 (output), `retf 4` | `text_rewrite()` |
| 0x04A2 | `xor ax,ax; xchg [DS:0x4426],ax` (reads and clears), `retf` | `io_result(&word)` |
| 0x709:0x0BF5 | row clear, print at row 0x18, key wait through `0x899:0x0059`, `retf 6` (`RES_0709_0BF5`) | `display_and_input(message, color)` |
| 0x0919 + 0x084A | string output width 0, then CR/LF through the buffered text service | `write_line(string)` |
| 0x05C1 | shared close/flush, `retf 4` | `text_close()` |

Host spans (PM review i8476): the live path span must include the raw byte after the
length byte, because the failed-open message reads `DS:0x537B` even for a zero-length
path; each node carries its supplied name span, checked before the byte-1 read (at least
2 bytes) and before the write (at least length + 1 bytes), so the model never reads past
host memory where the engine reads raw DS or heap bytes; a zero-length name still has its
byte 1 tested and one byte forwarded, as in the engine. Host guards (`max_open_attempts`,
`max_nodes`) bound the two engine loops and report
`RETRY_LIMIT` or `NODE_LIMIT`, statuses the engine cannot produce. A missing callback at a
reached boundary reports `NEED_HOOK` with the source offset in `next_offset`; a callback
failure reports `HOST_FAILURE` with every earlier effect kept. The private CS literals are
caller-supplied with their exact original lengths (12, 17, 1), the convention of the merged
+02AE model; their bytes are not in the public tree.

## 4. Classification

Whole-function **path-complete model**: all 84 instruction starts are covered at the
semantics level and all five branches are represented; the 14 calls are boundaries. It is
not a strict full translation (the text-file services, the prompt and +02AE are callbacks)
and no native run was made (host heavy-native hold). No strict credit is claimed.

## 5. Tests

Synthetic callbacks record the call order. Success path with three nodes (one skipped on
0x2A, one with an empty name whose byte at +1 is still read): order `E A R I W W C`, the
assembled path, counts, the two lines passed to the writer; nil list still closes; two
failed opens with the exact message (prefix + first path character + suffix) and color
0x0E, then success; the retry guard; each of the seven boundaries missing, in reach order,
with the stopped offset; close and directory failures keeping earlier effects; a cyclic
list hitting the node guard; path overflow past 255; argument checks (literal lengths,
live-string span, null hooks, null output, null node name); host-span negatives: a
zero-length live path without its next byte (refused before any effect) and with it
(accepted, the byte lands in the message), a node span of 1 (refused at that node), a
length byte past the node span (refused before the write), zero-length names deciding
on byte 1 and forwarding one byte, and a later node's span failure keeping the earlier
line. Builds: strict
(`-std=c11 -Wall -Wextra -Wpedantic -Werror`) and the same under
`-fsanitize=address,undefined`.

## 6. Proposed registration (for PM, not applied)

```cmake
# poolrad_recovered sources: + src/recovered/overlay_u6f_01b8_write_list.c
add_executable(poolrad_overlay_u6f_01b8_write_list_tests tests/recovered/test_overlay_u6f_01b8_write_list.c)
target_link_libraries(poolrad_overlay_u6f_01b8_write_list_tests PRIVATE poolrad_recovered)
add_test(NAME poolrad_overlay_u6f_01b8_write_list_tests COMMAND poolrad_overlay_u6f_01b8_write_list_tests)
```

## 7. Queue

Follow-ons after PM review of this unit, in the assigned order: +02AE (whole function,
composing with its existing behavioral model) and +1446 (cold entry; pN's ownership map and
full-state notes are the leads). Neither is started.
