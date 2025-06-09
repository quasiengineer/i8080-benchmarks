# difference between program that has 2 NOPs and 1 NOP should be 4 cycles
NOP          # 0x00
NOP          # 0x00
LDA [0xF880] # 3A 80 F8
OUT 0x01     # D3 01
HLT          # 76