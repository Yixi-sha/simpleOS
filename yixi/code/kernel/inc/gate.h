#ifndef __GATE_H__
#define __GATE_H__

#define _set_gate(gate_selector_addr,attr,ist,code_addr) \
do \
{\
    unsigned long __d0 = 0, __d1 = 0;    \
    __asm__ __volatile__(           \
        "movw %%dx, %%ax    \n\t"   \
        "andq $0x7, %%rcx   \n\t"   \
        "addq %4, %%rcx      \n\t"  \
        "shlq $32, %%rcx    \n\t"   \
        "addq %%rcx, %%rax  \n\t"   \
        "xorq %%rcx, %%rcx  \n\t"   \
        "movl %%edx, %%ecx  \n\t"   \
        "shrq $16, %%rcx     \n\t"  \
        "shlq $48, %%rcx     \n\t"  \
        "addq %%rcx, %%rax  \n\t"   \
        "movq %%rax, %0     \n\t"   \
        "shrq $32, %%rdx    \n\t"   \
        "movq %%rdx, %1     \n\t"   \
        :"=m"(*((unsigned long *)(gate_selector_addr)))	,					\
	    "=m"(*(1 + (unsigned long *)(gate_selector_addr))),"=&a"(__d0),"=&d"(__d1)		\
        :"i"(attr << 8),									\
	    "3"((unsigned long *)(code_addr)),"2"(0x8 << 16),"c"(ist)				\
	    :"memory"		\
    );         \
}while(0)
// %0==>gate_selector_addr %1==>gate_selector_addr + 1  %4==>attr<<8   d==>code_addr==>funcAddr a==>0x8 << 16 c==>ist

#define load_TR(n) \
do{                \
     __asm__ __volatile__(           \
     "ltr %%ax         \n\t"   \
     :                        \
     :"a"(n << 3)            \
     :"memory"               \
    );         \
}while(0)

struct desc_struct 
{
	unsigned char x[8];
};

struct gate_struct
{
	unsigned char x[16];
};

inline void set_intr_gate(unsigned int n, unsigned char ist, void* addr);
inline void set_trap_gate(unsigned int n, unsigned char ist, void* addr);
inline void set_system_gate(unsigned int n, unsigned char ist, void* addr);
inline void set_system_intr_gate(unsigned int n,unsigned char ist,void * addr);
void set_tss64(unsigned int * Table,unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3, \
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7);
inline void set_tss_descriptor(unsigned int n,void * addr);


#endif