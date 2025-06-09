@echo off

SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

zcc +8080 hello.c ../../shared/hal.asm -m -o hello