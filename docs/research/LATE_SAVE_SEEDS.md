# Later-game DOS save seeds: research handoff

2026-10-11. Scope: original *Pool of Radiance* DOS v1.3 parity fixtures for
Sokal Keep, wilderness, and later campaign areas. **No complete, load-verified
late-game DOS save bundle was found in this search.** Do not count the leads
below as parity fixtures until they load in the audited DOS game.

## Verified owner-held saves

The ignored `resources/DOS/POOLRAD` installation has two complete six-person
slots. Its `START.EXE` SHA-256 is
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`;
`GAME.OVR` is
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`.
These are the audited v1.3 executables accepted by
[`tools/oracle/prepare_run.sh`](../../tools/oracle/prepare_run.sh).

| Slot | World-file SHA-256 | Direct DOS observation | Use |
| --- | --- | --- | --- |
| A | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` | 15,4 N at 10:50; 14,4 N after one step ([oracle record](../oracle/README.md)) | Existing early movement baseline |
| J | `4b64213a9d19d082af6a075b7aa54407c5cfe3883f6ade1f3862283c130d6ca9` | 14,5 S at 10:56, loaded directly in an isolated DOSBox-X run `late-save-research-slot-j-001` | Another early New Phlan baseline |

Both world files are 13,137 bytes. They differ in only 289 byte positions and
are both in the first local area; neither seeds Sokal or wilderness. Slot J's
screen and log are in ignored `resources/oracle-runs/late-save-research-slot-j-001`.

## Public leads, not yet fixtures

| Lead | What is available | Why it cannot yet seed DOS parity |
| --- | --- | --- |
| [Author-posted C64 Kobold Caves save disk](https://www.reddit.com/r/c64/comments/e3sxk1/an_unusual_pool_of_radiance_save_disk/) | The author describes a `.D64` checkpoint before the main caves battle, after extensive campaign progress. | The linked `anwu.org` host did not resolve on 2026-10-11; no bytes or hash were obtained. It is C64, not DOS. [Wish](https://github.com/malcyon/wish) implements C64/DOS conversion and a [DOS load-and-walk validation script](https://github.com/malcyon/wish/blob/main/tools/convert/README.md), but this disk has not been converted or loaded. |
| [Reported DOS `SAVGAMA.DAT`](https://www.resetera.com/threads/pool-of-radiance-save-bugged-out-help.162989/) | A forum lead describes a corrupted world-state save. The page was unreadable from the PM review environment. | No complete, load-verified bundle or matching sidecars were independently established. Treat this only as an unverified corruption lead. |
| [Original PC save directory](https://forums.goldbox.games/index.php?topic=2430.0) | A contributor supplied original Wizard Works default characters. | It is a starting party, not a later-area checkpoint. |

No distinct late DOS save was found in the likely owner Steam, GOG, Downloads,
or Documents paths. The other workspace copies contain the same A/J installation.
This is a targeted search result, not proof that no public save exists.

## Exact DOS bundle and acceptance gate

For slot `X` (A–J), retain `SAVGAMX.DAT` plus every matching
`CHRDATX1`–`CHRDATX8` `.SAV`, `.ITM`, and `.SPC` file that exists. For slot A,
the actual prefix is `CHRDATA`; slot J is `CHRDATJ`. Missing optional item or
spell sidecars are normal. `POOL.CFG` must point the save path at the directory
containing these files. The [DOS save-file description](https://www.easydamus.com/PoR.pdf)
specifies the save filename pattern and POOL.CFG save path; the local v1.3
A/J bundles confirm it.

1. Keep any acquired bundle in ignored `resources/`, record source URL, date,
   per-file SHA-256, and claimed game version. Never overwrite the source.
2. Use `tools/oracle/prepare_run.sh NAME` for a private v1.3 run. Copy the
   complete bundle into that run's `POOLRAD` directory, leaving the audited
   executable and game resources intact.
3. Start with `tools/oracle/launch.sh RUN_DIR`, choose **Load Saved Game** and
   the slot, then verify the visible area/position, one movement or event, and
   a subsequent original-game save/reload. Preserve the exact input sequence,
   screen captures, and before/after hashes under ignored `resources/`.
4. Promote only an original-game load-verified bundle to the parity fixture
   inventory. A converter or editor output must pass the same gate and be
   identified as converted or edited, not as a naturally played DOS save.

## Recommended original-game capture work

The shortest reliable route is to play the audited DOS v1.3 game from the
verified A/J checkpoint and save **in the original game** at these boundaries:

| Priority | Checkpoint | Why it is useful |
| --- | --- | --- |
| 1 | Sokal Keep boat arrival; before the first undead encounter; after the keep's quest resolution | Tests area transition, scripted text, password/parley, combat, and quest flags. The recovered area map identifies Sokal as ECL/GEO 21. |
| 2 | Stojanow Gate before exiting; first wilderness step; adjacent wilderness sector; before entering Kobold Caves | Tests city-to-overland transition, outdoor movement/encounters, sector boundary, and wilderness-to-dungeon transition. The recovered maps identify the gate as 9 and Kobold Caves as 13; wilderness scripts are 25–27. |
| 3 | One later city/quest area and Valjevo Castle entrance | Tests later script flags and a richer party without requiring a full campaign replay first. |

Use separate slot letters or independent private runs so each checkpoint
survives. Record area/coordinates/facing, game time, party, quest state, exact
keys, and hashes. Follow the [oracle capture protocol](../oracle/README.md)
and [recovered area identities](../prior-research/macmaps/AREA_NAMES.md).
Teleporting or byte-editing an early save can help investigate a handler, but
it is a synthetic state and cannot establish a natural DOS campaign route.

**Proposed next step:** assign one original-DOS play/capture lane after
staffing and capacity review, starting with Sokal and then wilderness.
Revisit the C64 Kobold Caves lead only if the disk becomes available; Wish
conversion then needs its own v1.3 load-and-walk check.
