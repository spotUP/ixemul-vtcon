        .globl  _bswap64
_bswap64:
        move.l  8(sp),d0     | original low -> return high
        move.l  4(sp),d1     | original high -> return low

        ror.w   #8,d0
        swap    d0
        ror.w   #8,d0

        ror.w   #8,d1
        swap    d1
        ror.w   #8,d1

        rts

