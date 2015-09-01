
#include "atomic_ppc.h"

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <asm/reg.h>
#include <asm/io.h>
#include <sysdev/fsl_soc.h>
#include <asm/udbg.h>

#include <linux/mpishm.h>
#include <linux/mpi.h>

//#define DEBUG_MPI

#ifdef DEBUG_MPI
#define debug_printk(X...) { printk("----->%s: ",__FUNCTION__); printk (X); printk ("\n\r"); }
#else
#define debug_printk(X...)
#endif

static void *s_system_start = 0;
static void *s_shm_start = 0;
static unsigned s_shm_offset = 0;
static unsigned s_shm_size = 0;
static unsigned s_system_regions = 0;
static unsigned s_total_length = 0;

struct Mpi_mutex upgrade_lock;
struct Mpi_mutex sync_shm_lock;

#define LOCK_KEEPER_LENGTH  (256*1024)
static char s_lock_keeper[LOCK_KEEPER_LENGTH];

void init_mpi(void *system_start, unsigned shm_offset,unsigned shm_size,unsigned core)
{
   unsigned total_length = 0;
   shm_offset &= 0xfffffff0;   
   assert(system_start&&shm_offset);
   assert(shm_size > MPI_SHM_LOCK_SIZE);
   
   total_length = shm_offset + shm_size;   

   if(!core)memset((void *)system_start, 0, total_length);

   s_system_start = system_start;
   s_shm_offset = shm_offset;
   s_shm_size = shm_size;
   s_system_regions = shm_offset/(sizeof(struct System_lock));
   s_total_length = total_length;
   s_shm_start = (void *)((unsigned)system_start + shm_offset);

   debug_printk("%s: system[%p:0x%x:0x%x],shm[%p:0x%x]\n",__FUNCTION__,
      s_system_start,s_shm_offset,s_system_regions,s_shm_start,s_shm_size);

   if(!core)mod_init((char *)s_shm_start,s_shm_size);
   
   //memset((void *)s_lock_keeper, LOCK_KEEPER_LENGTH);
   
}

int init_shm(void)
{
   return mod_init((char *)s_shm_start,s_shm_size);
}

int Mpi_mutex_init(struct Mpi_mutex *mutex, unsigned lock)
{
   int ret = 0;
   assert(mutex);
   assert(lock<LOCK_KEEPER_LENGTH);
   
   if(s_lock_keeper[lock]) return -1;
   
   s_lock_keeper[lock] = 1;
   mutex->m_lock = lock;
   
   return ret;
}

int Mpi_mutex_destroy(struct Mpi_mutex *mutex)
{
   int ret = 0;
   assert(mutex);
   assert(mutex->m_lock<LOCK_KEEPER_LENGTH);
   
   s_lock_keeper[mutex->m_lock] = 0;
   
   return ret;
}

void Mpi_lock(struct Mpi_mutex *mutex)
{
   assert(mutex);
   system_lock(mutex->m_lock);
   
}

void Mpi_unlock(struct Mpi_mutex *mutex)
{
   assert(mutex);
   system_unlock(mutex->m_lock);
}


unsigned get_system_locks(void)
{
   return s_system_regions;
}
   
void system_lock(unsigned lock)
{
   struct System_lock *plock = 0;
   
   //printk("system lock: lock[%d], max[%d]\n", lock, s_system_regions);
   assert(lock<s_system_regions);
   plock = ((struct System_lock *)s_system_start) + lock;
   debug_printk("%s: lock[%d], plock[%p:%p]\n",__FUNCTION__, lock, plock, &plock->m_lock);

   while(atomic_test_and_set((int *)plock)!=0);
}

void system_unlock(unsigned lock)
{
   struct System_lock *plock = 0;
   assert(lock<s_system_regions);
   plock = ((struct System_lock *)s_system_start) + lock;
   debug_printk("%s: lock[%d], plock[%p:%p:0x%x]\n",__FUNCTION__, lock, plock, &plock->m_lock,plock->m_lock);

   plock->m_lock = 0;
}


void system_enter_region(unsigned region)
{
   system_lock(region);
}

void system_leave_region(unsigned region)
{
   system_unlock(region);
}

void test_delay(int de)
{
   int i,j;
   for(i=0; i<80000; ++i)
   {
      for(j=0;j<de;++j);         
   }
}

void shm_fill_test(unsigned number, unsigned offset)
{
   unsigned *pshm = (unsigned *)s_shm_start + offset;
   unsigned value = *pshm;
   unsigned lock = 0;
   unsigned i;

   debug_printk("%s: filling [%d] to [%d:%p], starting[0x%x:%d]\n", __FUNCTION__,number, offset, pshm,value,value);
   for(i=0;i<number;++i)
   {
      system_lock(lock);
      value = *pshm;
      ++value;
      *pshm = value;
      //++(*pshm);
      system_unlock(lock);      
      test_delay(1);
   }
   value = *pshm;
   debug_printk("%s: final value[0x%x:%d]\n",__FUNCTION__, value,value);

}

void shm_fill_test_nolock(unsigned number, unsigned offset)
{
   unsigned *pshm = (unsigned *)s_shm_start + offset;
   unsigned value = *pshm;
   unsigned i;

   debug_printk("%s: filling [%d] to [%d:%p], starting[0x%x:%d]\n", __FUNCTION__,number, offset, pshm,value,value);
   for(i=0;i<number;++i)
   {
      value = *pshm;
      ++value;
      *pshm = value;
      //++(*pshm);
      test_delay(1);
   }
   value = *pshm;
   debug_printk("%s: final value[0x%x:%d]\n",__FUNCTION__, value,value);

}

EXPORT_SYMBOL(upgrade_lock);
EXPORT_SYMBOL(sync_shm_lock);

