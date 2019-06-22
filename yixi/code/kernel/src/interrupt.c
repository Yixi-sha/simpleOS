#include "../inc/interrupt.h"
#include "../inc/linkage.h"
#include "../inc/lib.h"
#include "../inc/printk.h"
#include "../inc/mm.h"
#include "../inc/gate.h"
#include "../inc/interrupt.h"



Build_IRQ(0x20)
Build_IRQ(0x21)
Build_IRQ(0x22)
Build_IRQ(0x23)
Build_IRQ(0x24)
Build_IRQ(0x25)
Build_IRQ(0x26)
Build_IRQ(0x27)
Build_IRQ(0x28)
Build_IRQ(0x29)
Build_IRQ(0x2a)
Build_IRQ(0x2b)
Build_IRQ(0x2c)
Build_IRQ(0x2d)
Build_IRQ(0x2e)
Build_IRQ(0x2f)
Build_IRQ(0x30)
Build_IRQ(0x31)
Build_IRQ(0x32)
Build_IRQ(0x33)
Build_IRQ(0x34)
Build_IRQ(0x35)
Build_IRQ(0x36)
Build_IRQ(0x37)

void (* interrupt[24])(void)=
{
	IRQ0x20_interrupt,
	IRQ0x21_interrupt,
	IRQ0x22_interrupt,
	IRQ0x23_interrupt,
	IRQ0x24_interrupt,
	IRQ0x25_interrupt,
	IRQ0x26_interrupt,
	IRQ0x27_interrupt,
	IRQ0x28_interrupt,
	IRQ0x29_interrupt,
	IRQ0x2a_interrupt,
	IRQ0x2b_interrupt,
	IRQ0x2c_interrupt,
	IRQ0x2d_interrupt,
	IRQ0x2e_interrupt,
	IRQ0x2f_interrupt,
	IRQ0x30_interrupt,
	IRQ0x31_interrupt,
	IRQ0x32_interrupt,
	IRQ0x33_interrupt,
	IRQ0x34_interrupt,
	IRQ0x35_interrupt,
	IRQ0x36_interrupt,
	IRQ0x37_interrupt,
};

irq_desc_T interrupt_desc[NR_IRQS] = {0};

void do_IRQ(struct pt_regs * regs,unsigned long nr)	//regs:rsp,nr
{
	unsigned char x;
	//printk("doirq");
	irq_desc_T* irq = &interrupt_desc[nr -32];
	if(irq->handler != NULL)
		irq->handler(nr, irq->parament, regs);
	
	if(irq->controller != NULL && irq->controller->ack != NULL)
		irq->controller->ack(nr);
	
}


int register_irq(unsigned long irq, void* arg, void (*handler)(unsigned long nr, unsigned long parament, struct pt_regs* regs), \
				unsigned long parament, hw_int_controller* controller, char* irqName)
{
	irq_desc_T * p = &interrupt_desc[irq - 32];

	p->controller = controller;
	p->irq_name = irqName;
	p->parament = parament;
	p->flag = 0;
	p->handler = handler;

	p->controller->install(irq, arg);
	p->controller->enable(irq);
	return 0;
}

int unregister_irq(unsigned long irq)
{
	irq_desc_T * p = &interrupt_desc[irq - 32];
	p->controller->disable(irq);
	p->controller->uninstall(irq);

	p->controller = NULL;
	p->irq_name = NULL;
	p->parament = NULL;
	p->flag = 0;
	p->handler = NULL;

	return 0;
}
