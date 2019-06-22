#ifndef __INTRRUPT_H__
#define __INTRRUPT_H__
#include "process.h"

#define SAVE_ALL				   \
	"cld;			   \n\t"		\
	"pushq	%rax;		\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%es,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%ds,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"xorq	%rax,	%rax;	\n\t"		\
	"pushq	%rbp;		\n\t"		\
	"pushq	%rdi;		\n\t"		\
	"pushq	%rsi;		\n\t"		\
	"pushq	%rdx;		\n\t"		\
	"pushq	%rcx;		\n\t"		\
	"pushq	%rbx;		\n\t"		\
	"pushq	%r8;		\n\t"		\
	"pushq	%r9;		\n\t"		\
	"pushq	%r10;		\n\t"		\
	"pushq	%r11;		\n\t"		\
	"pushq	%r12;		\n\t"		\
	"pushq	%r13;		\n\t"		\
	"pushq	%r14;		\n\t"		\
	"pushq	%r15;		\n\t"		\
	"movq	$0x10,	%rdx;	\n\t"		\
	"movq	%rdx,	%ds;	\n\t"		\
	"movq	%rdx,	%es;	\n\t"

#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr)  IRQ_NAME2(IRQ##nr)

#define Build_IRQ(nr)							\
void IRQ_NAME(nr);						\
__asm__ (	\
            SYMBOL_NAME_STR(IRQ)#nr"_interrupt:		\n\t"	\
			"pushq	$0x00				\n\t"	\
			SAVE_ALL					\
			"movq	%rsp,	%rdi			\n\t"	\
			"leaq	ret_from_intr(%rip),	%rax	\n\t"	\
			"pushq	%rax				\n\t"	\
			"movq	$"#nr",	%rsi			\n\t"	\
			"jmp	do_IRQ	\n\t"                \
            );

typedef struct hw_int_type
{
	void (*enable)(unsigned long irq);
	void (*disable)(unsigned long irq);

	unsigned long (*install)(unsigned long irq,void * arg);
	void (*uninstall)(unsigned long irq);

	void (*ack)(unsigned long irq);
}hw_int_controller;

typedef struct 
{
	hw_int_controller* controller;

	char* irq_name;
	unsigned long parament;
	void (*handler)(unsigned long nr, unsigned long parament, struct pt_regs* regs);
	unsigned long flag;
}irq_desc_T;

#define NR_IRQS 24

int register_irq(unsigned long irq, void* arg, void (*handler)(unsigned long nr, unsigned long parament, struct pt_regs* regs), \
				unsigned long parament, hw_int_controller* controller, char* irqName);

void do_IRQ(struct pt_regs * regs,unsigned long nr);
int unregister_irq(unsigned long irq);

#endif