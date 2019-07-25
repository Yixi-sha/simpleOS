#include "../inc/SMP.h"
#include "../inc/cpu.h"
#include "../inc/printk.h"
#include "../inc/lib.h"
#include "../inc/process.h"
#include "../inc/gate.h"
#include "../inc/APIC.h"
#include "../inc/spinLock.h"
#include "../inc/mm.h"
#include "../inc/interrupt.h"

extern unsigned char _APU_boot_start[];
extern unsigned char _APU_boot_end[];
extern unsigned long _stack_start;

int global_i = 0;
spinLock_T SMPLock;
struct INT_CMD_REG icr_entry;
unsigned int * tss = NULL;



extern void (* SMP_interrupt[10])(void);
extern irq_desc_T SMP_IPI_desc[10] = {0};
void SMP_init()
{
    int i = 0;
    unsigned int a = 0, b = 0, c = 0, d = 0;

    for(i = 200; i < 210; i++)
	{
		set_intr_gate(i, 2, SMP_interrupt[i - 200]);
	}
	memset(SMP_IPI_desc, 0, sizeof(irq_desc_T) * 10);

    for(i = 0; ; i++)
    {
        get_cpuid(0x0b, i, &a, &b, &c, &d);
        if((c >> 8 & 0xff) == 0 )
            break;
        color_printk(WHITE,BLACK,"local APIC ID Package_../Core_2/SMT_1,type(%x) Width:%#010x,num of logical processor(%x)\n",c >> 8 & 0xff,a & 0x1f,b & 0xff);
    }
    color_printk(WHITE,BLACK,"x2APIC ID level:%#010x\tx2APIC ID the current logical processor:%#010x\n",c & 0xff,d);


	color_printk(WHITE,BLACK,"SMP copy byte:%#010x\n",(unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);
	memcpy(_APU_boot_start,(unsigned char *)0xffff800000020000,(unsigned long)&_APU_boot_end - (unsigned long)&_APU_boot_start);

	icr_entry.vector = 0x00;
	icr_entry.deliver_mode =  APIC_ICR_IOAPIC_INIT;
	icr_entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	icr_entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	icr_entry.res_1 = 0;
	icr_entry.level = ICR_LEVEL_DE_ASSERT;
	icr_entry.trigger = APIC_ICR_IOAPIC_Edge;
	icr_entry.res_2 = 0;
	icr_entry.dest_shorthand = ICR_ALL_EXCLUDE_Self;
	icr_entry.res_3 = 0;
	icr_entry.destination.x2apic_destination = 0x00;

	wrmsr(0x830,*(unsigned long *)&icr_entry);	//INIT IPI
	spin_init(&SMPLock);
	for(global_i = 1;global_i < 4;global_i++)
	{
		spin_lock(&SMPLock);
		
		_stack_start = (unsigned long)kmalloc(STACK_SIZE,0) + STACK_SIZE;
		tss = (unsigned int *)kmalloc(128,0);
		set_tss_descriptor(10 + global_i * 2,tss);
		set_tss64(tss,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start,_stack_start);
	
		icr_entry.vector = 0x20;
		icr_entry.deliver_mode = ICR_Start_up;
		icr_entry.dest_shorthand = ICR_No_Shorthand;
		icr_entry.destination.x2apic_destination = global_i;
	
		wrmsr(0x830,*(unsigned long *)&icr_entry);	//Start-up IPI
		wrmsr(0x830,*(unsigned long *)&icr_entry);	//Start-up IPI
	}
	printk("send\n");
	icr_entry.vector = 0xc8;
	icr_entry.destination.x2apic_destination = 1;
	icr_entry.deliver_mode = APIC_ICR_IOAPIC_Fixed;

	wrmsr(0x830,*(unsigned long *)&icr_entry);

	icr_entry.vector = 0xc9;
	wrmsr(0x830,*(unsigned long *)&icr_entry);

	
}

void Start_SMP()
{
	unsigned int x,y;

	color_printk(RED,YELLOW,"APU starting......\n");


	//enable xAPIC & x2APIC
	__asm__ __volatile__(	"movq 	$0x1b,	%%rcx	\n\t"
				"rdmsr	\n\t"
				"bts	$10,	%%rax	\n\t"
				"bts	$11,	%%rax	\n\t"
				"wrmsr	\n\t"
				"movq 	$0x1b,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y)
				:
				:"memory");
	
//	if(x&0xc00)
//		color_printk(RED,YELLOW,"xAPIC & x2APIC enabled\n");

	//enable SVR[8]
	__asm__ __volatile__(	"movq 	$0x80f,	%%rcx	\n\t"
				"rdmsr	\n\t"
				"bts	$8,	%%rax	\n\t"
//				"bts	$12,	%%rax\n\t"
				"wrmsr	\n\t"
				"movq 	$0x80f,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y)
				:
				:"memory");

//	if(x&0x100)
//		color_printk(RED,YELLOW,"SVR[8] enabled\n");
//	if(x&0x1000)
//		color_printk(RED,YELLOW,"SVR[12] enabled\n");

	//get local APIC ID
	__asm__ __volatile__(	"movq $0x802,	%%rcx	\n\t"
				"rdmsr	\n\t"
				:"=a"(x),"=d"(y)
				:
				:"memory");
	
	color_printk(RED,YELLOW,"x2APIC ID:%#010x\n",x);
	// /color_printk(RED,YELLOW,"global is %d\n",global_i);
	load_TR(10 + (global_i -1)* 2);
	spin_unlock(&SMPLock);
	sti();
	
	while(1)
		hlt();
}