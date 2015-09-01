
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <asm/reg.h>
#include <asm/io.h>
#include <sysdev/fsl_soc.h>
#include <asm/udbg.h>
#include <linux/slab.h>
#include <linux/mpishm.h>
#include <linux/module.h> 

//#define DEBUG_SHM

#ifdef DEBUG_SHM
#define debug_printk(X...) { printk("----->%s: ",__FUNCTION__); printk (X); printk ("\n\r"); }
#else
#define debug_printk(X...)
#endif


#define POOL_1K_NUMBER   4096
#define POOL_2K_NUMBER   4096
#define POOL_4K_NUMBER   2048
#define POOL_8K_NUMBER   2048
#define POOL_16K_NUMBER   64
#define POOL_32K_NUMBER   32
#define POOL_64K_NUMBER   16
#define POOL_128K_NUMBER   8
#define POOL_256K_NUMBER   8
#define POOL_512K_NUMBER   4
#define POOL_1M_NUMBER      4
#define POOL_2M_NUMBER      4
#define POOL_4M_NUMBER      4
#define POOL_8M_NUMBER      2
#define POOL_16M_NUMBER      0

#define BLOCK_OVERHEAD_SIZE (sizeof(struct Mod_mem)-4)

static struct Mod_list list_1k[POOL_1K_NUMBER];
static struct Mod_list list_2k[POOL_2K_NUMBER];
static struct Mod_list list_4k[POOL_4K_NUMBER];
static struct Mod_list list_8k[POOL_8K_NUMBER];
static struct Mod_list list_16k[POOL_16K_NUMBER];
static struct Mod_list list_32k[POOL_32K_NUMBER];
static struct Mod_list list_64k[POOL_64K_NUMBER];
static struct Mod_list list_128k[POOL_128K_NUMBER];
static struct Mod_list list_256k[POOL_256K_NUMBER];
static struct Mod_list list_512k[POOL_512K_NUMBER];
static struct Mod_list list_1M[POOL_1M_NUMBER];
static struct Mod_list list_2M[POOL_2M_NUMBER];
static struct Mod_list list_4M[POOL_4M_NUMBER];
static struct Mod_list list_8M[POOL_8M_NUMBER];
static struct Mod_list list_16M[POOL_16M_NUMBER];

static struct Shm_control_area *pshm_control_area = 0;

static struct Mod_heap s_mod_heap_array[TOTAL_POOL_NUMBER];
   
static unsigned s_pool_number_array[TOTAL_POOL_NUMBER] =
{ 
   POOL_1K_NUMBER,
   POOL_2K_NUMBER,
   POOL_4K_NUMBER,
   POOL_8K_NUMBER,
   POOL_16K_NUMBER,
   POOL_32K_NUMBER,
   POOL_64K_NUMBER,
   POOL_128K_NUMBER,
   POOL_256K_NUMBER,
   POOL_512K_NUMBER,
   POOL_1M_NUMBER,
   POOL_2M_NUMBER,
   POOL_4M_NUMBER,
   POOL_8M_NUMBER,
   POOL_16M_NUMBER,
};

static struct Mod_list *s_pool_list_array[TOTAL_POOL_NUMBER] =
{ 
   list_1k,
   list_2k,
   list_4k,
   list_8k,
   list_16k,
   list_32k,
   list_64k,
   list_128k,
   list_256k,
   list_512k,
   list_1M,
   list_2M,
   list_4M,
   list_8M,
   list_16M,
};

extern struct Mpi_mutex sync_shm_lock;

extern int message_write(unsigned slot, int message);

#define false 0
#define true 1
static int mod_mem_init_flag = false;
#define SPR_PIR	     0x3FF
static unsigned get_cpuid(void)
{
   unsigned cpuid;
   __asm__ volatile ("mfspr %0,%1" : "=r" (cpuid) : "i" (SPR_PIR));

   return cpuid;
}


static unsigned total_shm_number(unsigned total_pools,unsigned *pool_number_array)
{
   unsigned ret = 0;
   int i;

   for(i=0;i<total_pools;++i)
   {
      ret += pool_number_array[i];
   }

   debug_printk("Total shm number[0x%x:%d]\n", ret,ret);
   return ret;
}

static unsigned total_shm_size(unsigned start_pool_order,unsigned total_pools,unsigned *pool_number_array)
{
   unsigned ret = 0;
   int i;
   unsigned size = 1<<start_pool_order; 
   
   for(i=0;i<total_pools;++i)
   {
      ret += pool_number_array[i] * size;
      size <<=1;
   }

   debug_printk("Total shm size[0x%x:%d]\n", ret,ret);
   return ret;
}

static int print_heap(struct Mod_heap *heap)
{
   int ret = 0;
   assert(heap);
   printk("%s: number[%d], head[%p],end[%p], block size[%d]\n", __FUNCTION__, 
      heap->m_number,heap->m_head,heap->m_end, heap->m_block_size+BLOCK_OVERHEAD_SIZE);
   printk("free[%d], busy[%d]\n", heap->m_free.m_number,heap->m_busy.m_number);

   return ret;
}

static int add_list_head(struct Mod_list_head *head, struct Mod_list *plist)
{
   int ret = 0;
   assert(plist);
   assert(head);

   plist->m_previous = 0;
   plist->m_list_next = head->m_list_next;
   if(head->m_list_next)head->m_list_next->m_previous = plist;
   head->m_list_next = plist;
   ++head->m_number;  
   return ret;
}

