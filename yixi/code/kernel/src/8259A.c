#include "../inc/8259A.h"
#include "../inc/linkage.h"
#include "../inc/printk.h"
#include "../inc/lib.h"
#include "../inc/gate.h"

extern void (* interrupt[24])(void);

void init_8259A()
{
	int i;
	for(i = 32;i < 56;i++)
	{
		set_intr_gate(i , 2 , interrupt[i - 32]);
	}

	color_printk(RED,BLACK,"8259A init \n");

	//8259A-master	ICW1-4
	io_out8(0x20,0x11);
	io_out8(0x21,0x20);
	io_out8(0x21,0x04);
	io_out8(0x21,0x01);

	//8259A-slave	ICW1-4
	io_out8(0xa0,0x11);
	io_out8(0xa1,0x28);
	io_out8(0xa1,0x02);
	io_out8(0xa1,0x01);

	//8259A-M/S	OCW1
	//io_out8(0x21,0x00);
	//io_out8(0xa1,0x00);

	//8259A-M/S  OCW1
	io_out8(0x21,0xfd);
	io_out8(0xa1,0xff);

	sti();
}

/*void do_IRQ(unsigned long regs,unsigned long nr)	//regs:rsp,nr
{
	unsigned char x;
	color_printk(RED,BLACK,"do_IRQ:%#08x\t",nr);
	x = io_in8(0x60);
	color_printk(RED,BLACK,"key code:%#08x\n",x);
	io_out8(0x20,0x20);
}*/
