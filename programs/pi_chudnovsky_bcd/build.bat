@echo off

SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

zcc +8080 pi.c bn.c ../../shared/hal.asm -m -o pi_chudnovsky_bcd_1000