static int remove_list_head(struct Mod_list_head *head)
{
   int ret = -1;
   assert(head);
   
   if(head->m_number)
   {
      struct Mod_list *plist = head->m_list_next;
      ret = 0;
      assert(plist);
      --head->m_number;
      head->m_list_next = plist->m_list_next;
      if(plist->m_list_next)
      {
         assert(head->m_number);
         plist->m_list_next->m_previous = 0;
      }
   }

   return ret;      
}

static int remove_list_entry(struct Mod_list_head *head,struct Mod_list *plist)
{
   int ret = -1;
   assert(plist);
   assert(head);
   
   if(head->m_number)
   {
      struct Mod_list *ptemp = head->m_list_next;
      assert(ptemp);
      while(ptemp)
      {
         if(ptemp==plist)
         {
            --head->m_number;
            if(ptemp->m_previous==0)
            {
               head->m_list_next = ptemp->m_list_next;
            }
            else
            {
               ptemp->m_previous->m_list_next = ptemp->m_list_next;               
            }
            
            if(ptemp->m_list_next)
            {
               assert(head->m_number);
               ptemp->m_list_next->m_previous = ptemp->m_previous;
            }            
            ret = 0;
            break;
         }
         ptemp = ptemp->m_list_next;
      }      
   }

   return ret;      
}
   
static int add_info_entry(struct Shm_info_head *head, struct Shm_block_info *plist)
{
   int ret = 0;
   assert(plist&&head);

   plist->m_previous = 0;
   plist->m_list_next = head->m_list_next;
   if(head->m_list_next)head->m_list_next->m_previous = plist;
   head->m_list_next = plist;
   ++head->m_number;  
   return ret;
}

static int remove_info_head(struct Shm_info_head *head)
{
   int ret = -1;
   assert(head);
   
   if(head->m_number)
   {
      struct Shm_block_info *plist = head->m_list_next;
      ret = 0;
      assert(plist);
      --head->m_number;
      head->m_list_next = plist->m_list_next;
      if(plist->m_list_next)
      {
         assert(head->m_number);
         plist->m_list_next->m_previous = 0;
      }
   }

   return ret;      
}

static int remove_info_entry(struct Shm_info_head *head,struct Shm_block_info *plist)
{
   int ret = -1;
   assert(plist&&head);
   
   if(head->m_number)
   {
      struct Shm_block_info *ptemp = head->m_list_next;
      assert(ptemp);
      while(ptemp)
      {
         if(ptemp==plist)
         {
            --head->m_number;
            if(ptemp->m_previous==0)
            {
               head->m_list_next = ptemp->m_list_next;
            }
            else
            {
               ptemp->m_previous->m_list_next = ptemp->m_list_next;               
            }
            
            if(ptemp->m_list_next)
            {
               assert(head->m_number);
               ptemp->m_list_next->m_previous = ptemp->m_previous;
            }            
            ret = 0;
            break;
         }
         ptemp = ptemp->m_list_next;
      }      
   }

   return ret;      
}
   
void print_control_info(void)
{
   if(pshm_control_area)
   {
      printk("%s: control pointer[%p],start order[%d], total pools[%d],block infor start[%p], free:busy[%d:%d]\n", __FUNCTION__,
         pshm_control_area,pshm_control_area->m_start_pool_order,pshm_control_area->m_total_pools,
         pshm_control_area->m_block_start,pshm_control_area->m_free.m_number,pshm_control_area->m_busy.m_number);
   }
}  
   

static int mod_mem_init_heap(struct Mod_heap *heap, unsigned order, unsigned number,struct Mod_list *plist, char *ppool, unsigned local_only)
{
   int ret = 0;
   struct Mod_mem *pblock = 0;
   int block_size,full_block_size;
   int i;

   if(!number) return ret;
   
   assert(heap&&order&&plist&&ppool);
   memset((void *)heap, 0, sizeof(*heap));
   
   full_block_size = 1<<order;
   debug_printk("heap[%p], order[%d], number[%d], size[0x%x:%d], plist[%p],ppool[%p]\n", 
      heap,order,number,full_block_size,full_block_size,plist,ppool);


   ret = Mpi_mutex_init(&heap->m_lock,MPI_SYSTEM_LOCK_SHM+order);
   if(ret!=0)
   {
      Mpi_mutex_destroy(&heap->m_lock);
      return -1;
   }

   block_size = full_block_size - BLOCK_OVERHEAD_SIZE;
   heap->m_number = number;
   heap->m_block_size = block_size;
   heap->m_order = order;
   heap->m_head = ppool;
   heap->m_end = 0;
   heap->m_list = plist;
   
   memset((void *)heap->m_list, 0, sizeof(struct Mod_list)*heap->m_number);
   heap->m_free.m_list_next = 0;
   heap->m_free.m_number = 0;

   heap->m_busy.m_list_next = 0;
   heap->m_busy.m_number = 0;
         
   for(i=0;i<heap->m_number;++i)
   {
      pblock = (struct Mod_mem *)ppool;

      if(!local_only)
      {
         pblock->m_flag = 0;
         pblock->m_signature = MOD_SIGNATURE;
         //pblock->m_heap = (char *)heap;
         pblock->m_order = order;
         pblock->m_number = i;
         pblock->m_length = 0;
         //pblock->m_list = plist;
      }

      plist->m_list_data = pblock;      
      add_list_head(&heap->m_free,plist);
      ++plist;      

      ppool +=full_block_size;      
   }
   heap->m_end = ppool;
   ret = full_block_size * number;

#ifdef DEBUG_SHM
   print_heap(heap);
#endif

#if 0   
   printk("%s: number[%d], head[%p],end[%p], block size[%d]\n", __FUNCTION__, 
      heap->m_number,heap->m_head,heap->m_end);
   printk("free[%d], busy[%d]\n", heap->m_free.size(),heap->m_busy.size());
#endif

   return ret;
}


