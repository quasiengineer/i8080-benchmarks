LD A, 0xBB   # 3E BB
STA [0x1006] # 32 06 10
LD A, 0xAA   # 3E AA
LDA [0x1006] # 3A 06 10
OUT 0x01     # D3 01
HLT          # 76