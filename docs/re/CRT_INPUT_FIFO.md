# Bounded CRT input FIFO

`pr_crt_input_fifo_service` supplies a small, caller-owned event FIFO for the
existing `pr_crt_int16_service` boundary. A host enqueues 16-bit AX events
(scan code in AH, character in AL). AH=01 checks the front without consuming
it and reports availability in ZF; when nonempty AX contains that event.
AH=00 consumes one event into AX. Other flags and CPU fields are preserved,
except IP advances past the two-byte INT. The empty AH=01 case leaves AX
unchanged. These explicit choices form the adapter contract; they are not a
claim about all registers or behavior of every BIOS implementation.

The fixed capacity is 16. Push reports full without changing the queue; the
caller owns the FIFO and serializes producer/service access. A missing event
for AH=00 or an unsupported function declines without changing the queue or
output CPU. The service never reads or writes guest RAM. It expects the CRT
caller's input and output CPU objects and queue storage not to overlap.

**A declined empty read is not a resumable yield.** The current CRT ReadKey
model returns `PR_CRT_SERVICE` at IP `031D` after earlier instructions have
already modified state, and accepts its normal entry at `030F`; restarting it
would repeat those effects. Supply an event before invoking a synchronous read,
or treat refusal as a terminal bounded stop until a separate continuation
contract exists. The FIFO has no OS keyboard source, prompt implementation,
resident display integration, save initialization, movement, or frame output.
It makes no gameplay, DOS parity, or strict-credit claim.

`poolrad_crt_input_fifo_tests` exercises the service directly and through the
existing CRT KeyPressed/ReadKey implementations, including FIFO wrap and full
behavior, nonconsuming poll, refusal, preserved CPU fields, no service RAM
writes, and extended Up (AX=4800h) across two ReadKey calls. The test inputs are
synthetic. The underlying CRT boundary and return behavior are documented in
`CRT_KEYPRESSED_02FD_CPU.md` and `CRT_READKEY_030F_CPU.md`.
