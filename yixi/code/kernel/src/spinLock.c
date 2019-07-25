#include "../inc/spinLock.h"

inline void spin_init(spinLock_T* lock)
{
    lock->lock = 1;
}

inline void spin_lock(spinLock_T* lock)
{
    __asm__ __volatile__(
        "1:    \n\t"
        "lock   decq    %0    \n\t"
        "jns    3f    \n\t"
        "2:    \n\t"
        "pause    \n\t"
        "cmpq   $0, %0    \n\t"
        "jle    2b    \n\t"
        "jmp    1b    \n\t"
        "3:    \n\t"
        :"=m"(lock->lock)
        :
        :"memory"
    );
}

inline void spin_unlock(spinLock_T* lock)
{
    __asm__ __volatile__(
        "movq   $1, %0    \n\t"
        :"=m"(lock->lock)
        :
        :"memory"
    );
}