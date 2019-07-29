#ifndef __SOFTIRQ_H__
#define __SOFTIRQ_H__

#define TIMER_SIRQ  (1 << 0)

struct softIrq 
{
    void (*m_action)(void* data);
    void* m_data;
};

void set_softIrq_status(unsigned long status);
unsigned long get_softIrq_status();
void register_softIrq(int nr, void (*action)(void* data), void* data);
void unregister_softIrq(int nr);
void softIrq_init();
void do_softIrq();

#endif