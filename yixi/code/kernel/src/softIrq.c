#include "../inc/softIrq.h"
#include "../inc/mm.h"

unsigned long softIrqStatus = 0;

struct softIrq softIrqVector[64] = {0};


void set_softIrq_status(unsigned long status)
{
    softIrqStatus |= status;
}

unsigned long get_softIrq_status()
{
    return softIrqStatus;
}

void register_softIrq(int nr, void (*action)(void* data), void* data)
{
    softIrqVector[nr].m_action = action;
    softIrqVector[nr].m_data = data;
}

void unregister_softIrq(int nr)
{
    softIrqVector[nr].m_action = 0;
    softIrqVector[nr].m_data = 0;
}

void softIrq_init()
{
    softIrqStatus = 0;
    memset(softIrqVector, 0, sizeof(struct softIrq) * 64);
}

void do_softIrq()
{
    int i;
    sti();
    for( i = 0; i < 64 && softIrqStatus; i++)
    {
        if(softIrqStatus & (1 << i))
        {
            softIrqVector[i].m_action(softIrqVector[i].m_data);
            softIrqStatus &= ~(1 << i);
        }
    }
    cli();
}
