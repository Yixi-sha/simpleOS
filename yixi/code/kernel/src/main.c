#include "../inc/printk.h"
#include "../inc/gate.h"
#include "../inc/trap.h"
#include "../inc/cpu.h"
#include "../inc/interrupt.h"
#include "../inc/mm.h"
#include "../inc/printk.h"
#include "../inc/process.h"
#include "../inc/APIC.h"
#include "../inc/keyboard.h"
#include "../inc/disk.h"
#include "../inc/block.h"
//#include "../inc/8259A.h"
#include "../inc/SMP.h"

extern struct slabCache kmallocCacheSize[16];
extern struct Global_Memory_Descriptor mm_struct;
extern struct BolckDeviceOperation IDEDeviceOperation;
extern unsigned long _stack_start;





void Start_Kernel(void)
{
	/*unsigned char *addr = (unsigned char *)0xffff800000a00000;
	int i;


	for(i = 0 ;i<1024*20;i++)
	{
		*((unsigned char *)addr+0)=(unsigned char)0x00;
		*((unsigned char *)addr+1)=(unsigned char)0x00;	
		addr +=2;	
	}
	for(i = 0 ;i<1024*20;i++)
	{
		*((unsigned char *)addr+0)=(unsigned char)0xf8;
		*((unsigned char *)addr+1)=(unsigned char)0x00;	
		addr +=2;	
	}
	for(i = 0 ;i<1024*20;i++)
	{
		*((unsigned char *)addr+0)=(unsigned char)0x07;
		*((unsigned char *)addr+1)=(unsigned char)0xE0;	
		addr +=2;	
	}
	for(i = 0 ;i<1024*20;i++)
	{
		*((unsigned char *)addr+0)=(unsigned char)0x00;
		*((unsigned char *)addr+1)=(unsigned char)0x1f;	
		addr +=2;	
	}
	for(i = 0 ;i<1024*20;i++)
	{
		*((unsigned char *)addr+0)=(unsigned char)0xff;
		*((unsigned char *)addr+1)=(unsigned char)0xff;	
		addr +=2;	
	}*/
	printkInit();
	printk("start..... %d\n",sizeof(unsigned int));
	void* tmp = 0;
	struct slab *pool = NULL;
	struct Page_Magement* page = 0; 
	unsigned char x;
	int i = 0;
	printkInit();
	printk("start..... %d\n",sizeof(unsigned int));
	trap_init();
	init_cpu();
	sys_vector_init();
	init_memory();
	slab_init();
	printk("init middle \n");
	frame_buffer_init();
	pagetable_init();
	APIC_IOAPIC_init();
	keyboard_init();
	//mouse_init();
	color_printk(RED,BLACK,"disk init \n");
	disk_init();
	color_printk(RED,BLACK,"disk init end\n");
	//task_init();
	SMP_init();
	
    while(1){
		if(!keyboard_exit_data())
        	analysis_keycode();
		//if(!mouse_exit_data())
        //	analysis_mousecode();
    }
    
}