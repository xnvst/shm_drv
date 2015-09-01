

#ifndef DEV_SHM_H
#define DEV_SHM_H

//#include <linux/ioctl.h>

enum Shm_mode
{
   SHM_MODE_NONE = 0,
   SHM_MODE_START,
   SHM_MODE_PAUSE,
   SHM_MODE_STOP,
   SHM_MODE_SERVICE,
   SHM_MODE_DEBUG
};


#define SHM_SET_MODE _IOW('s', 0, unsigned long)
#define SHM_SERVICE _IOW('s', 1, unsigned long)
#define SHM_UNLOCK_CORE0 _IOW('s', 2, unsigned long)
#define SHM_LOCK_SYNC _IOW('s', 3, unsigned long)
#define SHM_UNLOCK_SYNC _IOW('s', 4, unsigned long)	 
#define SHM_UNLOCK_SYNC_NEED _IOW('s', 9, unsigned long)	

int sync_shm_lock_func(int lock);

#endif

