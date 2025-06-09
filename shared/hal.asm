SECTION code_user

PUBLIC fputc_cons_native
PUBLIC _fputc_cons_native

PUBLIC fgetc_cons
PUBLIC _fgetc_cons


fputc_cons_native:
_fputc_cons_native:
    pop     bc  ;return address
    pop     hl  ;character to print in l
    push    hl
    push    bc
    ld      a,l
    out     (1),a
    ret

fgetc_cons:
_fgetc_cons:
    ret
