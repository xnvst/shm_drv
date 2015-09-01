

#ifndef _SHM_H_
#define _SHM_H_

#include "mpi.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ntohs
#define ntohs(x) (x)
#endif


#define WU_SIGNATURE      0xB7C529DA
#define assert(x) BUG_ON(!(x))

struct Wu_mem
{
   int m_signature;
   int m_length;
   void *m_pdata;
};

void *wu_malloc(unsigned size);
void wu_free(void *ptr);

#define MOD_SIGNATURE      0x2FA0978C
#define MOD_MEMORY_BUSY_FLAG   (1<<0)

#define SHM_NAME_MAX_LENGTH 32
struct Mod_list;
struct Mod_mem
{
   int m_signature;
   char *m_heap;
   int m_length;
   unsigned m_order;
   unsigned m_number;
   int m_flag;
   struct Mod_list *m_list;
   void *m_pdata;
};

#define SHM_INFO_BLOCK_SIGNATURE      0x6AE528C1
#define SHM_BLOCK_ALLOCATED_FLAG   (1<<0)
struct Shm_block_info
{
   int m_signature;
   int m_id;
   unsigned m_flag;
   unsigned m_status;
   unsigned m_length;
   void *m_mem;
   struct Shm_block_info *m_previous;
   struct Shm_block_info *m_list_next;
   char m_name[SHM_NAME_MAX_LENGTH];
   unsigned m_open;
   unsigned m_update;
   unsigned m_size;
   unsigned m_offset;
   int m_reserve[4];
};

struct Mod_list
{
   struct Mod_mem *m_list_data;
   struct Mod_list *m_list_next;   
   struct Mod_list *m_previous;
};

struct Mod_list_head
{
   struct Mod_list *m_list_next;
   unsigned m_number;
};

struct Mod_heap{
   int m_number;
   int m_block_size;
   unsigned m_order;
   char *m_head;
   char *m_end;
   struct Mod_list *m_list;
   struct Mpi_mutex m_lock;
   struct Mod_list_head m_free;
   struct Mod_list_head m_busy;
};

struct Shm_info_head
{
   struct Shm_block_info *m_list_next;
   unsigned m_number;
};

#define MAX_POOL_NUMBER 100
struct Shm_control_area
{
   unsigned m_init_flag;
   unsigned m_start_pool_order;
   unsigned m_total_pools;
   unsigned m_total_blocks;
   struct Shm_info_head m_free;
   struct Shm_info_head m_busy;   
   struct Shm_block_info *m_block_start;
   unsigned m_pool_array[MAX_POOL_NUMBER];
   unsigned m_reserve[100];
};

#define STARTING_POOL_ORDER    10            
#define TOTAL_POOL_NUMBER       15

struct Pool_control
{
   unsigned m_number;
   struct Mod_list *m_list;
};


int mod_init(char *start, unsigned length);
void *mod_malloc(unsigned size);
void mod_free(void *ptr);
int mod_mem_stat(unsigned pool);
int mod_mem_stat_all(void);


void print_control_info(void);

int mpi_shm_open(const char *name, unsigned *length, unsigned flag);
int mpi_shm_close(int shm_handle);
void *mpi_shm_get_mem(int shm_handle);
int mpi_shm_send(int shm_handle, const void *data, unsigned length);
int mpi_shm_get(int shm_handle, void *data, unsigned length, unsigned update);
int mpi_shm_send_offset(int shm_handle, const void *data, unsigned length,unsigned offset);
int mpi_shm_get_offset(int shm_handle, void *data, unsigned length, unsigned update,unsigned offset);

#define MPI_SHM_OPEN_ERR   -1
#define MPI_SHM_OPEN_LENGTH_ERR   -2
#define MPI_SHM_OPEN_BEFORE   -3
#define MPI_SHM_OPEN_MALLOC_ERR   -4
#define MPI_SHM_NOT_OPEN   -5
#define MPI_SHM_NULL   -6
#define MPI_SHM_NOT_UPDATED   -7
#define MPI_SHM_INVALID   -8
#define MPI_SHM_ERROR   -9

#define MPI_SHM_TEST_NAME  "mpi_shm_test"
#define MPI_SHM_TEST_LENGTH  2000
int mpi_shm_get_test_handle(void);
int mpi_shm_test_send(char *message);
int mpi_shm_test_get(char *message, unsigned length);

int mpi_default_channel_init(void);


typedef int (*mpi_channel_callback_func)(void *);

struct Mpi_channel
{
   char m_send_name[SHM_NAME_MAX_LENGTH];
   char m_recv_name[SHM_NAME_MAX_LENGTH];
   int m_send_handler;
   int m_recv_handler;
   unsigned m_send_length;
   unsigned m_recv_length;
   struct Mpi_mutex m_lock;
   int m_message_slot;
   unsigned m_channel;
   mpi_channel_callback_func m_callback;
   void *m_private_data;
   char *m_buffer;
   struct Mpi_channel *m_next;
};

struct Mpi_channel *mpi_create_channel(unsigned number, unsigned length,mpi_channel_callback_func callback);
int mpi_destroy_channel(struct Mpi_channel *channel);
struct Mpi_channel *mpi_create_channel_with_data(unsigned number, unsigned length,mpi_channel_callback_func callback, void *data, char *buffer);
int mpi_channel_send(struct Mpi_channel *channel, const void *data, unsigned length);
void dump_hex(const unsigned char *buffer, unsigned int size);
#ifdef __cplusplus
}
#endif

#endif 


