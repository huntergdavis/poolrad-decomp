#!/usr/bin/env python3
"""Compare complete u6A +1601 party reorder with its original instructions."""
from party_reorder_oracle import main

if __name__ == "__main__":
    main(entry=0x1601, end=0x1720, instructions=84,
         digest="5d1b9b66f76f31872abcd27dc75a268a9b35263adc2c02fb9725a95e21ace285",
         symbol="pr_u6a_party_after_1601",
         source="overlay_u6a_party_after_1601.c", caller=0x1813)