static int mod_mem_init(char *start, unsigned length, unsigned start_pool_order,unsigned total_pools,unsigned *pool_number_array, unsigned local_only)
{
   int ret = 0;
   char *rounded_start;
   unsigned rounded_length;
   int i;
   int pool_size;
   unsigned total_number, control_size;
   char *ppool;   

   debug_printk("start[%p], lenght[0x%x:%d],start order[%d], total pools[%d]\n", 
                       start,length,length,start_pool_order,total_pools);
   
   if(total_pools>TOTAL_POOL_NUMBER)
   {
      printk("%s: pool number too big[%d] > [%d]\n",__FUNCTION__,total_pools,TOTAL_POOL_NUMBER);
      return -1;
   }
      
   assert(start);     

   total_number = total_shm_number(total_pools, pool_number_array);
   
   control_size = total_number * sizeof(struct Shm_block_info) + sizeof(struct Shm_control_area);
   
   rounded_start = (char *)((unsigned)(start + control_size + 4096 - 1) & (~(1024-1)));
   
   assert(length > (rounded_start - start));
   
   rounded_length = length - (rounded_start - start);
   debug_printk("control size[0x%x], rounded start[%p],length[0x%x:%d]\n", control_size, rounded_start,rounded_length,rounded_length);

   if(rounded_length < total_shm_size(start_pool_order,total_pools,pool_number_array))
   {
      printk("%s: not enough memory size for the pool\n",__FUNCTION__);
      return -1;
   }

   ppool = rounded_start;
   
   for(i=0;i<total_pools;++i)
   {      
      pool_size = mod_mem_init_heap(&s_mod_heap_array[i], start_pool_order+i, 
                           pool_number_array[i],s_pool_list_array[i],ppool,local_only);

      if(pool_size<0) 
      {
         printk("init memory heap failed.\n");
         return -1;
      }
      ppool = (char *)((unsigned)ppool + pool_size);
      debug_printk("i[%d], pool_size[0x%x:%d], ppool[%p]\n", i, pool_size,pool_size, ppool);
   }

   
   return ret;
}

static struct Mpi_mutex s_mod_lock;
static int test_handle = -1;

int mod_init(char *start, unsigned length)
{
   int ret;
   int i;
   struct Shm_block_info *pinfo;
   unsigned total_number;
   unsigned start_pool_order,total_pools;
   unsigned local_only = 0;
   unsigned *pool_number_array = s_pool_number_array;

   unsigned length1;

   if(mod_mem_init_flag) 
   {
      printk("%s: mod memory pool was inited before\n",__FUNCTION__);
      return -1;
   }

   ret = Mpi_mutex_init(&s_mod_lock,MPI_SYSTEM_LOCK_SHM);
   if(ret!=0)
   {
      printk("%s: can not init share memory lock\n", __FUNCTION__);
      Mpi_mutex_destroy(&s_mod_lock);
      return -1;
   }  
   
   Mpi_lock(&s_mod_lock);
   

   pshm_control_area = (struct Shm_control_area *)start;
   if(pshm_control_area->m_init_flag)
   {
      debug_printk("share memory was initialized by other core\n");
      local_only = 1;
      start_pool_order = pshm_control_area->m_start_pool_order;
      total_pools = pshm_control_area->m_total_pools;
      if((start_pool_order!=STARTING_POOL_ORDER)||(total_pools!=TOTAL_POOL_NUMBER))
      {
         printk("share memory pool number[%d:%d] and starting order[%d:%d] do not match\n",
            total_pools,TOTAL_POOL_NUMBER,start_pool_order,STARTING_POOL_ORDER);

         pshm_control_area = 0;
         Mpi_unlock(&s_mod_lock);
         Mpi_mutex_destroy(&s_mod_lock);
         return -1;
      }
      
      pool_number_array = pshm_control_area->m_pool_array;
   }
   else
   {
      debug_printk("Initializing share memory the first time\n");
      start_pool_order = STARTING_POOL_ORDER;
      total_pools = TOTAL_POOL_NUMBER;
   }

   ret = mod_mem_init(start,length, start_pool_order,total_pools,pool_number_array, local_only);     
   if(ret!=0)
   {
      pshm_control_area = 0;
      Mpi_unlock(&s_mod_lock);
      Mpi_mutex_destroy(&s_mod_lock);
      return ret;
   }

   if(!local_only)
   {
      //pshm_control_area = (struct Shm_control_area *)start;
      pshm_control_area->m_init_flag = 1;
      pshm_control_area->m_start_pool_order = start_pool_order;
      pshm_control_area->m_total_pools = total_pools;

      pshm_control_area->m_block_start = (struct Shm_block_info *)((unsigned)(start + sizeof(struct Shm_control_area) + 32 - 1) & (~(32-1)));
      pinfo = pshm_control_area->m_block_start;

      total_number = total_shm_number(total_pools, s_pool_number_array);
      pshm_control_area->m_total_blocks = total_number;
      for(i=0;i<total_number;++i)
      {
         pinfo->m_id = i;
         pinfo->m_signature = 0x6AE528C1;
         add_info_entry(&pshm_control_area->m_free,pinfo);
         ++pinfo;
      }

      assert(total_pools<MAX_POOL_NUMBER);
      for(i=0;i<total_pools;++i)
      {
         pshm_control_area->m_pool_array[i] = s_pool_number_array[i];
      }
   }

   Mpi_unlock(&s_mod_lock);
   
   mod_mem_init_flag = true;

   print_control_info();

   length1 = MPI_SHM_TEST_LENGTH;
   test_handle = mpi_shm_open(MPI_SHM_TEST_NAME,&length1,0);
   debug_printk("test handle[%d]\n", test_handle);


   mpi_default_channel_init();
   return ret;
   
}

