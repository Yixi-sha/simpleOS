#ifndef __PROCESS_H__
#define __PROCESS_H__
#include "lib.h"
#include "mm.h"
#include "cpu.h"

#define STACK_SIZE 32768
#define TASK_RUNNING		(1 << 0)
#define TASK_INTERRUPTIBLE	(1 << 1)
#define	TASK_UNINTERRUPTIBLE	(1 << 2)
#define	TASK_ZOMBIE		    (1 << 3)	
#define	TASK_STOPPED		(1 << 4)
#define PF_KTHREAD	(1 << 0)

#define KERNEL_CS 	(0x08)
#define	KERNEL_DS 	(0x10)

#define	USER_CS		(0x28)
#define USER_DS		(0x30)

#define CLONE_FS	(1 << 0)
#define CLONE_FILES	(1 << 1)
#define CLONE_SIGNAL	(1 << 2)



struct memory_struct
{
    pml4t_t* pgd; // page table point

    unsigned long codeStart, codeEnd;
    unsigned long dataStart, dataEnd;
    unsigned long rodataStart, rodataEnd;
    unsigned long brkStart, brkEnd;
    unsigned long stackStart;
};

struct thread_struct
{
    unsigned long rsp0;	//kernel stack

	unsigned long rip;  // switch ip
	unsigned long rsp;	// switch stack

	unsigned long fs;
	unsigned long gs;

	unsigned long cr2;
	unsigned long trap_nr;
	unsigned long error_code;
};

struct task_struct
{
    struct List list;
    volatile long state;
    unsigned long flag;

    struct memory_struct* mm;
    struct thread_struct* thread;

    unsigned long addrLimit;
    long pid;
    long counter;
    long signal;
    long priority;
};

union task_union
{
    struct task_struct task;
    unsigned long stack[STACK_SIZE / sizeof(unsigned long)];
}__attribute__((aligned (8)));




#define INIT_TASK(tsk)	\
{			\
	.state = TASK_UNINTERRUPTIBLE,		\
	.flag = PF_KTHREAD,		\
	.mm = &init_mm,			\
	.thread = &init_thread,		\
	.addrLimit = 0xffff800000000000,	\
	.pid = 0,			\
	.counter = 1,		\
	.signal = 0,		\
	.priority = 0		\
}

struct tss_struct
{
	unsigned int  reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist1;
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomapbaseaddr;
}__attribute__((packed));

#define INIT_TSS \
{	.reserved0 = 0,	 \
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.reserved1 = 0,	 \
	.ist1 = 0xffff800000007c00,	\
	.ist2 = 0xffff800000007c00,	\
	.ist3 = 0xffff800000007c00,	\
	.ist4 = 0xffff800000007c00,	\
	.ist5 = 0xffff800000007c00,	\
	.ist6 = 0xffff800000007c00,	\
	.ist7 = 0xffff800000007c00,	\
	.reserved2 = 0,	\
	.reserved3 = 0,	\
	.iomapbaseaddr = 0	\
}


struct pt_regs
{
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rbx;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long rbp;
	unsigned long ds;
	unsigned long es;
	unsigned long rax;
	unsigned long func;
	unsigned long errcode;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

#define switch_to(prev,next)			\
do{							\
	__asm__ __volatile__ ( \
				"pushq	%%rax	\n\t"	\
				"pushq	%%rbp    \n\t"   \
				"pushq	%%rdi    \n\t"   \
				"pushq	%%rsi    \n\t"   \
				"pushq	%%rdx    \n\t"   \
				"pushq	%%rcx    \n\t"   \
				"pushq	%%rbx    \n\t"   \
				"pushq	%%r8    \n\t"   \
				"pushq	%%r9    \n\t"   \
				"pushq	%%r10    \n\t"   \
				"pushq	%%r11    \n\t"   \
				"pushq	%%r12    \n\t"   \
				"pushq	%%r13    \n\t"   \
				"pushq	%%r14    \n\t"   \
				"pushq	%%r15    \n\t"   \
				"movq	%%es,	%%rax    \n\t"   \
				"pushq	%%rax    \n\t"   \
				"movq	%%ds,	%%rax    \n\t"   \
				"pushq	%%rax    \n\t"   \
				\
				"movq	%%rsp,	%0	\n\t"	\
				"movq	%2,	%%rsp	\n\t"	\
				"leaq	1f(%%rip),	%%rax	\n\t"	\
				"movq	%%rax,	%1	\n\t"	\
				"pushq	%3		\n\t"	\
				"jmp	__switch_to	\n\t"	\
				"1:	\n\t"	\
				\
				"popq	%%rax;	\n\t"	\
				"movq	%%rax,	%%ds;  \n\t"	\
				"popq	%%rax;   \n\t"   \
				"movq	%%rax,	%%es;   \n\t"	\
				"popq	%%r15	\n\t"	\
				"popq	%%r14	\n\t"	\
				"popq	%%r13	\n\t"	\
				"popq	%%r12	\n\t"	\
				"popq	%%r11	\n\t"	\
				"popq	%%r10	\n\t"	\
				"popq	%%r9  	\n\t"	\
				"popq	%%r8	 \n\t"	\
				"popq	%%rbx    \n\t"   \
				"popq	%%rcx    \n\t"   \
				"popq	%%rdx    \n\t"   \
				"popq	%%rsi    \n\t"   \
				"popq	%%rdi    \n\t"   \
				"popq	%%rbp    \n\t"   \
				"popq	%%rax	\n\t"	\
				:"=m"(prev->thread->rsp),"=m"(prev->thread->rip)		\
				:"m"(next->thread->rsp),"m"(next->thread->rip),"D"(prev),"S"(next)	\
				:"memory"		\
				);			\
}while(0)


#define GET_CURRENT \
	"movq %rsp,	%rbx	\n\t"	\
	"andq $-32768,%rbx	\n\t"



inline struct task_struct* get_current();
void task_init();
int kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg, unsigned long flag);
unsigned long do_exit(unsigned long code);
unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);
unsigned long init(unsigned long arg);
inline void __switch_to(struct task_struct *prev,struct task_struct *next);
unsigned long do_execve(struct pt_regs* regs);
void userFunc();
unsigned long systemCallFunc(struct pt_regs* regs);

#define current get_current()

//system call
#define MAX_SYSTEM_CALL_NR 128
unsigned long noSystemCall(struct pt_regs* regs);
typedef unsigned long (* systemCallF)(struct pt_regs*);
unsigned long sys_printf(struct pt_regs* regs);

#endif