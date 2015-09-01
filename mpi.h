
#ifndef MPI_H
#define MPI_H

#ifdef __cplusplus
extern          "C" {
#endif


#define MPI_SYSTEM_START_ADDRESS   0xd0000000
#define MPI_SHM_ADDRESS_OFFSET  0x00100000
#define MPI_SHM_SIZE 0x06000000

#define MPI_SHM_LOCK_SIZE 0x00100000

#define MPI_SYSTEM_LOCK_CONTROL  0
#define MPI_SYSTEM_LOCK_SHM  500
#define MPI_SYSTEM_LOCK_DEFAULT_CHANNEL  501
#define MPI_SYSTEM_LOCK_CORE0  554
#define MPI_SYSTEM_LOCK_SYNC_SHM  555
#define MPI_SYSTEM_LOCK_SHM_NUMBER  600


struct System_lock
{
   unsigned m_lock;
};

unsigned get_system_locks(void);

void system_enter_region(unsigned region);
void system_leave_region(unsigned region);

void system_lock(unsigned lock);
void system_unlock(unsigned lock);


void shm_fill_test(unsigned number, unsigned offset);
void shm_fill_test_nolock(unsigned number, unsigned offset);

struct Mpi_mutex
{
   unsigned m_lock;
};


void init_mpi(void *system_start, unsigned shm_offset,unsigned shm_size,unsigned core);
int Mpi_mutex_init(struct Mpi_mutex *mutex, unsigned lock);
int Mpi_mutex_destroy(struct Mpi_mutex *mutex);
void Mpi_lock(struct Mpi_mutex *mutex);
void Mpi_unlock(struct Mpi_mutex *mutex);

int init_shm(void);

#ifdef __cplusplus
}
#endif

   
#endif 

