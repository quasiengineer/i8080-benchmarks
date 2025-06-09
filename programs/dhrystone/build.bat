@echo off

SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

zcc +8080 dhry_1.c dhry_2.c ../../shared/hal.asm -O2 -m -o dhrystone