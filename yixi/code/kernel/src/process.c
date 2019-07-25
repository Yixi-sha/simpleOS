#include "../inc/process.h"
#include "../inc/gate.h"
#include "../inc/lib.h"
#include "../inc/printk.h"
#include "../inc/mm.h"

extern char _data;
extern char _rodata;
extern char _erodata;
struct memory_struct init_mm;
struct thread_struct init_thread;
extern unsigned long _stack_start;

union task_union init_task_union __attribute__((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};

struct task_struct *init_task[NR_CPUS] = {&init_task_union.task, 0};
extern unsigned long* GlobalCR3;
extern struct Global_Memory_Descriptor mm_struct;
extern void kernel_thread_func(void);
extern void system_call(void);
extern void ret_system_call();
__asm__(
    "kernel_thread_func:   \n\t"
    GET_CURRENT
    "addq $32768, %rbx \n\t"
    "subq $192, %rbx \n\t"
    "movq %rbx, %rsp \n\t"
    "popq %r15	\n\t"
    "popq %r14	\n\t"	
    "popq %r13	\n\t"	
    "popq %r12	\n\t"	
    "popq %r11	\n\t"	
    "popq %r10	\n\t"	
    "popq %r9	    \n\t"	
    "popq %r8	    \n\t"	
    "popq %rbx	\n\t"	
    "popq %rcx	\n\t"
    "popq %rdx	\n\t"	
    "popq %rsi	\n\t"	
    "popq %rdi	\n\t"	
    "popq %rbp	\n\t"	
    "popq %rax	\n\t"
    "movq %rax,	%ds	\n\t"
    "popq %rax		\n\t"
    "movq %rax,	%es	\n\t"
    "popq	%rax		\n\t"
    "addq	$0x38,	%rsp	\n\t"
/////////////////////////////////
    "movq	%rdx,	%rdi	\n\t"
    "callq	*%rbx		\n\t"
    "movq	%rax,	%rdi	\n\t"
    "callq	do_exit		\n\t"
);

struct thread_struct init_thread = 
{
    .rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
    .rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
    .fs = KERNEL_DS,
    .gs = KERNEL_DS,
    .cr2 = 0,
    .trap_nr = 0,
    .error_code = 0
};

struct tss_struct init_tss[NR_CPUS] = { [0 ... NR_CPUS-1] = INIT_TSS };

inline struct task_struct* get_current()
{
    struct task_struct* ret = NULL;
    __asm__ __volatile__(
        "andq %%rsp ,%0 \n\t"
        :"=r"(ret)
        :"0"(~32767UL)
    );
    return ret;
}

inline void __switch_to(struct task_struct *prev,struct task_struct *next)
{

	init_tss[0].rsp0 = next->thread->rsp0;

	//set_tss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	__asm__ __volatile__("movq	%%fs,	%0 \n\t":"=a"(prev->thread->fs));
	__asm__ __volatile__("movq	%%gs,	%0 \n\t":"=a"(prev->thread->gs));

	__asm__ __volatile__("movq	%0,	%%fs \n\t"::"a"(next->thread->fs));
	__asm__ __volatile__("movq	%0,	%%gs \n\t"::"a"(next->thread->gs));

	color_printk(WHITE,BLACK,"prev->thread->rsp0:%#018lx\n",prev->thread->rsp0);
	color_printk(WHITE,BLACK,"next->thread->rsp0:%#018lx\n",next->thread->rsp0);
}

void task_init()
{
    struct task_struct* task = NULL;
    init_mm.pgd = (pml4t_t*)GlobalCR3;
    init_mm.codeStart = mm_struct.codeStart;
    init_mm.codeEnd = mm_struct.codeEnd;
    init_mm.dataStart = (unsigned long)(&_data);
    init_mm.dataEnd = mm_struct.dataEnd;
    init_mm.rodataStart = (unsigned long)(&_rodata);
    init_mm.rodataEnd = (unsigned long)(&_erodata);
    init_mm.brkStart = 0;
    init_mm.brkEnd = mm_struct.brkEnd;
    init_mm.stackStart = _stack_start;

    //set_tss64(,init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);
    list_init(&init_task_union.task.list);
    wrmsr(0x174, KERNEL_CS);
    wrmsr(0x175, current->thread->rsp0);
    wrmsr(0x176, (unsigned long)system_call);
    kernel_thread(init,10,CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
    init_task_union.task.state = TASK_RUNNING;
    task = container_of(list_next(&current->list),struct task_struct,list);

	switch_to(current,task);

}

unsigned long init(unsigned long arg)
{
    struct pt_regs* regs = NULL;
	color_printk(RED,BLACK,"init task is running,arg:%#018lx\n",arg);
    color_printk(RED,BLACK,"sizeof is %d\n",sizeof(struct pt_regs));

    current->thread->rip = (unsigned long)ret_system_call;
    current->thread->rsp = (unsigned long)current + STACK_SIZE - sizeof(struct pt_regs);
    regs = (struct pt_regs*)current->thread->rsp;

    __asm__  __volatile__(
        "movq %1, %%rsp  \n\t"
        "pushq %2   \n\t"
        "jmp do_execve \n\t"
        :
        :"D"(regs),"m"(current->thread->rsp),"m"(current->thread->rip)
        :"memory"
    );
    
    
	return 1;
}

void userFunc()
{
    long ret = 0;
    char string[] = "hello,yixi\n";
    color_printk(RED,BLACK,"this is user func \n");
    __asm__  __volatile__(
        "pushq %%rdx \n\t"
        "pushq %%rcx \n\t"
        "leaq sysexit_return(%%rip), %%rdx  \n\t"
        "movq %%rsp, %%rcx   \n\t"
        "sysenter            \n\t"
        "sysexit_return:      \n\t"
        "popq %%rcx \n\t"
        "popq %%rdx \n\t"
        :"=a"(ret)
        :"0"(1),"D"(string)
        :"memory"
    );
    color_printk(RED,BLACK,"system call return \n");
    while(1);
}

unsigned long do_execve(struct pt_regs* regs)
{
    regs->rdx = 0x800000;
    regs->rcx = 0xa00000;

    regs->rax = 1;
    regs->ds = 0;
    regs->es = 0;
    color_printk(RED,BLACK,"this is user do_execve\n");
    memcpy(userFunc, (void*)0x800000, 1024);

    return 0;

}


int kernel_thread(unsigned long (*fn)(unsigned long), unsigned long args, unsigned long flag)
{
    struct pt_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rbx = (unsigned long)fn;
    regs.rdx = (unsigned long)args;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;

    regs.rflags = (1 << 9);
    regs.rip = (unsigned long)kernel_thread_func;
    
    return do_fork(&regs,flag, 0, 0);
}

unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
    struct task_struct* tsk = NULL;
    struct thread_struct* thd = NULL;
    struct Page_Magement* p = NULL;
    // alloc memory
    p = allocPage(PHY_NORMAL, 1, PG_PTable_Maped| PG_Active | PG_Kernel);
    tsk = (struct task_struct*)Phy_To_Virt(p->start);
    printk("tsk is %p \n", tsk);
    memset(tsk, 0, sizeof(*tsk));
    //initialize tsk
    *tsk = *current;
    list_init(&tsk->list);
    list_add_to_before(&init_task_union.task.list, &tsk->list);
    tsk->pid++;
    tsk->state = TASK_UNINTERRUPTIBLE;

    thd = (struct thread_struct*)(tsk + 1);
    tsk->thread = thd;

    memcpy(regs,(void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs)),sizeof(struct pt_regs));

    thd->rsp0 = (unsigned long)tsk + STACK_SIZE;
    thd->rip = regs->rip;
    thd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs);
    thd->fs = KERNEL_DS;
    thd->gs = KERNEL_DS;

    if(!((tsk->flag) & PF_KTHREAD))
        thd->rip = regs->rip = (unsigned long)ret_system_call;
    
    tsk->state = TASK_RUNNING;

    return 0; 
}

unsigned long do_exit(unsigned long code)
{
    struct task_struct* task = &(init_task_union.task);
	color_printk(RED,BLACK,"exit task is running,arg:%#018lx\n",code);
    switch_to(current,task);
	while(1);
}

systemCallF system_call_table[MAX_SYSTEM_CALL_NR] = 
{
    [0] = noSystemCall,
    [1] = sys_printf,
	[2 ... MAX_SYSTEM_CALL_NR-1] = noSystemCall
};

unsigned long systemCallFunc(struct pt_regs* regs)
{
    return system_call_table[regs->rax](regs);
}

unsigned long noSystemCall(struct pt_regs* regs)
{
    color_printk(RED,BLACK,"no noSystem Call of number is %d\n",regs->rax);
    return -1;
}

unsigned long sys_printf(struct pt_regs* regs)
{
    printk((char*)regs->rdi);
    return 0;
}
