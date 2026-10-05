        .globl  _bswap16
_bswap16:
        moveq   #0,d0        | zero-extend
        move.w  6(sp),d0     | load argument
        ror.w   #8,d0        | swap bytes
        rts

