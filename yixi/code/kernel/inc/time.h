#ifndef __TIME_H__

#define __TIME_H__

struct time
{
	int second;	//00
	int minute;	//02
	int hour;	//04
	int day;	//07
	int month;	//08
	int year;	//09+32
};

#define CMOS_READ(addr) ({ \
io_out8(0x70,0x80 | addr); \
io_in8(0x71); \
})

#define	BCD2BIN(value)	(((value) & 0xf) + ((value) >> 4 ) * 10)

int get_cmos_time(struct time *time);
void do_timer();
void timer_init();

#endif