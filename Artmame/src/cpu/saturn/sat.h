enum {
	SATURN_A=1, SATURN_B, SATURN_C, SATURN_D,
	SATURN_R0, SATURN_R1, SATURN_R2, SATURN_R3, SATURN_R4,
	SATURN_RSTK0, SATURN_RSTK1, SATURN_RSTK2, SATURN_RSTK3,
	SATURN_RSTK4, SATURN_RSTK5, SATURN_RSTK6, SATURN_RSTK7,
    SATURN_PC, SATURN_D0, SATURN_D1,

	SATURN_P,
	SATURN_IN,
	SATURN_OUT,
	SATURN_CARRY,
	SATURN_ST,
	SATURN_HST,

	SATURN_EA,
	SATURN_NMI_STATE,
	SATURN_IRQ_STATE
};

#define PEEK_OP(pc) cpu_readop(pc)
#define PEEK_NIBBLE(adr) cpu_readmem_20(adr)