int mod_mem_stat(unsigned pool)
{
   int ret = 0;

   if(pool > TOTAL_POOL_NUMBER)
   {
      printk("Wrong pool number[%d], range[0 -- %d]\n", pool,TOTAL_POOL_NUMBER);
      return -1;
   }

#if 0
   if(pshm_control_area)
   {
      pheap = s_mod_heap_array[(pshm_control_area->m_start_pool_order - STARTING_POOL_ORDER)]
   }
#endif

   print_heap(&s_mod_heap_array[pool]);
   
   return ret;
}

int mod_mem_stat_all(void)
{
   int ret = 0;
   int i;

   for(i=0;i<TOTAL_POOL_NUMBER;++i)
   {
      print_heap(&s_mod_heap_array[i]);
   }

   return ret;
}


static void *mod_malloc_heap(unsigned size, struct Mod_heap *heap)
{
   void *ret = 0;
   struct Mod_mem *pblock = 0;
   struct Mod_list *plist = 0;
   assert(heap);
   assert(size&&(size<=heap->m_block_size));

   Mpi_lock(&heap->m_lock);

   if(heap->m_free.m_number)
   {
      plist = heap->m_free.m_list_next;
      assert(plist);
      pblock = plist->m_list_data;

      debug_printk("pblock[%p], signature[0x%x], pdata[%p]\n", pblock, pblock->m_signature, &(pblock->m_pdata));      
      pblock->m_flag |=MOD_MEMORY_BUSY_FLAG;
      pblock->m_length = size;

      remove_list_head(&heap->m_free);      
      add_list_head(&heap->m_busy,plist);
      
      ret = &(pblock->m_pdata);
      Mpi_unlock(&heap->m_lock);

      debug_printk(">>>mod_malloc:[%p/%p], length[%d], signature[0x%x], order[%d],number[%d]\n",ret, pblock, size, 
         pblock->m_signature, pblock->m_order,pblock->m_number);
   }
   else
   {
      Mpi_unlock(&heap->m_lock);
      debug_printk("size[%d], pool[%d] run out of memory\n", size, heap->m_block_size+BLOCK_OVERHEAD_SIZE);
      ret = 0;
   }

   //print_heap(heap);
   //debug_printk("size[%d],ret[%p]\n", size,ret);
   //DPRINT_MOD("size[%d],ret[%p]\n", size,ret);

   return ret;
}

void *mod_malloc(unsigned size)
{
   unsigned i;
   void *ret = 0;
   
   if((!pshm_control_area)||(!mod_mem_init_flag)||(!size))return 0;

   assert(pshm_control_area->m_start_pool_order >= STARTING_POOL_ORDER);
   
   i = pshm_control_area->m_start_pool_order - STARTING_POOL_ORDER;
   
   for(;i<pshm_control_area->m_total_pools;++i)
   {
      if(size<=s_mod_heap_array[i].m_block_size)
      {
         ret = mod_malloc_heap(size, &s_mod_heap_array[i]);
         if(ret)
         {
            return ret;
         }
      }
   }
   
   return ret;

}

void mod_free(void *ptr)
{
   unsigned order,number;
   unsigned index;
   struct Mod_heap *pheap;
   struct Mod_list *plist, *pplist;
   struct Mod_mem *pmem;
   
   if((!pshm_control_area)||(!mod_mem_init_flag)||(!ptr))return;
   
   pmem = (struct Mod_mem *)((char *)ptr - BLOCK_OVERHEAD_SIZE);

   order = pmem->m_order;
   number = pmem->m_number;

   //printk("%s: ptr[%p]\n",__FUNCTION__, ptr);
   debug_printk(">>>ptr[%p],length[%d],order[%d],number[%d]\n", ptr, pmem->m_length,order,number);

   if(pmem->m_signature != MOD_SIGNATURE)
   {
      printk("%s: ptr[%p], wrong signature[0x%x]\n", __FUNCTION__, ptr, pmem->m_signature);
      return;
   }
   assert(order < (pshm_control_area->m_start_pool_order+pshm_control_area->m_total_pools));   
   assert(order > STARTING_POOL_ORDER);

   index = order - STARTING_POOL_ORDER;
   pheap = &s_mod_heap_array[index];
   pplist = s_pool_list_array[index];
   plist = &pplist[number];

   Mpi_lock(&pheap->m_lock);   
   if((pmem->m_flag&MOD_MEMORY_BUSY_FLAG)==0)
   {
      Mpi_unlock(&pheap->m_lock);
      printk("%s: ptr[%p], wrong flag[0x%x]\n", __FUNCTION__, ptr, pmem->m_flag);
      return;
   }
   
   if(((char *)ptr<pheap->m_head)||((char *)ptr>pheap->m_end))
   {
      Mpi_unlock(&pheap->m_lock);
      printk("%s: ptr[%p], not in the range[%p - %p]\n", __FUNCTION__, ptr, pheap->m_head, pheap->m_end);
      return;
   }

   if(pmem->m_length > pheap->m_block_size)
   {
      Mpi_unlock(&pheap->m_lock);
      printk("%s: ptr[%p], size too big[%d > %d]\n", __FUNCTION__, ptr, pmem->m_length, pheap->m_block_size);
      return;
   }

   if(!pheap->m_busy.m_number)
   {
      Mpi_unlock(&pheap->m_lock);
      printk("%s: ptr[%p],not in busy list? since it's empty\n", __FUNCTION__, ptr);      
      return;
   }

   if(remove_list_entry(&pheap->m_busy,plist)!=0)
   {
      Mpi_unlock(&pheap->m_lock);
      printk("%s: ptr[%p],not in busy list? failed to remove\n", __FUNCTION__, ptr);      
      return;      
   }

   pmem->m_flag &=  (~MOD_MEMORY_BUSY_FLAG);
   pmem->m_length = 0;
   
   add_list_head(&pheap->m_free,plist);
   
   Mpi_unlock(&pheap->m_lock);
   
   //print_heap(pheap);   
}


