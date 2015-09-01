#ifndef UTILS_H
#define UTILS_H

#include "fsl_mpc86xx_reg.h"
#include "fsl_mpc86xx_law_reg.h"
#include <linux/mpishm.h>

#define CCSR_BASE       0xe0000000

// SPR registers
#define IBAT0U  528
#define IBAT0L  529
#define IBAT1U  530
#define IBAT1L  531
#define IBAT2U  532
#define IBAT2L  533
#define IBAT3U  534
#define IBAT3L  535

#define IBAT4U  560
#define IBAT4L  561
#define IBAT5U  562
#define IBAT5L  563
#define IBAT6U  564
#define IBAT6L  565
#define IBAT7U  566
#define IBAT7L  567

#define DBAT0U  536
#define DBAT0L  537
#define DBAT1U  538
#define DBAT1L  539
#define DBAT2U  540
#define DBAT2L  541
#define DBAT3U  542
#define DBAT3L  543

#define DBAT4U  568
#define DBAT4L  569
#define DBAT5U  570
#define DBAT5L  571
#define DBAT6U  572
#define DBAT6L  573
#define DBAT7U  574
#define DBAT7L  575

#define HID0   1008
#define HID1   1009

#define ICTRL   1011
#define MSSCR0  1014
#define MSSSR0  1015
#define LDSTCR  1016
#define L2CR    1017

// UART registers
#define URBR(port)  (0x4500 + 0x100 * (port))
#define UTHR(port)  (0x4500 + 0x100 * (port))
#define UDLB(port)  (0x4500 + 0x100 * (port))
#define UIER(port)  (0x4501 + 0x100 * (port))
#define UDMB(port)  (0x4501 + 0x100 * (port))
#define UIIR(port)  (0x4502 + 0x100 * (port))
#define UFCR(port)  (0x4502 + 0x100 * (port))
#define UAFR(port)  (0x4502 + 0x100 * (port))
#define ULCR(port)  (0x4503 + 0x100 * (port))
#define UMCR(port)  (0x4504 + 0x100 * (port))
#define ULSR(port)  (0x4505 + 0x100 * (port))
#define UMSR(port)  (0x4506 + 0x100 * (port))
#define USCR(port)  (0x4507 + 0x100 * (port))
#define UDSR(port)  (0x4510 + 0x100 * (port))

//POR registers, GU_BASE
#define GU_PORPLLSR    (0x000 + GU_BASE)
#define GU_PORBMSR     (0x004 + GU_BASE)
#define GU_PORIMPCR   (0x008 + GU_BASE)
#define GU_PORDEVSR   (0x00c + GU_BASE)
#define GU_PORDBGMSR (0x010 + GU_BASE)
#define GU_PORCIR        (0x020 + GU_BASE)
#define GU_RSTRSCR (0x094 + GU_BASE)
#define GU_RSTCR    (0x0b0 + GU_BASE)

//Local configuration control registers, LA_BASE
#define CCSRBAR    (0x000 + LA_BASE)
#define ALTCBAR    (0x008 + LA_BASE)
#define ALTCAR      (0x010 + LA_BASE)
#define BPTR          (0x020 + LA_BASE)   
#define LA_LAWBAR0   (LAWBAR0 + LA_BASE)

//Memory Map/Register 
#define ABCR         (0x000 + MCM_BASE)
#define DBCR         (0x008 + MCM_BASE)
#define PCR           (0x010 + MCM_BASE)

//PIC registers
#define PIC_PIR    (0x01090 + PIC_BASE)
#define PIC_PRR    (0x01098 + PIC_BASE)

#define SPR_PIR	     0x3FF

unsigned get_cpuid(void);
void disable_cpu(int core,bool disable);

//void print_spr(unsigned spr);
void print_reg(unsigned reg);
unsigned mpc8640_in32(unsigned a);


#define PRINT_SPR(spr)                                          \
    do {                                                        \
        unsigned val;                                           \
                                                                \
        __asm__ volatile ("mfspr %0,%1" : "=r" (val) : "I" (spr));       \
        printk("%s = %08x\n", #spr, val);          \
    } while(0)

#define PRINT_REG_PIC(reg)  PRINT_REG((reg)+PIC_BASE)

void print_registers(void);
void delay(int de);
void print_pic_registers(void);

void reset_system(void);
void hreset_peer(void);
void sreset_peer(void);

int do_boot(const char *bootpath,char * const cmd_argv[], const char *pfdt);
void print_envs(void);
int duart_enable_interrupt(unsigned int);

int utils_init(void);
void print_spr_registers(void);

void message_int_init(unsigned message);
int message_read(unsigned slot);
int message_int_reg(struct Mpi_channel *channel);
int message_int_unreg(struct Mpi_channel *channel);

void dcache_enable(void);
typedef int (*entry_t)(int, char *const [], char *const []);
int ppc_jump(int, char **, char **, unsigned *,unsigned *,entry_t, unsigned *);
int message_write(unsigned slot, int message);

/*
 * Memory Command Flags:
 */
#define CMD_FLAG_REPEAT		0x0001	/* repeat last command		*/
#define CMD_FLAG_BOOTD		0x0002	/* command is from bootd	*/

int do_mem_md ( int flag, int argc, char *argv[]);
int do_mem_mm (int flag, int argc, char *argv[]);

#define FLASH_SYNC_MSGR		3

#endif /* UTILS_H */

