"""Project validated supplied U19 RGB24 rows without claiming a native target.

The supplied packet hashes its complete byte extent, including row padding.
An independent target comparison needs the first 640 RGB24 pixels of each
row. Both digests remain distinct; this module does not bind a backend event,
clock, target, or display completion.
"""

from dataclasses import dataclass
from hashlib import sha256

from tools.replay.cold_start_codeword_row14_completed_backend_rgb24_v2 import (
    CompletedFrameV2, HEIGHT, MAX_RGB_BYTES, STATUS as RGB_STATUS, WIDTH,
)


STATUS = "SUPPLIED_ROW_PROJECTION_NO_NATIVE_TARGET_ASSERTION"
ROW_BYTES = WIDTH * 3
CANONICAL_BYTE_LENGTH = ROW_BYTES * HEIGHT


@dataclass(frozen=True)
class SuppliedRgbProjectionV2:
    status: str
    frame: CompletedFrameV2
    full_byte_length: int
    full_sha256: str
    canonical_rgb24: bytes
    canonical_sha256: str
    native_coverage_proved: bool = False


def project_supplied_rgb24_rows_v2(
    frame: CompletedFrameV2, *, enabled: bool = False,
) -> SuppliedRgbProjectionV2:
    """Keep the full supplied digest and derive tight 640x400 RGB24 rows."""
    if enabled is not True:
        raise ValueError("U19 supplied-row projection requires exact opt-in")
    if (type(frame) is not CompletedFrameV2 or frame.version != 2 or
            frame.status != RGB_STATUS or frame.width != WIDTH or
            frame.height != HEIGHT or frame.pixel_format != "RGB24" or
            frame.channel_order != "RGB" or
            type(frame.row_stride) is not int or
            not ROW_BYTES <= frame.row_stride <= MAX_RGB_BYTES // HEIGHT or
            type(frame.rgb24) is not bytes or
            len(frame.rgb24) != frame.row_stride * HEIGHT or
            frame.rgb_byte_length != len(frame.rgb24)):
        raise ValueError("validated supplied RGB24 frame required")

    full_sha256 = sha256(frame.rgb24).hexdigest()
    if frame.rgb_sha256 != full_sha256:
        raise ValueError("supplied full-byte digest differs")

    canonical = b"".join(
        frame.rgb24[row * frame.row_stride:
                    row * frame.row_stride + ROW_BYTES]
        for row in range(HEIGHT)
    )
    if len(canonical) != CANONICAL_BYTE_LENGTH:
        raise ValueError("supplied canonical row extent differs")
    return SuppliedRgbProjectionV2(
        status=STATUS, frame=frame,
        full_byte_length=len(frame.rgb24), full_sha256=full_sha256,
        canonical_rgb24=canonical,
        canonical_sha256=sha256(canonical).hexdigest(),
    )
