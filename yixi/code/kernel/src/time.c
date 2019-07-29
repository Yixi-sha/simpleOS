#include "../inc/time.h"
#include "../inc/lib.h"
#include "../inc/printk.h"
#include "../inc/softIrq.h"

struct time time;
extern volatile unsigned long jiffies;

int get_cmos_time(struct time *time)
{
	cli();

	do
	{	time->year =   CMOS_READ(0x09) + CMOS_READ(0x32) * 0x100;
		time->month =  CMOS_READ(0x08);
		time->day =    CMOS_READ(0x07);	
		time->hour =   CMOS_READ(0x04);	
		time->minute = CMOS_READ(0x02);
		time->second = CMOS_READ(0x00);
	}while(time->second != CMOS_READ(0x00));
	
	io_out8(0x70,0x00); 

	sti();
}

void do_timer()
{
	color_printk(RED, BLACK, "(HPET:%ld)",jiffies);
}

void timer_init()
{
	jiffies = 0;
	register_softIrq(0, &do_timer, NULL);
}