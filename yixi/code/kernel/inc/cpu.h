#ifndef __CPU_H__

#define __CPU_H__


#define NR_CPUS 8

inline void get_cpuid(unsigned int Mop,unsigned int Sop,unsigned int * a,unsigned int * b,unsigned int * c,unsigned int * d);
void init_cpu(void);
#endif