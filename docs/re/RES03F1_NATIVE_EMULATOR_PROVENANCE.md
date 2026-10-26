# RES03F1 native executable provenance

Task t108, RES03F1 clear0137 companion. Base: `52aac3da2c6db39979b072ae49cb0efc905dbef3`.

The native provider resolves DOSBox-X once through the shared helper, launches that returned path, and records executable version/hash/pin status. Its audit companion remains the row checker. Recovered C, cases, masks, captured windows and tracing are unchanged. This is provenance evidence, not conversion credit.

Fresh before/after comparisons are **not yet verified**. Baseline provider source is preserved from the stated main revision. Both phases must use identical C/header inputs and produce complete reports equal after removing only `dosbox_x`, including the native capture hash. Each phase records provider/model hashes and executable identity. Partial runs do not count as passing receipts.

The native report has `result_sha256` for captured output; it does not emit an original-body `source_sha256` field. The existing audit source checks are a separate mechanism. This change adds no original-body pin or pinned-build certification.

The CSV removes exactly this provider: 184 lists/169 files becomes 183/168. These are structural counts; a fresh tracked-source AST census remains pending.

Original media and generated programs remain under ignored resources. Private receipts: `resources/t108-batch6/resume/`. Native, media and reviewer gates must be named by exact head. No natural-route/full-game/full-RAM claim or recovery/replacement credit is made.