int mpi_shm_open(const char *name, unsigned *length, unsigned flag)
{
   int ret = MPI_SHM_OPEN_ERR;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();
   unsigned size = *length;
   void *pmem = 0;
   
   assert(pshm_control_area);
   assert(name && length);

   if(!size) return ret;

   debug_printk("name[%s], length[%d],flag[0x%x],pshm_control_area[%p]\n", name, *length, flag,pshm_control_area);
   Mpi_lock(&s_mod_lock);
   pinfo = pshm_control_area->m_busy.m_list_next;
   debug_printk("pinfo: %p\n", pinfo);
   while(pinfo)
   {
      if(!strcmp(pinfo->m_name, name))
      {
         debug_printk("%s was already opened\n", name);
         if(size > pinfo->m_length) 
         {
            debug_printk("over size: %d --> %d\n", size, pinfo->m_length);
            ret = MPI_SHM_OPEN_LENGTH_ERR;
         }
         else if(pinfo->m_open & (1<<core)) 
         {
            debug_printk("%s was opened by local before\n", name);
            //ret = MPI_SHM_OPEN_BEFORE;
            *length = pinfo->m_length;
            ret = pinfo->m_id;                           
         }
         else
         {
            *length = pinfo->m_length;
            pinfo->m_open |= (1<<core);
            ret = pinfo->m_id;
            debug_printk("open size: %d --> %d, id[%d]\n", size, pinfo->m_length, ret);
         }
         break;
      }
      pinfo = pinfo->m_list_next;
   }

   debug_printk("pinfo[%p]...\n", pinfo);
   if((!pinfo)&&(pshm_control_area->m_free.m_list_next))
   {
      pinfo = pshm_control_area->m_free.m_list_next;
      debug_printk("pinfo[%p]\n", pinfo);
      pmem = mod_malloc(size);
      debug_printk("pmem[%p]\n", pmem);
      if(pmem)
      {
         remove_info_head(&pshm_control_area->m_free);
         add_info_entry(&pshm_control_area->m_busy,pinfo);
         pinfo->m_length = size;
         pinfo->m_open |= (1<<core);
         pinfo->m_mem = pmem;
         size = strlen(name);
         size = (size>=SHM_NAME_MAX_LENGTH)?(SHM_NAME_MAX_LENGTH-1):size;
         strncpy(pinfo->m_name,name,size);
         pinfo->m_name[size] = '\0';
         ret = pinfo->m_id;
         debug_printk("open data: %p length:%d, id[%d]\n",pmem, pinfo->m_length, ret);
      }
      else
      {
         ret = MPI_SHM_OPEN_MALLOC_ERR;
      }

      debug_printk("ret[%d]\n", ret);
      //debug_printk("pblock[%p], signature[0x%x], pdata[%p]\n", pblock, pblock->m_signature, &(pblock->m_pdata));      
      //pblock->m_flag |=MOD_MEMORY_BUSY_FLAG;
      //pblock->m_length = size;

      //remove_list_head(&heap->m_free);      
      //add_list_head(&heap->m_busy,plist);      
      
   }
      
   Mpi_unlock(&s_mod_lock);
   
   return ret;
}

