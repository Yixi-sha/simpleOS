#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef struct
{
    volatile unsigned long lock; 
}spinLock_T;

inline void spin_init(spinLock_T* lock);
inline void spin_lock(spinLock_T* lock);
inline void spin_unlock(spinLock_T* lock);

#endif