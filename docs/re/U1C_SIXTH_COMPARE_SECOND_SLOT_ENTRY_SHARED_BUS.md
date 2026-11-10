# Bounded sixth COMPARE second slot entry

The continuation stores the returned first word at SS:BP-2, changes AL to 2 while preserving AH and flags, pushes AX, and follows the checked far call at 018C and the loaded 0025 jump stub. It stops at U25:0136 before executing the second slot body. Memory effects use the shared bus, including aliased stack and data. A malformed call or stub retains effects already performed.

The direct boundary requires ordinary CS, DS, and SS, BP at least 020A, and SP equal to BP-0204. The composed wrapper retains its nested empty-session guards and the existing outer frame constraint. Code is expected to remain immutable and outside the written stack footprint. Earlier stop and yield paths remain those of the first-slot-return wrapper.

The source sequence is based on the candidate analysis in `U1C_COMPARE_0126_CPU.md` and the established call boundary in `U1C_SIXTH_COMPARE_SLOT_ENTRY_SHARED_BUS.md`. The integration fixture uses synthetic preceding SAVE and first-operand data. Direct checks cover flags, stack/data aliasing, malformed call and stub, and invalid frame. This does not establish native reach, operand type, the second slot body, complete COMPARE behavior, or decompilation readiness or credit.
