#include "../inc/mm.h"
#include "../inc/lib.h"
#include "../inc/printk.h"

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;
struct Global_Memory_Descriptor mm_struct = {{0}, 0};

int PHY_DMA_INDEX	= 0;
int PHY_NORMAL_INDEX	= 0;	//low 1GB RAM ,was mapped in pagetable
int PHY_UNMAPED_INDEX	= 0;	//above 1GB RAM,unmapped in pagetable
unsigned long* GlobalCR3 = NULL;

void init_memory()
{
    struct E820 *p = NULL;
    unsigned long TotalMem = 0 ;
    int i = 0, j =0;
    color_printk(BLUE,BLACK,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
    p = (struct E820*)0xffff800000007e00;
    for(i = 0;i < 32;i++)
	{
        color_printk(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
        mm_struct.e820[i].address = p->address;
        mm_struct.e820[i].length  = p->length;
        mm_struct.e820[i].type = p->type;
		unsigned long tmp = 0;
		if(p->type == 1)
		{
			TotalMem += p->length;
		}
        mm_struct.length = i;
		p++;
		if(p->type > 4 || p->length == 0 || p->type < 1)
			break;			
	}
    color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%p\n",TotalMem); 
    PHY_DMA_INDEX	= 0;
    PHY_NORMAL_INDEX	= 0;	//low 1GB RAM ,was mapped in pagetable
    PHY_UNMAPED_INDEX	= 0;	//above 1GB RAM,unmapped in pagetable  
    

    TotalMem = mm_struct.e820[mm_struct.length].address + mm_struct.e820[mm_struct.length].length;
    mm_struct.codeStart = (unsigned long)(&_text);
    mm_struct.codeEnd = (unsigned long)(&_etext);
    mm_struct.dataEnd = (unsigned long)(&_edata);
    mm_struct.brkEnd = (unsigned long)(&_end);
    

    mm_struct.bitMap = (unsigned long*)((mm_struct.brkEnd + PAGE_4K_SIZE - 1) & PAGE_4K_MASK); // insulate with kernel
    mm_struct.bitNum = TotalMem >> PAGE_2M_SHIFT;
    mm_struct.bitLength =(((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & ( ~ (sizeof(long) - 1)); // boder byte allign
    printk("mm_struct.bitMap is %p mm_struct.bitLength is %d\n",mm_struct.bitMap,mm_struct.bitLength);
    memset(mm_struct.bitMap,0xff,mm_struct.bitLength);

    mm_struct.page = (struct Page_Magement*)(((((unsigned long)mm_struct.bitMap) + mm_struct.bitLength) + PAGE_4K_SIZE - 1) &  PAGE_4K_MASK); 
    mm_struct.pageNum = TotalMem >> PAGE_2M_SHIFT;
    mm_struct.PageStrLength = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page_Magement) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1)); // boder byte allign
    printk("mm_struct.page is %p mm_struct.PageStrLength is %d\n",mm_struct.page, mm_struct.PageStrLength);
    memset(mm_struct.page, 0x00, mm_struct.PageStrLength);

    mm_struct.PHY = (struct PHY_Memory*)(((unsigned long)mm_struct.page + mm_struct.PageStrLength + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
    mm_struct.PHYNum = 0;
    mm_struct.PHYStrLength = ((sizeof(struct PHY_Memory) * 5) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1));
    memset(mm_struct.PHY,0x00,mm_struct.PHYStrLength);

    TotalMem = 0;

    for( i = 0 ;  i < mm_struct.length; i++)
    {
        struct PHY_Memory* PHY = NULL;
        struct Page_Magement* page = NULL;
        unsigned start = 0, end = 0;
        if(mm_struct.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(mm_struct.e820[i].address);
        end = ((mm_struct.e820[i].address + mm_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if(end <= start)
            continue;
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
        PHY  = mm_struct.PHY + mm_struct.PHYNum;
        mm_struct.PHYNum++;
        PHY->glabol =  &mm_struct;
        PHY->start = start;
        PHY->end = end;
        PHY->length = end - start;
        PHY->attribute = 0;
        PHY->page = mm_struct.page + (start >> PAGE_2M_SHIFT);
        PHY->pageNum = (end - start) >> PAGE_2M_SHIFT;
        PHY->pageUsedNum = 0;
        PHY->PageFreeNum = PHY->pageNum;
        PHY->pageTotalLinkNum = 0;
        
        page = PHY->page;
        for(j = 0; j < PHY->pageNum; j++, page++)
        {
            page->phy = PHY;
            page->start = PHY->start + j * PAGE_2M_SIZE;
            page->attribute = 0;
            page->refrencecount = 0;
            page->creationTime =0;
            
            *(mm_struct.bitMap + ((page->start >> PAGE_2M_SHIFT) >> 6)) ^= (1UL << (page->start >> PAGE_2M_SHIFT) % 64);
        }
    }
    mm_struct.page->phy = mm_struct.PHY;  // low of 2M
    mm_struct.page->start = 0UL;
    mm_struct.page->attribute = 0;
    mm_struct.page->refrencecount = 0;
    mm_struct.page->creationTime = 0;

    mm_struct.PHYStrLength = ((sizeof(struct PHY_Memory) * mm_struct.PHYNum) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1));
    printk("mm_struct.PHY is %p mm_struct.PHYNum is %d mm_struct.PHYStrLength is %d\n",mm_struct.PHY, mm_struct.PHYNum, mm_struct.PHYStrLength);

    for(i = 0 ; i < mm_struct.PHYNum; i++)
    {
        struct PHY_Memory* PHY = mm_struct.PHY + i;
        color_printk(ORANGE,BLACK,"phy start is %p page is %p page num is %d\n",PHY->start, PHY->page, PHY->pageNum);
        if(PHY->start == 0x100000000)
            PHY_UNMAPED_INDEX = i;
    }
    printk("PHY_UNMAPED_INDEX is %d \n", PHY_UNMAPED_INDEX);
    mm_struct.strEnd = ((unsigned long)mm_struct.PHY + mm_struct.PHYStrLength + sizeof(long) * 32) & (~(sizeof(long) - 1 ));
   
    color_printk(ORANGE,BLACK,"OS start address is %#018lx, end address is %#018lx mm_struct end is %p\n" ,mm_struct.codeStart, mm_struct.brkEnd,mm_struct.strEnd);

    i = Virt_To_Phy(mm_struct.strEnd) >> PAGE_2M_SHIFT;
    for(j = 0; j <= i; j++)
    {
        struct Page_Magement* tmpPage = mm_struct.page + j;
        page_init(tmpPage, PG_PTable_Maped|PG_Kernel_Init|PG_Active|PG_Kernel);
        tmpPage->phy->pageUsedNum++;
        tmpPage->phy->PageFreeNum--;
        *(mm_struct.bitMap + ((tmpPage->start >> PAGE_2M_SHIFT) >> 6)) |= 1UL << ((tmpPage->start >> PAGE_2M_SHIFT) % 64);
    }

    GlobalCR3 = getGdt();
    color_printk(INDIGO,BLACK,"&mm_struct is\t:%p\n",&mm_struct);
    color_printk(INDIGO,BLACK,"Global_CR3\t:%p\n",GlobalCR3);
	color_printk(INDIGO,BLACK,"*Global_CR3\t:%p\n",*Phy_To_Virt(GlobalCR3) & (~0xff));
	color_printk(PURPLE,BLACK,"**Global_CR3\t:%p\n",*Phy_To_Virt(*Phy_To_Virt(GlobalCR3) & (~0xff)) & (~0xff));
    /* for(i = 0; i < 10; i++)
    {
        *(Phy_To_Virt(GlobalCR3) + i) = 0UL;
    }*/
    updateTlb();
    color_printk(INDIGO,BLACK,"&mm_struct is\t:%p\n",&mm_struct);

    color_printk(RED,BLACK,"1.mm_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",*mm_struct.bitMap, \
    mm_struct.PHY->pageUsedNum, mm_struct.PHY->PageFreeNum);
    
}   

void page_init(struct Page_Magement* page, unsigned long flag)
{
   page->attribute |= flag;
   
   if(!page->refrencecount || (page->attribute & PG_K_Share_To_U))
   {
       page->refrencecount++;
       page->phy->pageTotalLinkNum++;
   }
}

unsigned long get_page_atttibute(struct Page_Magement* page)
{
    if(page == NULL)
    {
        color_printk(RED, BLACK, "get_page_atttibute() error %s %d \n", __FILE__, __LINE__);
        return 0;
    }
    else
        return page->attribute;
}

long set_page_atttibute(struct Page_Magement* page, unsigned long flag)
{
    if(page == NULL)
    {
        color_printk(RED, BLACK, "set_page_atttibute() error %s %d \n", __FILE__, __LINE__);
        return -1;
    }
    else
    {
        page->attribute = flag;
        return 0;
    }
}

inline unsigned long * getGdt()
{
    unsigned long * ret;
    __asm__ __volatile__(
        "movq %%cr3, %0 \n\t"
        :"=r"(ret)
        :
        :"memory"
    );
    return ret;
}

// number <= 64
struct Page_Magement* allocPage(int PHYSelect, int number, unsigned long flag)
{
    struct Page_Magement* ret = NULL;
    unsigned int PHYStart = 0; // include
    unsigned int PHYEnd = 0;   // include 
    unsigned int i = 0;
    unsigned char loopFlag = 1;

    if(number <= 0 || number > 64)
    {
        color_printk(RED,BLACK,"PHYSelect error in  allocPage() %s %d\n",__FILE__, __LINE__);
        return NULL;
    }
    switch (PHYSelect)
    {
    case PHY_DMA:
        PHYStart = 0;
        PHYEnd = PHY_DMA_INDEX;
        if(flag == 0)
            flag = PG_PTable_Maped;
        break;

    case PHY_NORMAL:
        PHYStart = PHY_DMA_INDEX;
        PHYEnd = PHY_NORMAL_INDEX;
        if(flag == 0)
            flag = PG_PTable_Maped;
        break;
    
    case PHY_UNMAPED:
        PHYStart = PHY_NORMAL_INDEX;
        PHYEnd = mm_struct.PHYNum - 1;
        break;
    default:
        color_printk(RED,BLACK,"PHYSelect error in  allocPage() %s %d\n",__FILE__, __LINE__);
        return NULL;
        break;
    }
    for(i = PHYStart; (i <= PHYEnd) && (1 == loopFlag); i++)
    {
        struct PHY_Memory * phy = mm_struct.PHY + i;
        unsigned long start = 0, end = 0, j = 0;
        printk("loopFlag is %d phy->PageFreeNum is %d\n",loopFlag, phy->PageFreeNum);
        if(phy->PageFreeNum < number)
            continue;
        
        start = phy->start >> PAGE_2M_SHIFT;
        end = phy->end >> PAGE_2M_SHIFT;
        printk("start is %d end is %d \n",start,end);
        for(j = start; (j < end) && (1 == loopFlag); j++)
        {
            unsigned long num = (1UL << number) - 1;
            unsigned long* bitMap = mm_struct.bitMap + (j >> 6);
            unsigned long shift = j % 64;
            if(!((((*bitMap) >> shift) | (*(bitMap + 1) << (64 - shift))) &  \
            (64 == number ? 0xffffffffffffffffUL  : ((1UL << number) - 1))))
            {
                unsigned long l = 0;
                struct Page_Magement * page = mm_struct.page + j;
                ret = page;
                for(;l < number; l++, page++)
                {
                    phy->pageUsedNum++;
                    phy->PageFreeNum--;
                    *(mm_struct.bitMap + ((page->start >> PAGE_2M_SHIFT) >> 6)) |= 1UL << ((page->start >> PAGE_2M_SHIFT) % 64);
                    page_init(page, flag);
                }
                loopFlag = 0;
            }
        }
    }
    return ret;
}

unsigned long page_clean(struct Page_Magement * page)
{
    page->phy->pageTotalLinkNum--;
    page->refrencecount--;
	if(!page->refrencecount)
    {
        page->attribute &= PG_PTable_Maped;
    }
	return 0;
}

void free_pages(struct Page_Magement * page,int number)
{	
	int i = 0;
	
	if(page == NULL)
	{
		color_printk(RED,BLACK,"free_pages() ERROR: page is invalid\n");
		return ;
	}	

	if(number >= 64 || number <= 0)
	{
		color_printk(RED,BLACK,"free_pages() ERROR: number is invalid\n");
		return ;	
	}
	
	for(i = 0;i<number;i++,page++)
	{
		*(mm_struct.bitMap + ((page->start >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->start >> PAGE_2M_SHIFT) % 64);
		page->phy->pageUsedNum--;
		page->phy->PageFreeNum++;
		page->attribute = 0;
	}
}

// size size of unit of allocation
struct slabCache* slabCacheCreate(unsigned long size, void* (*constructor)(void* Vaddress, unsigned long arg), \
 void* (*destructor)(void* Vaddress, unsigned long arg), unsigned long arg)
{
    struct slabCache* ret = NULL;

    ret = (struct slabCache*)kmalloc(sizeof(struct slabCache), 0); //////////////////////////////////////

    if(NULL == ret)
    {
        printk("slabCacheCreate()==>kmalloc fail %s %d\n", __FILE__, __LINE__);
        return NULL;
    }

    memset(ret, 0, sizeof(struct slabCache));
    ret->size = SIZEOF_LONG_ALIGN(size);
    ret->totalUsed = 0;
    ret->totalFree = 0;
    ret->cachePool = (struct slab*)kmalloc(sizeof(struct slab), 0);
    if(NULL == ret->cachePool)
    {
        printk("slabCacheCreate()==>kmalloc fail %s %d\n", __FILE__, __LINE__);
        kfree(ret);
        return NULL;
    }
    memset(ret->cachePool, 0, sizeof(struct slab));
    ret->dmaPool = NULL;
    ret->constructor = constructor;
    ret->destructor = destructor;
    
    list_init(&ret->cachePool->list);

    ret->cachePool->page = allocPage(PHY_NORMAL, 1, PG_Kernel);
    if(NULL == ret->cachePool->page)
    {
        printk("slabCacheCreate()==>don`t have free page %s %d\n", __FILE__, __LINE__);
        kfree(ret->cachePool);
        kfree(ret);
        return NULL;
    }
    ret->cachePool->usedCount = 0;
    ret->cachePool->freeCount = PAGE_2M_SIZE / ret->size;

    ret->totalFree = ret->cachePool->freeCount;
    ret->cachePool->Vaddress = Phy_To_Virt(ret->cachePool->page->start);
    ret->cachePool->colorCount = ret->cachePool->freeCount;
    ret->cachePool->colorLength = ((ret->cachePool->colorCount + (sizeof(unsigned long) * 8 - 1)) >> 6) << 3; 
    ret->cachePool->colorMap = (unsigned long*)kmalloc(ret->cachePool->colorLength, 0);
    if(NULL == ret->cachePool->colorMap)
    {
        printk("slabCacheCreate()==>kmalloc(ret->cachePool->colorLength) fail  %s %d\n", __FILE__, __LINE__);
        free_pages(ret->cachePool->page, 1);
        kfree(ret->cachePool);
        kfree(ret);
        return NULL;
    }

    memset(ret->cachePool->colorMap, 0, ret->cachePool->colorLength);
    
    return ret;
}

long slabDestroy(struct slabCache* obj)
{
    struct slab* cachePool = obj->cachePool;
    if(0 != obj->totalUsed)
    {
        printk("slabCache totalused is not 0   %s %d\n", __FILE__, __LINE__);
        return -1;
    }

    while(!list_is_empty(&cachePool->list))
    {
        struct slab* mpool = cachePool;
        cachePool = container_of(list_next(&cachePool->list), struct slab, list);
        list_del(&mpool->list);
        kfree(mpool->colorMap);
        page_clean(mpool->page);
        free_pages(mpool->page, 1);
        kfree(mpool);
    }

    kfree(cachePool->colorMap);
    page_clean(cachePool->page);
    free_pages(cachePool->page, 1);

    kfree(cachePool);
    kfree(obj);

    return 0;
}

void* slabMalloc(struct slabCache* cache, unsigned long arg)
{
    void* ret = NULL;
    struct slab* tmp = NULL;
    if(0 == cache->totalFree)
    {
        int j =0;
        tmp = (struct slab*)kmalloc(sizeof(struct slab), 0);
        if(NULL == tmp)
        {
            printk("slabMalloc()====> kmalloc fail %s %d\n",__FILE__,__LINE__);
            return ret;
        }
        memset(tmp, 0, sizeof(struct slab));
        tmp->page = allocPage(PHY_NORMAL, 1, PG_Kernel);
        if(NULL == tmp->page)
        {
            printk("slabMalloc()====> allocPage fail %s %d\n",__FILE__,__LINE__);
            kfree(tmp);
            return ret;
        }
        tmp->usedCount = 0;
        tmp->freeCount = PAGE_2M_SIZE / cache->size;
        tmp->Vaddress = Phy_To_Virt(tmp->page->start); 
        tmp->colorLength = ((tmp->freeCount + sizeof(unsigned long) * 8 - 1) >> 6 ) << 3;
        tmp->colorMap = (unsigned long*)kmalloc(tmp->colorLength, 0);
        if(NULL == tmp->colorMap)
        {
            printk("slabMalloc()====> kmalloc fail %s %d\n",__FILE__,__LINE__);
            free_pages(tmp->page, 1);
            kfree(tmp);
            return ret;
        }
        tmp->colorCount = tmp->freeCount;
        memset(tmp->colorMap, 0, tmp->colorLength);
        list_init(&tmp->list);
        list_add_to_behind(&cache->cachePool->list, &tmp->list);
        cache->totalFree += tmp->colorCount;

        for(; j < tmp->colorCount;j++)
        {
            if(((*(tmp->colorMap + (j >> 6))) & (1UL << (j % 64))) == 0)
            {
                *(tmp->colorMap + (j >> 6)) |= 1UL << (j % 64);
                tmp->freeCount--;
                tmp->usedCount++;
                cache->totalFree--;
                cache->totalUsed--;
                if(cache->constructor != NULL)
                    ret = cache->constructor((char *)tmp->Vaddress + cache->size * j, arg);
                else
                    ret = (void *)((char *)tmp->Vaddress + cache->size * j);

                return ret;
            }
        }
    }
    else 
    {
        struct slab* pool = cache->cachePool;
        do
        {
            if(pool->freeCount != 0)
            {
                int j = 0;
                for(;j < pool->colorCount; j++)
                {
                    if(*(pool->colorMap + (j >> 6)) == 0xffffffffffffffffUL)
                    {
                        j += 63;
                        continue;
                    }
                    if((*(pool->colorMap + (j >> 6)) & (1UL << (j % 64))) == 0)
                    {
                        *(pool->colorMap + (j >> 6)) |= (1UL << (j % 64));
                        pool->usedCount++;
                        pool->freeCount--;

                        cache->totalUsed++;
                        cache->totalFree--;

                        if(cache->constructor != NULL)
                            ret = cache->constructor((char *)pool->Vaddress + cache->size * j, arg);
                        else
                            ret = (void *)((char *)pool->Vaddress + cache->size * j);
                        
                        return ret;
                    }
                }
            }

            pool = container_of(list_next(&pool->list), struct slab, list);
        }while((pool != cache->cachePool) && (ret != NULL)); 
    }
    printk("can not slabMalloc %s %d \n", __FILE__, __LINE__);
    if(tmp != NULL)
    {
        list_del(&tmp->list);
        if(tmp->colorMap != NULL)
            kfree(tmp->colorMap);
        if(tmp->page != NULL)
        {
            page_clean(tmp->page);
            free_pages(tmp->page, 1);
        }
        kfree(tmp);     
    }
    return ret;
}

long slab_free(struct slabCache* cache, void* address, unsigned long arg)
{
    struct slab* pool = cache->cachePool;
    int index = 0;
    do
    {
        if((pool->Vaddress <= address) && (address < (pool->Vaddress + PAGE_2M_SIZE)))
        {
            index = (address - pool->Vaddress) / cache->size;
            *(pool->colorMap + (index >> 6)) ^= 1UL << (index % 64);
            pool->freeCount++;
            pool->usedCount--;

            cache->totalFree++;
            cache->totalUsed--;

            if(NULL != cache->destructor)
            {
                cache->destructor((char*)pool->Vaddress + cache->size * index, arg);
            }
            if((0 == pool->usedCount) && (cache->totalFree >= (pool->colorCount * 3 /2)))
            {
                if( pool == cache->cachePool)
                {
                    cache->cachePool = container_of(list_next(&pool->list), struct slab, list);
                }
                cache->totalFree -= pool->colorCount;
                kfree(pool->colorMap);
                page_clean(pool->page);
                free_pages(pool->page, 1);
                kfree(pool);
            }
            return 0;
        }
        pool = container_of(list_next(&pool->list), struct slab, list);
    } while (pool != cache->cachePool);
    
    printk("slab_free() fail %s %d",__FILE__,__LINE__);
    return -1;
}

struct slabCache kmallocCacheSize[16] = 
{
	{32	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{64	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{128	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{256	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{512	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{1024	,0	,0	,NULL	,NULL	,NULL	,NULL},			//1KB
	{2048	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{4096	,0	,0	,NULL	,NULL	,NULL	,NULL},			//4KB
	{8192	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{16384	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{32768	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{65536	,0	,0	,NULL	,NULL	,NULL	,NULL},			//64KB
	{131072	,0	,0	,NULL	,NULL	,NULL	,NULL},			//128KB
	{262144	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{524288	,0	,0	,NULL	,NULL	,NULL	,NULL},
	{1048576,0	,0	,NULL	,NULL	,NULL	,NULL},			//1MB
};

long slab_init()
{
    int i = 0;
    unsigned long pageEnd = 0, initStrEnd = mm_struct.strEnd;
    struct Page_Magement* page =NULL;
    for(;i < 16; i++)
    {
        kmallocCacheSize[i].cachePool = (struct slab*)mm_struct.strEnd;
        mm_struct.strEnd = mm_struct.strEnd + sizeof(struct slab) + sizeof(unsigned long) * 10;

        ///////init slab
        list_init(&kmallocCacheSize[i].cachePool->list);
        kmallocCacheSize[i].cachePool->usedCount = 0;
        kmallocCacheSize[i].cachePool->freeCount = PAGE_2M_SIZE / kmallocCacheSize[i].size;
        kmallocCacheSize[i].cachePool->colorCount = kmallocCacheSize[i].cachePool->freeCount;
        kmallocCacheSize[i].cachePool->colorLength = ((kmallocCacheSize[i].cachePool->colorCount + sizeof(unsigned long) * 8 -1) >> 6 ) << 3;
        kmallocCacheSize[i].cachePool->colorMap = (unsigned long*)mm_struct.strEnd;

        mm_struct.strEnd = (unsigned long)(mm_struct.strEnd + kmallocCacheSize[i].cachePool->colorLength +sizeof(unsigned long) * 10) & \
                            (~(sizeof(unsigned long) - 1));
        memset(kmallocCacheSize[i].cachePool->colorMap, 0, kmallocCacheSize[i].cachePool->colorLength);
        kmallocCacheSize[i].totalUsed = 0;
        kmallocCacheSize[i].totalFree = kmallocCacheSize[i].cachePool->freeCount; 
    }
   
    /////////////////init page////////////
    pageEnd = Virt_To_Phy(mm_struct.strEnd) >> PAGE_2M_SHIFT;
    i = PAGE_2M_ALIGN(Virt_To_Phy(initStrEnd)) >> PAGE_2M_SHIFT;
    for(; i <= pageEnd; i++)
    {
        page = mm_struct.page + i;
        page->phy->pageUsedNum++;
        page->phy->PageFreeNum--;
        *(mm_struct.bitMap + (((unsigned long)page->start >> PAGE_2M_SHIFT) >> 6)) |=  1UL << (((unsigned long)page->start >> PAGE_2M_SHIFT) % 64);
        page->attribute = 0;
        page_init(page, PG_PTable_Maped | PG_Kernel | PG_Kernel_Init);
    }
    color_printk(RED,BLACK,"2.mm_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",*mm_struct.bitMap, \
    mm_struct.PHY->pageUsedNum, mm_struct.PHY->PageFreeNum);
    
    for(i = 0; i < 16; i++)
    {
        unsigned long* virt = (unsigned long*)((mm_struct.strEnd + PAGE_2M_SIZE * i + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
        page = Virt_To_2M_Page(virt);
        page->attribute = 0;
        page->phy->pageUsedNum++;
        page->phy->PageFreeNum--;
        *(mm_struct.bitMap + ((page->start >> PAGE_2M_SHIFT) >> 6)) |= 1UL << ((page->start >> PAGE_2M_SHIFT) % 64);
        page_init(page, PG_PTable_Maped | PG_Kernel | PG_Kernel_Init);
        kmallocCacheSize[i].cachePool->page = page;
        kmallocCacheSize[i].cachePool->Vaddress = virt;
    }
    color_printk(RED,BLACK,"3.mm_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",*mm_struct.bitMap, \
    mm_struct.PHY->pageUsedNum, mm_struct.PHY->PageFreeNum);
    color_printk(ORANGE,BLACK,"start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",mm_struct.codeStart, \
    mm_struct.codeEnd, mm_struct.dataEnd, mm_struct.brkEnd, mm_struct.strEnd);
    
    return 0;
}

void* kmalloc(unsigned long size, unsigned long flag)
{
    void* ret = NULL;
    struct slab* pool = NULL;
    int i = 0, j = 0;
    if(size > 1048576)
    {
        printk("kmalloc error size too long %s %d\n", __FILE__,__LINE__);
        return NULL;
    }
    for(; i < 16; i++)
    {
        if(kmallocCacheSize[i].size >= size)
            break;
    }
    if(0 != kmallocCacheSize[i].totalFree)
    {
        pool = kmallocCacheSize[i].cachePool;
        do
        {
            if(0 != pool->freeCount)
                break;
            pool = container_of(list_next(&pool->list), struct slab, list);
        }while(pool != kmallocCacheSize[i].cachePool);
    }
    else
    {
        pool = kmallocCreate(kmallocCacheSize[i].size);
        if(NULL == pool)
        {
            color_printk(RED, BLACK,"kmalloc()==>kmallocCreate() fail %s %d \n", __FILE__, __LINE__);
            return NULL;
        }
        kmallocCacheSize[i].totalFree += pool->freeCount;
        color_printk(RED, BLACK,"kmalloc()==>kmallocCreate() success %s %d \n", __FILE__, __LINE__);
        list_add_to_before(&kmallocCacheSize[i].cachePool->list, &pool->list);
    }

    for(j = 0; j < pool->colorCount; j++)
    {
        if(*(pool->colorMap + (j >> 6)) == 0xffffffffffffffff)
        {
            i += 63;
            continue;
        }
        if((*(pool->colorMap + (j >> 6)) & (1UL << (j % 64))) == 0)
        {
            *(pool->colorMap + (j >> 6)) |= 1UL << (j % 64);
            pool->freeCount--;
            pool->usedCount++;

            kmallocCacheSize[i].totalFree--;
            kmallocCacheSize[i].totalUsed++;

            ret = (void *)(((unsigned long)pool->Vaddress) + kmallocCacheSize[i].size * j);
            return ret;
        }
    }
    color_printk(RED, BLACK, "kmalloc  fail %s %d \n", __FILE__, __LINE__);
    return ret;
}

long kfree(void* address)
{
    void* pageBase = (void*)(((unsigned long)address) & PAGE_2M_MASK);
    unsigned int i = 0;
    struct slab* pool = NULL;

    for(;i < 16; i++)
    {
        pool = kmallocCacheSize[i].cachePool;
        do
        {
            if(pool->Vaddress == pageBase)
            {
                unsigned int index = ((unsigned long)address - (unsigned long)pageBase) / kmallocCacheSize[i].size;
                *(pool->colorMap + (index >> 6)) ^= 1UL << (index % 64);
                pool->usedCount--;
                pool->freeCount++;

                kmallocCacheSize[i].totalFree++;
                kmallocCacheSize[i].totalUsed--;

                if( (pool->usedCount == 0) && (kmallocCacheSize[i].totalFree >= (pool->colorCount *3 /2)) && \
                (kmallocCacheSize[i].cachePool != pool) )
                {
                    list_del(&pool->list);
                    kmallocCacheSize[i].totalFree -= pool->freeCount;
                    page_clean(pool->page);
                    free_pages(pool->page, 1);
                    if(kmallocCacheSize[i].size > 512)
                    {
                        kfree(pool->colorMap);
                        kfree(pool);
                    }
                }
                return 0;
            }
            pool = container_of(list_next(&pool->list), struct slab, list);
        }while(pool != kmallocCacheSize[i].cachePool);
    }

    return -1;
}

struct slab* kmallocCreate(unsigned size)
{
    struct Page_Magement * page = allocPage(PHY_NORMAL, 1, PG_Kernel);
    unsigned long* vaddress = NULL;
    unsigned long strSize = 0;
    unsigned long colorLength;
    struct slab* pool = NULL;

    if(NULL == page)
    {
        color_printk(RED, BLACK, "kmallocCreate() no page %s %d\n",__FILE__, __LINE__);
        return pool;
    }

    switch(size)
    {
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
            vaddress = Phy_To_Virt(page->start);
            strSize = sizeof(struct slab) + (PAGE_2M_SIZE / size) / 8;
            pool = (struct slab*)(((unsigned long)vaddress) + PAGE_2M_SIZE - strSize);
            list_init(&pool->list);
            pool->page = page;
            pool->Vaddress = vaddress;
            pool->usedCount = 0;
            pool->freeCount = (PAGE_2M_SIZE - strSize) / size;
            pool->colorCount = pool->freeCount;
            pool->colorLength = (((unsigned long)vaddress) + PAGE_2M_SIZE) - ((unsigned long)pool) - sizeof(struct slab);
            pool->colorMap = (unsigned long *)(((unsigned long)pool) + sizeof(struct slab));
            memset(pool->colorMap, 0, pool->colorLength);
            break;
        case 1024:
        case 2048:
        case 4096:
        case 8129:
        case 16384:
        case 32768:
        case 65536:
        case 131072:
        case 262144:
        case 524288:
        case 1048576:
            pool = kmalloc(sizeof(struct slab), 0);
            if(NULL == pool)
            {
                free_pages(page, 1);
                return pool;
            }
            list_init(&pool->list);
            pool->page = page;
            pool->usedCount = 0;
            pool->freeCount = PAGE_2M_SIZE / size;
            pool->Vaddress = Phy_To_Virt(page->start);
            pool->colorCount = pool->freeCount;
            pool->colorLength = ((pool->colorCount + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
            pool->colorMap = (unsigned long*)kmalloc(pool->colorLength, 0);
            if(NULL == pool->colorMap)
            {
                kfree(pool);
                free_pages(page, 1);
                return NULL;
            }
            memset(pool->colorMap, 0, pool->colorLength);
            break;
        default:
            free_pages(page, 1);
            return pool;
    }
    return pool;
}

void pagetable_init()
{
    unsigned long * tmpAddr = NULL;
    GlobalCR3 = getGdt();
    int i = 0;
    for(; i < mm_struct.PHYNum; i++)
    {
        int j = 0;
        struct PHY_Memory* phy = mm_struct.PHY + i;
        struct Page_Magement* page = phy->page;
        printk("PHY_UNMAPED_INDEX is %d\n", PHY_UNMAPED_INDEX);
        if(PHY_UNMAPED_INDEX && i == PHY_UNMAPED_INDEX)
            break;
        
        for(; j < phy->pageNum; j++, page++)
        {
            unsigned long* tmp = (unsigned long *)(((unsigned long)Phy_To_Virt((unsigned long)GlobalCR3 & (~ 0xfffUL))) + \
            (((unsigned long)Phy_To_Virt(page->start) >> PAGE_GDT_SHIFT) & 0x1ff) * 8);
            if(*tmp == 0)
            {
                unsigned long* virtual = kmalloc(PAGE_4K_SIZE, 0);
                set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual),PAGE_KERNEL_GDT));
            }
            tmp = (unsigned long *)((unsigned long)Phy_To_Virt(*tmp & (~ 0xfffUL)) + \
            (((unsigned long)Phy_To_Virt(page->start) >> PAGE_1G_SHIFT) & 0x1ff) * 8);
			if(*tmp == 0)
			{
				unsigned long * virtual = kmalloc(PAGE_4K_SIZE,0);
				set_pdpt(tmp,mk_pdpt(Virt_To_Phy(virtual),PAGE_KERNEL_Dir));
			}
            tmp = (unsigned long *)((unsigned long)Phy_To_Virt(*tmp & (~ 0xfffUL)) + \
            (((unsigned long)Phy_To_Virt(page->start) >> PAGE_2M_SHIFT) & 0x1ff) * 8);

			set_pdt(tmp,mk_pdt(page->start,PAGE_KERNEL_Page));
            if(j % 50 == 0)
				color_printk(GREEN,BLACK,"@:%#018lx,%#018lx\t\n",(unsigned long)tmp,*tmp);
            updateTlb();
        }
    }
}