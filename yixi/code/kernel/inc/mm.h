#ifndef __MM_H__
#define __MM_H__

#include "lib.h"

#define PAGE_OFFSET	((unsigned long)0xffff800000000000)

#define PAGE_GDT_SHIFT	39
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK) //boder
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

#define Virt_To_2M_Page(kaddr)	(mm_struct.page + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(mm_struct.page + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))


///struct page attribute (alloc_pages flags)

//
#define PG_PTable_Maped	(1 << 0)

//
#define PG_Kernel_Init	(1 << 1)

//
#define PG_Referenced	(1 << 2)

//
#define PG_Dirty	(1 << 3)

//
#define PG_Active	(1 << 4)

//
#define PG_Up_To_Date	(1 << 5)

//
#define PG_Device	(1 << 6)

//
#define PG_Kernel	(1 << 7)

//
#define PG_K_Share_To_U	(1 << 8)

//
#define PG_Slab		(1 << 9)

struct E820
{
    unsigned long address;
    unsigned long length;
    unsigned int type; 
}__attribute__((packed));

struct Global_Memory_Descriptor
{
    struct E820 e820[32];
    unsigned long length;

    struct Page_Magement* page;
    unsigned long pageNum;
    unsigned long PageStrLength;

    struct PHY_Memory* PHY;
    unsigned long PHYNum;
    unsigned long PHYStrLength;

    unsigned long *bitMap;
    unsigned long bitNum;
    unsigned long bitLength;

    unsigned long codeStart,codeEnd,dataEnd,brkEnd;

    unsigned long strEnd;
};

struct Page_Magement
{
    struct PHY_Memory* phy;
    unsigned long start;
    unsigned long attribute;

    unsigned long refrencecount;
    unsigned long creationTime;
};


struct PHY_Memory
{
    struct Global_Memory_Descriptor* glabol;
    unsigned long start;
    unsigned long end;
    unsigned long length;
    unsigned long attribute;

    struct Page_Magement* page;
    unsigned long pageNum;
    unsigned long pageUsedNum;
    unsigned long PageFreeNum;
    unsigned long pageTotalLinkNum;
};

#define updateTlb() \
do \
{ \
    unsigned long tmpreg;         \
    __asm__ __volatile__(          \
        "movq %%cr3, %0   \n\t"   \
        "movq %0, %%cr3   \n\t"   \
        :"=r"(tmpreg)             \
        :                         \
        :"memory"                 \
    );                            \
}while(0)

#define PHY_DMA	(1 << 0)

//
#define PHY_NORMAL	(1 << 1)

//
#define PHY_UNMAPED	(1 << 2)

typedef struct {
    unsigned long pml4t;
    } pml4t_t;

inline unsigned long * getGdt();
void init_memory();
void page_init(struct Page_Magement* page, unsigned long flag);
struct Page_Magement* allocPage(int PHYSelect, int number, unsigned long flag);
unsigned long page_clean(struct Page_Magement * page);
void free_pages(struct Page_Magement * page,int number);
unsigned long get_page_atttibute(struct Page_Magement* page);
long set_page_atttibute(struct Page_Magement* page, unsigned long flag);

/*slab*/
#define SIZEOF_LONG_ALIGN(size) ((size + sizeof(long) - 1) & ~(sizeof(long) - 1) )
#define SIZEOF_INT_ALIGN(size) ((size + sizeof(int) - 1) & ~(sizeof(int) - 1) )

struct slab
{
    struct List list;
    struct Page_Magement* page;

    unsigned long usedCount;
    unsigned long freeCount;

    void* Vaddress;

    unsigned long colorLength;
    unsigned long colorCount;

    unsigned long* colorMap;
};

struct slabCache
{
    unsigned long size;
    unsigned long totalUsed;
    unsigned long totalFree;
    struct slab* cachePool;

    struct slab* dmaPool;

    void* (*constructor)(void* Vaddress, unsigned long arg);
    void* (*destructor)(void* Vaddress, unsigned long arg);
};

struct slabCache* slabCacheCreate(unsigned long size, void* (*constructor)(void* Vaddress, unsigned long arg), \
void* (*destructor)(void* Vaddress, unsigned long arg), unsigned long arg);
void* slabMalloc(struct slabCache* cache, unsigned long arg);
long slabDestroy(struct slabCache* obj);
long slab_free(struct slabCache* cache, void* address, unsigned long arg);
void* kmalloc(unsigned long size, unsigned long flag);
long kfree(void* address);
struct slab* kmallocCreate(unsigned size);
long slab_init();


//	bit 63	Execution Disable:
#define PAGE_XD		(1UL << 63)

//	bit 12	Page Attribute Table
#define	PAGE_PAT	(1UL << 12)

//	bit 8	Global Page:1,global;0,part
#define	PAGE_Global	(1UL << 8)

//	bit 7	Page Size:1,big page;0,small page;
#define	PAGE_PS		(1UL << 7)

//	bit 6	Dirty:1,dirty;0,clean;
#define	PAGE_Dirty	(1UL << 6)

//	bit 5	Accessed:1,visited;0,unvisited;
#define	PAGE_Accessed	(1UL << 5)

//	bit 4	Page Level Cache Disable
#define PAGE_PCD	(1UL << 4)

//	bit 3	Page Level Write Through
#define PAGE_PWT	(1UL << 3)

//	bit 2	User Supervisor:1,user and supervisor;0,supervisor;
#define	PAGE_U_S	(1UL << 2)

//	bit 1	Read Write:1,read and write;0,read;
#define	PAGE_R_W	(1UL << 1)

//	bit 0	Present:1,present;0,no present;
#define	PAGE_Present	(1UL << 0)

//1,0
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)

//1,0	
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)

//7,1,0
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)

//2,1,0
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)

//7,2,1,0
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)

#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))

typedef struct 
{
    unsigned long pdpt;
} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))

typedef struct 
{
    unsigned long pdt;
} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))

typedef struct 
{
    unsigned long pt;
} pt_t;
#define mk_pt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr,ptval)		(*(ptptr) = (ptval))
void pagetable_init();

#endif