int mpi_shm_close(int shm_handle)
{
   int ret = 0;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();

   assert(pshm_control_area);

   if((shm_handle<0)||(shm_handle>=pshm_control_area->m_total_blocks)) return -1;   

   pinfo = &pshm_control_area->m_block_start[shm_handle];

   if(pinfo->m_open&(1<<core))
   {   
      Mpi_lock(&s_mod_lock);
      pinfo->m_open &= (~(1<<core));
      if(!pinfo->m_open)
      {
         pinfo->m_name[0] = '\0';
         pinfo->m_length = 0;
         pinfo->m_update &= (~(1<<core));
         mod_free(pinfo->m_mem);
         pinfo->m_mem = 0;

         if(!pshm_control_area->m_busy.m_number)
         {
            Mpi_unlock(&s_mod_lock);
            printk("%s: handle[%d],not in busy list? since it's empty\n", __FUNCTION__, shm_handle);      
            return MPI_SHM_ERROR;
         }

         if(remove_info_entry(&pshm_control_area->m_busy,pinfo)!=0)
         {
            Mpi_unlock(&s_mod_lock);
            printk("%s: handle[%d],not in busy list? failed to remove\n", __FUNCTION__, shm_handle);      
            return MPI_SHM_ERROR;      
         }         
         add_info_entry(&pshm_control_area->m_free,pinfo);  
         debug_printk("close this handle[%d]\n", shm_handle);
      }
      else
      {
         debug_printk("keeping this handle[%d] by peer\n", shm_handle);
      }
      Mpi_unlock(&s_mod_lock);
   }
   else
   {
      printk("%s: handle[%d] was not opened\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NOT_OPEN;
   }
   
   return ret;
}

void *mpi_shm_get_mem(int shm_handle)
{
   void *ret = 0;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();
   
   assert(pshm_control_area);
   if((shm_handle<0)||(shm_handle>=pshm_control_area->m_total_blocks)) return ret;   

   pinfo = &pshm_control_area->m_block_start[shm_handle];
   if(pinfo->m_open&(1<<core))
   {
      ret = pinfo->m_mem;
      debug_printk("returning the memory pointer[%p] for handle[%d]\n", ret, shm_handle);
   }
   else
   {
      printk("%s: handle[%d] was not opened\n", __FUNCTION__,shm_handle);
   }
   
      
   return ret;
}

int mpi_shm_get_test_handle(void)
{
   return test_handle;
}

int mpi_shm_send(int shm_handle, const void *data, unsigned length)
{
   int ret = 0;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();
   unsigned size = 0;
   
   assert(pshm_control_area);
   if((shm_handle<0)||(shm_handle>=pshm_control_area->m_total_blocks)) return MPI_SHM_OPEN_ERR;   
   
   pinfo = &pshm_control_area->m_block_start[shm_handle];

   if(pinfo->m_mem==0)
   {
      printk("%s: handle[%d] has NULL memory pointer\n", __FUNCTION__,shm_handle);
      
      ret = MPI_SHM_NULL;
   }   
   else if(pinfo->m_open&(1<<core))
   {
      size = (length>pinfo->m_length)? pinfo->m_length:length;
      Mpi_lock(&s_mod_lock);
      memcpy(pinfo->m_mem, data, size);
      pinfo->m_update |= (1<<core);
      ret = size;
      pinfo->m_size = size;
      pinfo->m_offset = 0;
      
      Mpi_unlock(&s_mod_lock);
      debug_printk("send length[%d:%d] on handle[%d]\n", length,ret, shm_handle);
   }
   else
   {
      printk("%s: handle[%d] was not opened\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NOT_OPEN;
   }

   return ret;
}

int mpi_shm_send_offset(int shm_handle, const void *data, unsigned length,unsigned offset)
{
   int ret = 0;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();
   unsigned size = 0;
   
   assert(pshm_control_area);
   if((shm_handle<0)||(shm_handle>=pshm_control_area->m_total_blocks)) return MPI_SHM_OPEN_ERR;   

   pinfo = &pshm_control_area->m_block_start[shm_handle];
   if(pinfo->m_length<=offset) return MPI_SHM_OPEN_LENGTH_ERR;

   if(pinfo->m_mem==0)
   {
      printk("%s: handle[%d] has NULL memory pointer\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NULL;
   }   
   else if(pinfo->m_open&(1<<core))
   {
      size = ((length+offset)>pinfo->m_length)? (pinfo->m_length-offset):length;
      Mpi_lock(&s_mod_lock);
      memcpy((char *)pinfo->m_mem + offset, data, size);
      pinfo->m_update |= (1<<core);
      pinfo->m_size = size;
      pinfo->m_offset = offset;
      ret = size;
      Mpi_unlock(&s_mod_lock);
      debug_printk("send length[%d:%d] on handle[%d]\n", length,ret, shm_handle);
   }
   else
   {
      printk("%s: handle[%d] was not opened\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NOT_OPEN;
   }

   return ret;
}

int mpi_shm_get(int shm_handle, void *data, unsigned length, unsigned update)
{
   int ret = 0;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();
   unsigned size = 0;

   if((!data) || (!length)) return MPI_SHM_INVALID;
   
   assert(pshm_control_area);
   if((shm_handle<0)||(shm_handle>=pshm_control_area->m_total_blocks)) return MPI_SHM_OPEN_ERR;   

   pinfo = &pshm_control_area->m_block_start[shm_handle];
   
   if(pinfo->m_mem==0)
   {
      printk("%s: handle[%d] has NULL memory pointer\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NULL;
   }   
   else if(pinfo->m_open&(1<<(1-core)))
   {
      //size = (length>pinfo->m_length)? pinfo->m_length:length;
      Mpi_lock(&s_mod_lock);
      
      if((update && (pinfo->m_update&(1<<core)))||(!update))
      {  
         size = (length>pinfo->m_size)? pinfo->m_size:length;
         if(size)memcpy(data, (char *)pinfo->m_mem+pinfo->m_offset, size);
         pinfo->m_update &= ~(1<<(1-core));
         pinfo->m_size = 0;
         pinfo->m_offset = 0;
         ret = size;
         debug_printk("get length[%d:%d] on handle[%d]\n", length,ret, shm_handle);
      }
      else
      {
         printk("%s: handle[%d] was not updated by peer\n", __FUNCTION__,shm_handle);
         ret = MPI_SHM_NOT_UPDATED;
      }
      Mpi_unlock(&s_mod_lock);
   }
   else
   {
      printk("%s: handle[%d] was not opened\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NOT_OPEN;
   }

   return ret;
}

int mpi_shm_get_offset(int shm_handle, void *data, unsigned length, unsigned update,unsigned offset)
{
   int ret = 0;
   struct Shm_block_info *pinfo = 0;
   unsigned core = get_cpuid();
   unsigned size = 0;

   if((!data) || (!length)) return MPI_SHM_INVALID;
   
   assert(pshm_control_area);
   if((shm_handle<0)||(shm_handle>=pshm_control_area->m_total_blocks)) return MPI_SHM_OPEN_ERR;   

   pinfo = &pshm_control_area->m_block_start[shm_handle];
   if(pinfo->m_length<=offset) return MPI_SHM_OPEN_LENGTH_ERR;
   
   if(pinfo->m_mem==0)
   {
      printk("%s: handle[%d] has NULL memory pointer\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NULL;
   }   
   else if(pinfo->m_open&(1<<(1-core)))
   {
      //size = (length>pinfo->m_length)? pinfo->m_length:length;
      size = ((length+offset)>pinfo->m_length)? (pinfo->m_length-offset):length;
      
      Mpi_lock(&s_mod_lock);
      
      if((update && (pinfo->m_update&(1<<core)))||(!update))
      {  
         //size = (length>pinfo->m_size)? pinfo->m_size:length;
         //size = ((length+offset)>pinfo->m_length)? (pinfo->m_length-offset):length;
         if(size)memcpy(data, (char *)pinfo->m_mem+offset, size);
         pinfo->m_update &= ~(1<<(1-core));
         //pinfo->m_size = 0;
         //pinfo->m_offset = 0;
         ret = size;
         debug_printk("get length[%d:%d] on handle[%d]\n", length,ret, shm_handle);
      }
      else
      {
         printk("%s: handle[%d] was not updated by peer\n", __FUNCTION__,shm_handle);
         ret = MPI_SHM_NOT_UPDATED;
      }
      Mpi_unlock(&s_mod_lock);
   }
   else
   {
      printk("%s: handle[%d] was not opened\n", __FUNCTION__,shm_handle);
      ret = MPI_SHM_NOT_OPEN;
   }

   return ret;
}


extern int message_int_reg(struct Mpi_channel *channel);
extern int message_int_unreg(struct Mpi_channel *channel);


static struct Mpi_channel s_mpi_default_channel;

static int mpi_default_channel_callback_func(void *data)
{
   int ret = 0;
   struct Mpi_channel *channel = (struct Mpi_channel *)data;
   char buffer[1024];
   
   assert(data==&s_mpi_default_channel);
   printk("%s: receive data[%p]\n", __FUNCTION__, data);

   ret = mpi_shm_get(channel->m_recv_handler,(void *)buffer, 1023, 0);
   if(ret>0) 
   {
      printk("received message[%d]:\n\t%s\n",ret, buffer);
   }    

   return ret;      
}
 
int mpi_default_channel_init(void)
{
   int ret = 0;
   unsigned length = 0;
   int core = get_cpuid();
   
#define MPI_DEFAULT_CHANNEL_UP_NAME    "mpi_default_channel_up"
#define MPI_DEFAULT_CHANNEL_DOWN_NAME    "mpi_default_channel_down"
#define MPI_DEFAULT_CHANNEL_SHM_LENGTH   4096

   memset((void *)&s_mpi_default_channel, 0, sizeof(s_mpi_default_channel));

   ret = Mpi_mutex_init(&s_mpi_default_channel.m_lock,MPI_SYSTEM_LOCK_DEFAULT_CHANNEL);
   if(ret!=0)
   {
      printk("%s: can not init lock for mpi default channel\n", __FUNCTION__);
      Mpi_mutex_destroy(&s_mpi_default_channel.m_lock);
      return -1;
   }

   if(core)
   {
      strcpy(s_mpi_default_channel.m_send_name,MPI_DEFAULT_CHANNEL_UP_NAME);
      strcpy(s_mpi_default_channel.m_recv_name,MPI_DEFAULT_CHANNEL_DOWN_NAME);
   }
   else
   {
      strcpy(s_mpi_default_channel.m_send_name,MPI_DEFAULT_CHANNEL_DOWN_NAME);
      strcpy(s_mpi_default_channel.m_recv_name,MPI_DEFAULT_CHANNEL_UP_NAME);   
   }

   length = MPI_DEFAULT_CHANNEL_SHM_LENGTH;   
   s_mpi_default_channel.m_send_handler = mpi_shm_open(s_mpi_default_channel.m_send_name,&length,0);   
   debug_printk("MPI default channel send handler[%d]\n",s_mpi_default_channel.m_send_handler);
   if(s_mpi_default_channel.m_send_handler<0) 
   {
      printk("%s: failed open share memory for sending\n", __FUNCTION__);
      return ret;
   }
   s_mpi_default_channel.m_send_length = length;
   
   length = MPI_DEFAULT_CHANNEL_SHM_LENGTH;   
   s_mpi_default_channel.m_recv_handler = mpi_shm_open(s_mpi_default_channel.m_recv_name,&length,0);   
   debug_printk("MPI default channel recv handler[%d]\n",s_mpi_default_channel.m_recv_handler);
   if(s_mpi_default_channel.m_recv_handler<0)
   {
      printk("%s: failed open share memory for receiving\n", __FUNCTION__);
      return ret;
   }

   s_mpi_default_channel.m_recv_length = length;   
   s_mpi_default_channel.m_message_slot = 1-core;
   s_mpi_default_channel.m_callback = mpi_default_channel_callback_func;
   message_int_reg(&s_mpi_default_channel);
   return ret;
}

int mpi_shm_interrupt_send(const void *data, unsigned length)
{
   int ret = -1;
   int core = get_cpuid();
   int shm_handle = s_mpi_default_channel.m_send_handler;

   debug_printk("data[%p], length[0x%x:0x%x],handle[%d]\n", data,length,s_mpi_default_channel.m_send_length,shm_handle);
   assert(shm_handle >= 0);
   assert(s_mpi_default_channel.m_send_length);

   if(s_mpi_default_channel.m_send_length<length)
   {
      printk("%s: message over size[%d > %d]\n", __FUNCTION__, length, s_mpi_default_channel.m_send_length);
      return ret;
   }
   
   ret = mpi_shm_send(shm_handle, data, length);   
   message_write(1-core, shm_handle);

   return ret;
}

int mpi_shm_interrupt_get(void *data, unsigned length)
{
   int ret = -1;
   int shm_handle = s_mpi_default_channel.m_recv_handler;
   assert(shm_handle >= 0);
   assert(s_mpi_default_channel.m_recv_length);

   ret = mpi_shm_get(shm_handle,data,length,0);

   return ret;
}


int mpi_shm_test_send(char *message)
{
   //return mpi_shm_send(test_handle, (const void *)message, strlen(message)+1);   
   return mpi_shm_interrupt_send((const void *)message, strlen(message)+1);   
}

int mpi_shm_test_get(char *message, unsigned length)
{
   //return mpi_shm_get(test_handle,(void *)message, length, 0);
   return mpi_shm_interrupt_get((void *)message, length);
}


struct Mpi_channel *mpi_create_channel(unsigned number, unsigned length,mpi_channel_callback_func callback)
{
   struct Mpi_channel *ret = 0;
   int core = get_cpuid();
   unsigned try_length = length;
   int err = 0;

   if((!length)||(!callback)) return ret;
   
   ret = (struct Mpi_channel *)kmalloc(sizeof(struct Mpi_channel), GFP_KERNEL);
   if(ret)
   {
      memset((void *)ret, 0, sizeof(*ret));

      err = Mpi_mutex_init(&ret->m_lock,MPI_SYSTEM_LOCK_DEFAULT_CHANNEL+number);
      if(err!=0)
      {
         printk("%s: can not init lock for mpi channel[%d]\n", __FUNCTION__,number);
         kfree((void *)ret);
         ret = 0;         
         return ret;
      }

      if(core)
      {
         sprintf(ret->m_send_name,"_mpi_channel_%d_down",number);
         sprintf(ret->m_recv_name,"_mpi_channel_%d_up",number);
         
      }
      else
      {
         sprintf(ret->m_send_name,"_mpi_channel_%d_up",number);
         sprintf(ret->m_recv_name,"_mpi_channel_%d_down",number);         
      }

      ret->m_send_handler = mpi_shm_open(ret->m_send_name,&try_length,0);   
      if(ret->m_send_handler<0) 
      {
         printk("%s: failed open share memory for sending\n", __FUNCTION__);
         Mpi_mutex_destroy(&ret->m_lock);
         kfree((void *)ret);
         ret = 0;                  
         return ret;
      }
      ret->m_send_length = try_length;
      ret->m_channel = number;
      debug_printk("MPI send channel[%s:%d] handler[%d],length[%d]\n",
         ret->m_send_name, number,ret->m_send_handler,ret->m_send_length);

      try_length = length;
      ret->m_recv_handler = mpi_shm_open(ret->m_recv_name,&try_length,0);   
      if(ret->m_recv_handler<0)
      {
         printk("%s: failed open share memory for receiving\n", __FUNCTION__);
         mpi_shm_close(ret->m_send_handler);
         Mpi_mutex_destroy(&ret->m_lock);
         kfree((void *)ret);
         ret = 0;                  
         return ret;
      }

      ret->m_recv_length = try_length;   
      ret->m_message_slot = 1-core;      

      debug_printk("MPI receive channel[%s:%d] handler[%d],length[%d]\n",
         ret->m_recv_name, number,ret->m_recv_handler,ret->m_recv_length);   

      ret->m_callback = callback;
      message_int_reg(ret);
      
   }

   return ret;
}

struct Mpi_channel *mpi_create_channel_with_data(unsigned number, unsigned length,mpi_channel_callback_func callback, void *data, char *buffer)
{
   struct Mpi_channel *ret = mpi_create_channel(number,length,callback);
   if(ret)
   {
      ret->m_private_data = data;
      ret->m_buffer = buffer;
   }

   return ret;
}

int mpi_destroy_channel(struct Mpi_channel *channel)
{
   int ret = 0;
   if(!channel) return -1;

   message_int_unreg(channel);
   mpi_shm_close(channel->m_send_handler);
   mpi_shm_close(channel->m_recv_handler);
   Mpi_mutex_destroy(&channel->m_lock);   
   kfree((void *)channel);
   
   return ret;
}

int mpi_channel_send(struct Mpi_channel *channel, const void *data, unsigned length)
{
   int ret = -1;
   int shm_handle;

   assert(channel);
   shm_handle = channel->m_send_handler;

   debug_printk("data[%p], length[0x%x:0x%x],handle[%d]\n", data,length,channel->m_send_length,shm_handle);
   assert(shm_handle >= 0);
   assert(channel->m_send_length);

   if(channel->m_send_length<length)
   {
      printk("%s: message over size[%d > %d]\n", __FUNCTION__, length, channel->m_send_length);
      return ret;
   }
   
   ret = mpi_shm_send(shm_handle, data, length);
   mb();	
   message_write(channel->m_message_slot, shm_handle);

   return ret;
}

/**
* dump mulitple lines in Hex and ASCII
*/
void dump_hex(const unsigned char *buffer, unsigned int size)
{
   unsigned int i, j = 0, WID = 16;
   unsigned char c;
   
   while (j*WID < size) 
   {
      printk("  %04x:  ",j*WID);
      
      for (i=0; i<WID; i++) 
      {
         if ( (i+j*WID) >= size) break;
         c = buffer[i+j*WID];
         printk("%02x ",(int)c);
      }
      printk("\n");

      printk("  %04x:  ",j*WID);
      for (i=0; i<WID; i++) 
      {
         if ( (i+j*WID) >= size) break;
         c = buffer[i+j*WID];
         printk(" %c ",isprint((int)c) ?c:'.');
      }
      printk("\n");
      j++;
   }
}


EXPORT_SYMBOL(mpi_create_channel);
EXPORT_SYMBOL(mpi_destroy_channel);
EXPORT_SYMBOL(mpi_shm_get);
EXPORT_SYMBOL(mpi_create_channel_with_data);
EXPORT_SYMBOL(mpi_channel_send);
EXPORT_SYMBOL(dump_hex);
