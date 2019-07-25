#include "../inc/gate.h"

extern struct desc_struct GDT_Table[];
extern struct gate_struct IDT_Table[];
extern unsigned int TSS64_Table[26]; 

inline void set_intr_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n , 0x8E , ist , addr);	//P = 1,DPL=0,TYPE=E
}


inline void set_trap_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n , 0x8F , ist , addr);	//P = 1,DPL=0,TYPE=F
}

inline void set_system_gate(unsigned int n, unsigned char ist, void* addr)
{
    _set_gate(IDT_Table + n , 0xEF , ist , addr);	//P = 1,DPL=3,TYPE=F
}

inline void set_system_intr_gate(unsigned int n,unsigned char ist,void * addr)	//int3
{
	_set_gate(IDT_Table + n , 0xEE , ist , addr);	//P = 1,DPL=3,TYPE=E
}

void set_tss64(unsigned int * Table,unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
	*(unsigned long *)(Table+1) = rsp0;
	*(unsigned long *)(Table+3) = rsp1;
	*(unsigned long *)(Table+5) = rsp2;

	*(unsigned long *)(Table+9) = ist1;
	*(unsigned long *)(Table+11) = ist2;
	*(unsigned long *)(Table+13) = ist3;
	*(unsigned long *)(Table+15) = ist4;
	*(unsigned long *)(Table+17) = ist5;
	*(unsigned long *)(Table+19) = ist6;
	*(unsigned long *)(Table+21) = ist7;	
}


inline void set_tss_descriptor(unsigned int n,void * addr)
{
	unsigned long limit = 103;
	*(unsigned long *)(GDT_Table + n) = (limit & 0xffff) | (((unsigned long)addr & 0xffff) << 16) | (((unsigned long)addr >> 16 & 0xff) << 32) | ((unsigned long)0x89 << 40) | ((limit >> 16 & 0xf) << 48) | (((unsigned long)addr >> 24 & 0xff) << 56);	/////89 is attribute
	*(unsigned long *)(GDT_Table + n + 1) = ((unsigned long)addr >> 32 & 0xffffffff) | 0;
}
