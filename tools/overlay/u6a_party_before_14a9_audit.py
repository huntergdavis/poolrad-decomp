#!/usr/bin/env python3
"""Compare complete u6A +14A9 party reorder with its original instructions."""
from party_reorder_oracle import main

if __name__ == "__main__":
    main(entry=0x14A9, end=0x1601, instructions=104,
         digest="76faaa709b22d0d563ae00745d122c9bd9d192a4f31bec85c60e69b1514486c7",
         symbol="pr_u6a_party_before_14a9",
         source="overlay_u6a_party_before_14a9.c", caller=0x1809)
