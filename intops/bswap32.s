        .globl  _bswap32
_bswap32:
        move.l  4(sp),d0     | load argument
        ror.w   #8,d0        | swap low byte pair
        swap    d0           | swap words
        ror.w   #8,d0        | swap high byte pair
        rts

