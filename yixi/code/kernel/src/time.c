#include "../inc/time.h"
#include "../inc/lib.h"
#include "../inc/printk.h"
#include "../inc/softIrq.h"
#include "../inc/mm.h"

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


struct timerList* timerListHead = NULL;

void do_timer()
{
	struct timerList* tmp = timerListHead;
	
	while(tmp != NULL && tmp->m_expierJiffies <= jiffies)
	{
		del_timer(tmp);
		tmp->m_func(tmp->m_data);
		tmp = timerListHead;
	}
}

test_timer(void* data)
{
	color_printk(BLUE, BLACK, "this is test_timer\n");
}

void timer_init()
{
	struct timerList* tmp = NULL;
	jiffies = 0;
	register_softIrq(0, &do_timer, NULL);
	timerListHead = NULL;
		
	tmp = (struct timerList*)kmalloc(sizeof(struct timerList), 0);
	init_sys_timer(tmp, test_timer, NULL, 5);
	add_sys_timer(tmp);
	

}	

void init_sys_timer(struct timerList* timer, void (*func)(void* data),\
	void* data, unsigned long expierJiffies) 
{
	list_init(&timer->m_list);
	timer->m_func = func;
	timer->m_data = data;
	timer->m_expierJiffies = expierJiffies;
}

void add_sys_timer(struct timerList* timer)
{
	int i = 0;
	if(timer == NULL)
		return;

	if(timerListHead == NULL)
	{
		timerListHead = timer;
		list_init(&timerListHead->m_list);
	}
	else
	{
		struct timerList* tmp = timerListHead;
		while(tmp->m_expierJiffies < timer->m_expierJiffies )
		{
			i++;
			tmp = container_of(list_next(&tmp->m_list), struct timerList, m_list);
			if(tmp == timerListHead)
			{
				list_add_to_before(&tmp->m_list, &timer->m_list);
				return;
			}
		}
		list_add_to_behind(&tmp->m_list, &timer->m_list);
		if(tmp == timerListHead)
		{
			timerListHead = timer;
		}
	}
}


void del_timer(struct timerList* timer)
{
	if(timer != NULL)
	{	
		
		struct timerList* next = container_of(list_next(&timer->m_list), struct timerList, m_list);
		struct timerList* prev = container_of(list_prev(&timer->m_list), struct timerList, m_list);
		if(next == timer && prev == timer)
		{
			timerListHead = NULL;
		}
		else if(timer == timerListHead)
		{
			timerListHead = next;
		}
		list_del(&timer->m_list);
	}	
}


