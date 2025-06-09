; CRT for i8080-sbc, should be placed at z88dk\lib\target\8080\classic\8080_crt.asm

    module i8080_crt0

    defc    crt0 = 1
    INCLUDE "zcc_opt.def"

    EXTERN    _main           ;main() is always external to crt0 code
    EXTERN    asm_im1_handler

    PUBLIC    cleanup         ;jp'd to by exit()

IFNDEF CLIB_FGETC_CONS_DELAY
    defc CLIB_FGETC_CONS_DELAY = 150
ENDIF

    defc    TAR__clib_exit_stack_size = 4
    defc    TAR__register_sp = 0x0000
    defc    CRT_KEY_DEL = 12
    defc    __CPU_CLOCK = 3125000
    defc    CONSOLE_COLUMNS = 64
    defc    CONSOLE_ROWS = 32
    INCLUDE "crt/classic/crt_rules.inc"

    defc    CRT_ORG_CODE = 0x0000

    org	    CRT_ORG_CODE

if (ASMPC <> $0000)
    defs    CODE_ALIGNMENT_ERROR
endif

    jp      program
    INCLUDE	"crt/classic/crt_z80_rsts.asm"

program:
    INCLUDE "crt/classic/crt_init_sp.asm"
    INCLUDE "crt/classic/crt_init_atexit.asm"
    call    crt0_init_bss
    ld      hl,0
    add     hl,sp
    ld      (exitsp), hl
; Optional definition for auto MALLOC init
; it assumes we have free space between the end of
; the compiled program and the stack pointer
IF DEFINED_USING_amalloc
    INCLUDE "crt/classic/crt_init_amalloc.asm"
ENDIF
    push    bc	;argv
    push    bc	;argc
    call    _main
    pop     bc
    pop     bc
cleanup:
    call    crt0_exit

    INCLUDE "crt/classic/crt_terminate.inc"
    INCLUDE "crt/classic/crt_runtime_selection.asm"
    INCLUDE	"crt/classic/crt_section.asm"
