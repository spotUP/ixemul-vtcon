#define VEC_STUB(name, vec) \
	__asm__(".text\n\t.even\n\t.globl " #name "\n" #name ":\n\t" \
	        "movel _ixemulbase,a0\n\tjmp a0@(-" #vec ":w)\n")
