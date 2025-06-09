@echo off

SET ZCCCFG=%Z88DK_DIR%lib\config\
SET PATH=%Z88DK_DIR%bin;%PATH%

zcc +8080 core_list_join.c core_main.c core_matrix.c core_state.c core_util.c core_portme.c ../../shared/hal.asm -DPERFORMANCE_RUN=1 -DITERATIONS=10 -O2 -m -o coremark
