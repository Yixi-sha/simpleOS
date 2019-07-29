#include "../inc/HPET.h"
#include "../inc/printk.h"
#include "../inc/mm.h"
#include "../inc/lib.h"
#include "../inc/APIC.h"
#include "../inc/time.h"
#include "../inc/interrupt.h"
#include "../inc/softIrq.h"

unsigned char * HPET_addr = NULL;

hw_int_controller HPET_int_controller = 
{
	.enable = IOAPIC_enable,
	.disable = IOAPIC_disable,
	.install = IOAPIC_install,
	.uninstall = IOAPIC_uninstall,
	.ack = IOAPIC_edge_ack,
};

volatile unsigned long jiffies = 0;

void HPET_handler(unsigned long nr, unsigned long parameter, struct pt_regs * regs)
{
	jiffies++;
	if(jiffies%100 == 0)
		//color_printk(RED, BLACK, "(HPET:%ld)",jiffies);
    	set_softIrq_status(TIMER_SIRQ);
}

extern struct time time;
extern unsigned char printFlagIrq ;
void HPET_init()
{
	unsigned int x;
	unsigned int * p;
	
	struct IO_APIC_RET_entry entry;
	HPET_addr = (unsigned char *)Phy_To_Virt(0xfed00000);
	
	//get RCBA address
	/* io_out32(0xcf8,0x8000f8f0);
	x = io_in32(0xcfc);
	x = x & 0xffffc000;	

	//get HPTC address
	if(x > 0xfec00000 && x < 0xfee00000)
	{
		p = (unsigned int *)Phy_To_Virt(x + 0x3404UL);
	}

	//enable HPET
	*p = 0x80;*/
	io_mfence();

	//init I/O APIC IRQ2 => HPET Timer 0
	entry.vector = 34;
	entry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
	entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	entry.polarity = APIC_IOAPIC_POLARITY_HIGH;
	entry.irr = APIC_IOAPIC_IRR_RESET;
	entry.trigger = APIC_ICR_IOAPIC_Edge;
	entry.mask = APIC_ICR_IOAPIC_Masked;
	entry.reserved = 0;

	entry.destination.physical.reserved1 = 0;
	entry.destination.physical.phy_dest = 0;
	entry.destination.physical.reserved2 = 0;

	
	
	color_printk(RED,BLACK,"HPET - GCAP_ID:<%#018lx>\n",*(unsigned long *)HPET_addr);

	*(unsigned long *)(HPET_addr + 0x10) = 3; //GEN_CONF
	io_mfence();
    
	//edge triggered & periodic
	*(unsigned long *)(HPET_addr + 0x100) = 0x004c; //TIM0_CONF
	io_mfence();
    
	//1S
	*(unsigned long *)(HPET_addr + 0x108) = 14318179; //TIM0_CMP
	io_mfence();
   
	//init MAIN_CNT & get CMOS time
	get_cmos_time(&time);
	*(unsigned long *)(HPET_addr + 0xf0) = 0;  // MAIN_CNT
    
	color_printk(RED,BLACK,"year%#010x,month:%#010x,day:%#010x,hour:%#010x,mintue:%#010x,second:%#010x\n",time.year,time.month,time.day,time.hour,time.minute,time.second);
    printFlagIrq = 1;
	register_irq(34, &entry , &HPET_handler, NULL, &HPET_int_controller, "HPET");
	io_mfence();
}