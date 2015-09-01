
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <asm/reg.h>
#include <asm/io.h>
#include <sysdev/fsl_soc.h>
#include <asm/udbg.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <asm/semaphore.h>
#include <linux/mpishm.h>
#include <linux/mpi.h>
#include <linux/module.h> 

#include "utils.h"

#include "irq_mpc864x.h"
#include "priority.h"
#include "pic_reg.h"



extern char *readline(const char *prompt,char *str,int max);

static void __iomem *immr_base;
#define PRINT_REG(reg)                                                  \
    do {                                                                \
        unsigned r;                                                     \
        unsigned val;                                                   \
                                                                        \
        r = reg;                                                        \
        val = in_be32(immr_base+r);                                          \
        printk("%s @ %08x = %08x\n", #reg, r, val);        \
    } while (0)

    
static __inline__ unsigned
mpc8640_in8(unsigned a)
{
    return in_8(immr_base + a);
}

unsigned
mpc8640_in32(unsigned a)
{
    return in_be32(immr_base+a);
}

static __inline__ void
mpc8640_out32(unsigned a,
              unsigned d)
{
    out_be32(immr_base+a, d);
}

static __inline__ void
mpc8640_update32(unsigned a,
                 unsigned mask,
                 unsigned value)
{
    unsigned orig;
    unsigned mod;

    orig = mpc8640_in32(a);
    mod = (orig & ~mask) | (value & mask);
    mpc8640_out32(a, mod);
}

static __inline__ void
mpc8640_out8(unsigned a,unsigned d)
{
    out_8(immr_base+a, d);
}

unsigned get_cpuid(void)
{
   unsigned cpuid;
   __asm__ volatile ("mfspr %0,%1" : "=r" (cpuid) : "i" (SPR_PIR));

   return cpuid;
}


void delay(int de)
{
   int i, j;   
   for(i=0; i<8000000; ++i)
   {
      for(j=0;j<de;++j);         
   }
}


#if 0
void print_spr(unsigned spr)
{
   unsigned val;                                                                          
   //__asm__ volatile ("mfspr %0,%1" : "=r" (val) : "I" (spr));       
   __asm__ volatile ("mfspr %0,%1" : "=r" (val) : "i" ());
   printk("SPR register[%d] = %08x\n", spr, val); 
}
#endif

void print_pic_registers(void)
{
    PRINT_REG_PIC(BRR1);
    PRINT_REG_PIC(BRR2);
    PRINT_REG_PIC(IPIDR0);
    PRINT_REG_PIC(IPIDR1);
    PRINT_REG_PIC(IPIDR2);
    PRINT_REG_PIC(IPIDR3);
    PRINT_REG_PIC(CTPR);
    PRINT_REG_PIC(WHOAMI);
    PRINT_REG_PIC(IACK);
    PRINT_REG_PIC(EOI);
    PRINT_REG_PIC(FRR);
    PRINT_REG_PIC(GCR);
    PRINT_REG_PIC(VIR);
    PRINT_REG_PIC(PIR);
    PRINT_REG_PIC(PRR);
    PRINT_REG_PIC(IPIVPR0);
    PRINT_REG_PIC(IPIVPR1);
    PRINT_REG_PIC(IPIVPR2);
    PRINT_REG_PIC(IPIVPR3);
    PRINT_REG_PIC(SVR);
    PRINT_REG_PIC(TFRRA);
    PRINT_REG_PIC(GTCCRA0);
    PRINT_REG_PIC(GTBCRA0);
    PRINT_REG_PIC(GTVPRA0);
    PRINT_REG_PIC(GTDRA0);
    PRINT_REG_PIC(GTCCRA1);
    PRINT_REG_PIC(GTBCRA1);
    PRINT_REG_PIC(GTVPRA1);
    PRINT_REG_PIC(GTDRA1);
    PRINT_REG_PIC(GTCCRA2);
    PRINT_REG_PIC(GTBCRA2);
    PRINT_REG_PIC(GTVPRA2);
    PRINT_REG_PIC(GTDRA2);
    PRINT_REG_PIC(GTCCRA3);
    PRINT_REG_PIC(GTBCRA3);
    PRINT_REG_PIC(GTVPRA3);
    PRINT_REG_PIC(GTDRA3);
    PRINT_REG_PIC(TCRA);
    PRINT_REG_PIC(ERQSR);
    PRINT_REG_PIC(IRQSR0);
    PRINT_REG_PIC(IRQSR1);
    PRINT_REG_PIC(IRQSR2);
    PRINT_REG_PIC(CISR0);
    PRINT_REG_PIC(CISR1);
    PRINT_REG_PIC(CISR2);
    PRINT_REG_PIC(PM0MR0);
    PRINT_REG_PIC(PM0MR1);
    PRINT_REG_PIC(PM0MR2);
    PRINT_REG_PIC(PM1MR0);
    PRINT_REG_PIC(PM1MR1);
    PRINT_REG_PIC(PM1MR2);
    PRINT_REG_PIC(PM2MR0);
    PRINT_REG_PIC(PM2MR1);
    PRINT_REG_PIC(PM2MR2);
    PRINT_REG_PIC(PM3MR0);
    PRINT_REG_PIC(PM3MR1);
    PRINT_REG_PIC(PM3MR2);
    PRINT_REG_PIC(MSGR0);
    PRINT_REG_PIC(MSGR1);
    PRINT_REG_PIC(MSGR2);
    PRINT_REG_PIC(MSGR3);
    PRINT_REG_PIC(MER);
    PRINT_REG_PIC(MSR);
    PRINT_REG_PIC(MSIR0);
    PRINT_REG_PIC(MSIR1);
    PRINT_REG_PIC(MSIR2);
    PRINT_REG_PIC(MSIR3);
    PRINT_REG_PIC(MSIR4);
    PRINT_REG_PIC(MSIR5);
    PRINT_REG_PIC(MSIR6);
    PRINT_REG_PIC(MSIR7);
    PRINT_REG_PIC(MSISR);
    PRINT_REG_PIC(MSIIR);
    PRINT_REG_PIC(TFRRB);
    PRINT_REG_PIC(GTCCRB0);
    PRINT_REG_PIC(GTBCRB0);
    PRINT_REG_PIC(GTVPRB0);
    PRINT_REG_PIC(GTDRB0);
    PRINT_REG_PIC(GTCCRB1);
    PRINT_REG_PIC(GTBCRB1);
    PRINT_REG_PIC(GTVPRB1);
    PRINT_REG_PIC(GTDRB1);
    PRINT_REG_PIC(GTCCRB2);
    PRINT_REG_PIC(GTBCRB2);
    PRINT_REG_PIC(GTVPRB2);
    PRINT_REG_PIC(GTDRB2);
    PRINT_REG_PIC(GTCCRB3);
    PRINT_REG_PIC(GTBCRB3);
    PRINT_REG_PIC(GTVPRB3);
    PRINT_REG_PIC(GTDRB3);
    PRINT_REG_PIC(TCRB);
    PRINT_REG_PIC(EIVPR0);
    PRINT_REG_PIC(EIDR0);
    PRINT_REG_PIC(EIVPR1);
    PRINT_REG_PIC(EIDR1);
    PRINT_REG_PIC(EIVPR2);
    PRINT_REG_PIC(EIDR2);
    PRINT_REG_PIC(EIVPR3);
    PRINT_REG_PIC(EIDR3);
    PRINT_REG_PIC(EIVPR4);
    PRINT_REG_PIC(EIDR4);
    PRINT_REG_PIC(EIVPR5);
    PRINT_REG_PIC(EIDR5);
    PRINT_REG_PIC(EIVPR6);
    PRINT_REG_PIC(EIDR6);
    PRINT_REG_PIC(EIVPR7);
    PRINT_REG_PIC(EIDR7);
    PRINT_REG_PIC(EIVPR8);
    PRINT_REG_PIC(EIDR8);
    PRINT_REG_PIC(EIVPR9);
    PRINT_REG_PIC(EIDR9);
    PRINT_REG_PIC(EIVPR10);
    PRINT_REG_PIC(EIDR10);
    PRINT_REG_PIC(EIVPR11);
    PRINT_REG_PIC(EIDR11);
    PRINT_REG_PIC(IIVPR0);
    PRINT_REG_PIC(IIDR0);
    PRINT_REG_PIC(IIVPR1);
    PRINT_REG_PIC(IIDR1);
    PRINT_REG_PIC(IIVPR2);
    PRINT_REG_PIC(IIDR2);
    PRINT_REG_PIC(IIVPR3);
    PRINT_REG_PIC(IIDR3);
    PRINT_REG_PIC(IIVPR4);
    PRINT_REG_PIC(IIDR4);
    PRINT_REG_PIC(IIVPR5);
    PRINT_REG_PIC(IIDR5);
    PRINT_REG_PIC(IIVPR6);
    PRINT_REG_PIC(IIDR6);
    PRINT_REG_PIC(IIVPR7);
    PRINT_REG_PIC(IIDR7);
    PRINT_REG_PIC(IIVPR8);
    PRINT_REG_PIC(IIDR8);
    PRINT_REG_PIC(IIVPR9);
    PRINT_REG_PIC(IIDR9);
    PRINT_REG_PIC(IIVPR10);
    PRINT_REG_PIC(IIDR10);
    PRINT_REG_PIC(IIVPR11);
    PRINT_REG_PIC(IIDR11);
    PRINT_REG_PIC(IIVPR12);
    PRINT_REG_PIC(IIDR12);
    PRINT_REG_PIC(IIVPR13);
    PRINT_REG_PIC(IIDR13);
    PRINT_REG_PIC(IIVPR14);
    PRINT_REG_PIC(IIDR14);
    PRINT_REG_PIC(IIVPR15);
    PRINT_REG_PIC(IIDR15);
    PRINT_REG_PIC(IIVPR16);
    PRINT_REG_PIC(IIDR16);
    PRINT_REG_PIC(IIVPR17);
    PRINT_REG_PIC(IIDR17);
    PRINT_REG_PIC(IIVPR18);
    PRINT_REG_PIC(IIDR18);
    PRINT_REG_PIC(IIVPR19);
    PRINT_REG_PIC(IIDR19);
    PRINT_REG_PIC(IIVPR20);
    PRINT_REG_PIC(IIDR20);
    PRINT_REG_PIC(IIVPR21);
    PRINT_REG_PIC(IIDR21);
    PRINT_REG_PIC(IIVPR22);
    PRINT_REG_PIC(IIDR22);
    PRINT_REG_PIC(IIVPR23);
    PRINT_REG_PIC(IIDR23);
    PRINT_REG_PIC(IIVPR24);
    PRINT_REG_PIC(IIDR24);
    PRINT_REG_PIC(IIVPR25);
    PRINT_REG_PIC(IIDR25);
    PRINT_REG_PIC(IIVPR26);
    PRINT_REG_PIC(IIDR26);
    PRINT_REG_PIC(IIVPR27);
    PRINT_REG_PIC(IIDR27);
    PRINT_REG_PIC(IIVPR28);
    PRINT_REG_PIC(IIDR28);
    PRINT_REG_PIC(IIVPR29);
    PRINT_REG_PIC(IIDR29);
    PRINT_REG_PIC(IIVPR30);
    PRINT_REG_PIC(IIDR30);
    PRINT_REG_PIC(IIVPR31);
    PRINT_REG_PIC(IIDR31);
    PRINT_REG_PIC(IIVPR32);
    PRINT_REG_PIC(IIDR32);
    PRINT_REG_PIC(IIVPR33);
    PRINT_REG_PIC(IIDR33);
    PRINT_REG_PIC(IIVPR34);
    PRINT_REG_PIC(IIDR34);
    PRINT_REG_PIC(IIVPR35);
    PRINT_REG_PIC(IIDR35);
    PRINT_REG_PIC(IIVPR36);
    PRINT_REG_PIC(IIDR36);
    PRINT_REG_PIC(IIVPR37);
    PRINT_REG_PIC(IIDR37);
    PRINT_REG_PIC(IIVPR38);
    PRINT_REG_PIC(IIDR38);
    PRINT_REG_PIC(IIVPR39);
    PRINT_REG_PIC(IIDR39);
    PRINT_REG_PIC(IIVPR40);
    PRINT_REG_PIC(IIDR40);
    PRINT_REG_PIC(IIVPR41);
    PRINT_REG_PIC(IIDR41);
    PRINT_REG_PIC(IIVPR42);
    PRINT_REG_PIC(IIDR42);
    PRINT_REG_PIC(IIVPR43);
    PRINT_REG_PIC(IIDR43);
    PRINT_REG_PIC(IIVPR44);
    PRINT_REG_PIC(IIDR44);
    PRINT_REG_PIC(IIVPR45);
    PRINT_REG_PIC(IIDR45);
    PRINT_REG_PIC(IIVPR46);
    PRINT_REG_PIC(IIDR46);
    PRINT_REG_PIC(IIVPR47);
    PRINT_REG_PIC(IIDR47);
    PRINT_REG_PIC(MIVPR0);
    PRINT_REG_PIC(MIDR0);
    PRINT_REG_PIC(MIVPR1);
    PRINT_REG_PIC(MIDR1);
    PRINT_REG_PIC(MIVPR2);
    PRINT_REG_PIC(MIDR2);
    PRINT_REG_PIC(MIVPR3);
    PRINT_REG_PIC(MIDR3);
    PRINT_REG_PIC(MSIVPR0);
    PRINT_REG_PIC(MSIDR0);
    PRINT_REG_PIC(MSIVPR1);
    PRINT_REG_PIC(MSIDR1);
    PRINT_REG_PIC(MSIVPR2);
    PRINT_REG_PIC(MSIDR2);
    PRINT_REG_PIC(MSIVPR3);
    PRINT_REG_PIC(MSDIR3);
    PRINT_REG_PIC(MSIVPR4);
    PRINT_REG_PIC(MSIDR4);
    PRINT_REG_PIC(MSIVPR5);
    PRINT_REG_PIC(MSIDR5);
    PRINT_REG_PIC(MSIVPR6);
    PRINT_REG_PIC(MSIDR6);
    PRINT_REG_PIC(MSIVPR7);
    PRINT_REG_PIC(MSDIR7);
    PRINT_REG_PIC(P0_IPIDR0);
    PRINT_REG_PIC(P0_IPIDR1);
    PRINT_REG_PIC(P0_IPIDR2);
    PRINT_REG_PIC(P0_IPIDR3);
    PRINT_REG_PIC(P0_CTPR0);
    PRINT_REG_PIC(P0_WHOAMI0);
    PRINT_REG_PIC(P0_IACK0);
    PRINT_REG_PIC(P0_EOI0);
    PRINT_REG_PIC(P1_IPIDR0);
    PRINT_REG_PIC(P1_IPIDR1);
    PRINT_REG_PIC(P1_IPIDR2);
    PRINT_REG_PIC(P1_IPIDR3);
    PRINT_REG_PIC(P1_CTPR1);
    PRINT_REG_PIC(P1_WHOAMI1);
    PRINT_REG_PIC(P1_IACK1);
    PRINT_REG_PIC(P1_EOI1);
    
}

void print_reg(unsigned reg)
{
   unsigned r;                                                     
   unsigned val;                                                   
                                 
   r = reg;                                                        
   val = mpc8640_in32(r);                                          
   printk("reg[%08x] = %08x\n", r, val);

}

static void dump_sr(void)
{
    unsigned k;
    unsigned sr;

    printk("======= dump sr register =======\n");
    for (k = 0; k < 16; k++) 
    {
        __asm__ volatile ("mfsrin %0,%1" : "=r" (sr) : "r" (k << 28));
        printk("SR%u = %08x\n", k, sr);
    }
}

void print_spr_registers(void)
{
   unsigned msr;
   __asm__ volatile ("mfmsr %0" : "=r" (msr));
      
   printk("SPR registers:\n");
   printk("MSR = %08x\n", msr);          
      
   PRINT_SPR(DBAT0L);
   PRINT_SPR(DBAT0U);
   PRINT_SPR(DBAT1L);
   PRINT_SPR(DBAT1U);
   PRINT_SPR(DBAT2L);
   PRINT_SPR(DBAT2U);
   PRINT_SPR(DBAT3L);
   PRINT_SPR(DBAT3U);
   PRINT_SPR(DBAT4L);
   PRINT_SPR(DBAT4U);
   PRINT_SPR(DBAT5L);
   PRINT_SPR(DBAT5U);
   PRINT_SPR(DBAT6L);
   PRINT_SPR(DBAT6U);
   PRINT_SPR(DBAT7L);
   PRINT_SPR(DBAT7U);

   PRINT_SPR(IBAT0L);
   PRINT_SPR(IBAT0U);
   PRINT_SPR(IBAT1L);
   PRINT_SPR(IBAT1U);
   PRINT_SPR(IBAT2L);
   PRINT_SPR(IBAT2U);
   PRINT_SPR(IBAT3L);
   PRINT_SPR(IBAT3U);
   PRINT_SPR(IBAT4L);
   PRINT_SPR(IBAT4U);
   PRINT_SPR(IBAT5L);
   PRINT_SPR(IBAT5U);
   PRINT_SPR(IBAT6L);
   PRINT_SPR(IBAT6U);
   PRINT_SPR(IBAT7L);
   PRINT_SPR(IBAT7U);

   PRINT_SPR(HID0);
   PRINT_SPR(HID1);

   PRINT_SPR(ICTRL);
   PRINT_SPR(MSSCR0);
   PRINT_SPR(MSSSR0);
   PRINT_SPR(LDSTCR);
   PRINT_SPR(L2CR);
   PRINT_SPR(SPRN_SDR1);

   dump_sr();

}

void print_registers(void)
{
   //int c;
   
   print_spr_registers();

   printk("GU registers:\n");
   PRINT_REG(GU_PORPLLSR);
   PRINT_REG(GU_PORBMSR);
   PRINT_REG(GU_PORIMPCR);
   PRINT_REG(GU_PORDEVSR);
   PRINT_REG(GU_PORDBGMSR);
   PRINT_REG(GU_PORCIR);
   PRINT_REG(GU_RSTRSCR);
   PRINT_REG(GU_RSTCR);

   printk("Local configuration registers:\n");
   PRINT_REG(CCSRBAR);
   PRINT_REG(ALTCBAR);
   PRINT_REG(ALTCAR);
   PRINT_REG(BPTR);
   PRINT_REG(LA_BASE+LAWBAR0);
   PRINT_REG(LA_BASE+LAWAR0);
   PRINT_REG(LA_BASE+LAWBAR1);
   PRINT_REG(LA_BASE+LAWAR1);
   PRINT_REG(LA_BASE+LAWBAR2);
   PRINT_REG(LA_BASE+LAWAR2);
   PRINT_REG(LA_BASE+LAWBAR3);
   PRINT_REG(LA_BASE+LAWAR3);
   PRINT_REG(LA_BASE+LAWBAR4);
   PRINT_REG(LA_BASE+LAWAR4);
   PRINT_REG(LA_BASE+LAWBAR5);
   PRINT_REG(LA_BASE+LAWAR5);
   PRINT_REG(LA_BASE+LAWBAR6);
   PRINT_REG(LA_BASE+LAWAR6);
   PRINT_REG(LA_BASE+LAWBAR7);
   PRINT_REG(LA_BASE+LAWAR7);
   PRINT_REG(LA_BASE+LAWBAR8);
   PRINT_REG(LA_BASE+LAWAR8);
   PRINT_REG(LA_BASE+LAWBAR9);
   PRINT_REG(LA_BASE+LAWAR9);

   printk("Memory map registers:\n");
   PRINT_REG(ABCR);
   PRINT_REG(DBCR);
   PRINT_REG(PCR);   

   printk("PIR registers:\n");
   PRINT_REG(PIC_PRR);
   PRINT_REG(PIC_PIR);

}


void disable_cpu(int core, bool disable)
{
   unsigned value = disable?0:1;
   mpc8640_update32(PCR, 1<<(core+24),value<<(core+24));
}


#define   PRI_MSG   PRI_SERIAL
//PIC registers
#define PIC_PIR    (0x01090 + PIC_BASE)
#define PIC_PRR    (0x01098 + PIC_BASE)
#define PIC_MSGR0    (0x01400 + PIC_BASE)
#define PIC_MER    (0x01500 + PIC_BASE)
#define PIC_MSR    (0x01510 + PIC_BASE)

struct Message_context
{
   int m_vector;
   int m_slot;
};

struct Message_context message_array[4] = 
{
      {0,0},
      {1,1},
      {2,2},
      {3,3},
};

static struct semaphore message_int_sem;

static struct Mpi_channel *message_int_call_stack_head = 0;
int message_int_reg(struct Mpi_channel *channel)
{
   int ret = 0;
   struct Mpi_channel *plink = 0;
   
   assert(channel);
   assert(channel->m_callback);

   channel->m_next = 0;
   down_interruptible(&message_int_sem);  
   plink = message_int_call_stack_head;
   if(plink)
   {
      while(plink->m_next)
      {
         plink = plink->m_next;
      }
      plink->m_next = channel;      
   }
   else
   {
      message_int_call_stack_head = channel;
   }
   up(&message_int_sem);
   
   return ret;
}

int message_int_unreg(struct Mpi_channel *channel)
{
   int ret = 0;
   struct Mpi_channel *plink = 0;
   
   assert(channel);
   assert(channel->m_callback);

   down_interruptible(&message_int_sem);   
   plink = message_int_call_stack_head;
   if(plink==channel)
   {
      message_int_call_stack_head = plink->m_next;
   }
   else
   {
      while(plink)
      {
         if(plink->m_next==channel)
         {
            plink->m_next = channel->m_next;
            break;
         }
         plink = plink->m_next;      
      }
   }
   up(&message_int_sem);
   
   return ret;
}

static irqreturn_t message_isr(int irq, void *vsc)
{
   struct Message_context *pcontext = (struct Message_context *)vsc;
   int msg = 0;
   int reg = 0;
   char buffer[1024];
   int ret = 0;
   int handled = 1;
   struct Mpi_channel *plink = message_int_call_stack_head;
   

   reg = PIC_MSGR0 + 0x10 * pcontext->m_slot;
   msg = mpc8640_in32(reg);

   while(plink)
   {
      if((plink->m_recv_handler==msg)&&(plink->m_callback))
      {
         plink->m_callback((void *)plink);
         break;
      }
      plink = plink->m_next;
   }

   if(!plink)
   {
      ret = mpi_shm_get(msg,(void *)buffer, 1023, 0);
   }

   return IRQ_RETVAL(handled);
}

#ifdef CONFIG_FLASHMSG
extern void cfi_flashmsg_spin_lock(unsigned long lock);

static irqreturn_t flash_sync_message_isr(int irq, void *vsc)
{
   struct Message_context *pcontext = (struct Message_context *)vsc;
   int msg = 0;
   int reg = 0;
   char buffer[1024];
   int ret = 0;
   int handled = 1;

   reg = PIC_MSGR0 + 0x10 * pcontext->m_slot;
   msg = mpc8640_in32(reg);   
   if (msg){
	cfi_flashmsg_spin_lock(1);
   }
   else{
	cfi_flashmsg_spin_lock(0);
   }
   	
   return IRQ_RETVAL(handled);
}
#endif

#define MPIC_MESSAGE_INTERRUPT_BASE 176
void message_int_init(unsigned slot)
{
   int err;
   unsigned irq = MPIC_MESSAGE_INTERRUPT_BASE + slot;
   struct Message_context *pcontext = 0;
   int mer = mpc8640_in32(PIC_MER);
   struct resource res[2];
   struct device_node *pdev = NULL;


   memset(res, 0, sizeof(res));

   pdev = of_find_compatible_node(pdev, "share_memory", "mpi");
   if(!pdev)
   {
      printk("%s: can not find share memory node in device tree\n", __FUNCTION__);
      return;
   }
   
   err = of_address_to_resource(pdev, 0, &res[0]);
   if(err)
   {
      printk("%s: can not map addre to resource, err[%d]\n", __FUNCTION__,err);
   }
   
   pcontext = &message_array[slot];
   
   if (slot!=FLASH_SYNC_MSGR){
		of_irq_to_resource(pdev, 0, &res[1]);
		err = request_irq(irq,message_isr,IRQF_PERCPU,"message mpi",(void *)pcontext);
   }
#ifdef CONFIG_FLASHMSG   
   else if (slot==FLASH_SYNC_MSGR){
		of_irq_to_resource(pdev, 1, &res[1]);
		err = request_irq(irq,flash_sync_message_isr,IRQF_PERCPU,"flash msg",(void *)pcontext);
   }
#endif   
   if(err)
   {
      printk("%s: can not assigned irq: %d, err[%d]\n", __FUNCTION__,irq,err);
      return;
   }
   pcontext->m_vector = irq;
   pcontext->m_slot = slot;

   mer |= 1<<slot;

   mpc8640_out32(PIC_MER, mer);      
   
}

int message_write(unsigned slot, int message)
{
   int ret = 0;
   if(slot>=4)
   {
      printk("%s: message slot[%d] too big, should be less than 4\n",__FUNCTION__, slot);      
      return -1;
   }
   
   mpc8640_out32(PIC_MSGR0+ (0x10 * slot), message);
   return ret;
}

int message_read(unsigned slot)
{
   if(slot>=4)
   {
      printk("%s: message slot[%d] too big, should be less than 4\n",__FUNCTION__, slot);
      return -1;
   }

   return mpc8640_in32(PIC_MSGR0+ (0x10 * slot));
}


/* upper BAT register */
#define BAT_BEPI(z)    ((z) & 0xfffe0000)
#define BAT_BL(z)      ((((z)-1)>>15)&0x0001ffc)
#define BAT_XBL(z)     ((((z)-1)>>15)&0x001fffc)  /* MPC7445/MPC7455 only */
#define BAT_BL_128K    0x00000000
#define BAT_BL_256K    0x00000004
#define BAT_BL_512K    0x0000000c
#define BAT_BL_1M      0x0000001c
#define BAT_BL_2M      0x0000003c
#define BAT_BL_4M      0x0000007c
#define BAT_BL_8M      0x000000fc
#define BAT_BL_16M     0x000001fc
#define BAT_BL_32M     0x000003fc
#define BAT_BL_64M     0x000007fc
#define BAT_BL_128M    0x00000ffc
#define BAT_BL_256M    0x00001ffc
#define BAT_VS         0x00000002   /* supervisor */
#define BAT_VP         0x00000001   /* user */

/* lower BAT register */
#define BAT_BRPN(z)    ((z) & 0xfffe0000)
#define BAT_W          0x00000040
#define BAT_I          0x00000020
#define BAT_M          0x00000010
#define BAT_IM         (BAT_I|BAT_M)
#define BAT_WM         (BAT_W|BAT_M)
#define BAT_G          0x00000008
#define BAT_PP_NA      0x00000000
#define BAT_PP_RO_S    0x00000001
#define BAT_PP_RW      0x00000002
#define BAT_PP_RO      0x00000003

#define SHARE_MEMORY_BASE  0x10000000  
#define SHARE_MEMORY_REAL   0x20000000  
#define SHARE_MEMORY_SIZE  (256 << 20)
#define SHARE_EFFECTIVE_MEMORY_BASE  0xe0000000  
#define SHARE_UPPER_MEMORY_BASE  0x30000000  
#define SHARE_UPPER_MEMORY_REAL   0x30000000  
#define SHARE_UPPER_MEMORY_SIZE  (256 << 20)

int utils_init(void)
{
   int ret = 0;
   unsigned core = get_cpuid();

   immr_base = ioremap(get_immrbase(), 0x100000);   

   init_MUTEX(&message_int_sem);
   
   init_mpi((void *)MPI_SYSTEM_START_ADDRESS,MPI_SHM_ADDRESS_OFFSET,MPI_SHM_SIZE,core);

   delay(1);
   mod_init((char *)(MPI_SYSTEM_START_ADDRESS+MPI_SHM_ADDRESS_OFFSET),MPI_SHM_SIZE);


   message_int_init(core);
#ifdef CONFIG_FLASHMSG   
   message_int_init(FLASH_SYNC_MSGR);  
#endif   
   
   return ret;
}


void reset_system(void)
{
#if 0

#define CFG_RESET_ADDRESS    0xfff00100

   unsigned addr = CFG_RESET_ADDRESS;
   
   //out8(PIXIS_BASE + PIXIS_RST, 0);

	/* flush and disable I/D cache */
	__asm__ __volatile__ ("mfspr	3, 1008"	::: "r3");
	__asm__ __volatile__ ("ori	5, 5, 0xcc00"	::: "r5");
	__asm__ __volatile__ ("ori	4, 3, 0xc00"	::: "r4");
	__asm__ __volatile__ ("andc	5, 3, 5"	::: "r5");
	__asm__ __volatile__ ("sync");
	__asm__ __volatile__ ("mtspr	1008, 4");
	__asm__ __volatile__ ("isync");
	__asm__ __volatile__ ("sync");
	__asm__ __volatile__ ("mtspr	1008, 5");
	__asm__ __volatile__ ("isync");
	__asm__ __volatile__ ("sync");

	__asm__ __volatile__ ("mtspr	26, %0"		:: "r" (addr));
	__asm__ __volatile__ ("li	4, (1 << 6)"	::: "r4");
	__asm__ __volatile__ ("mtspr	27, 4");
	__asm__ __volatile__ ("rfi");
#endif

#if 0
   printk("setting this register[0x%x]\n", GU_RSTCR);
   PRINT_REG(GU_RSTRSCR);
   PRINT_REG(GU_RSTCR);
   
   //mpc8640_update32(GU_RSTCR, 1<<1,1<<1);
   mpc8640_out32(GU_RSTCR, 0x02);

   printk("asserted...\n");
   PRINT_REG(GU_RSTRSCR);
   PRINT_REG(GU_RSTCR);
   delay(30);
   
   mpc8640_out32(GU_RSTCR, 0x00);
   printk("Negated...\n");
   delay(30);
   mpc8640_out32(GU_RSTCR, 0x02);
   printk("asserted again...\n");
#endif

}


void hreset_peer(void)
{
   unsigned core = 1<<(1-get_cpuid());
   printk("%s: to value[%d]\n", __FUNCTION__,core);

   printk("setting this register[0x%x]\n", PIC_PRR);
   PRINT_REG(PIC_PRR);
   
   mpc8640_out32(PIC_PRR, core);

   printk("asserted...\n");
   PRINT_REG(PIC_PRR);
   delay(30);
   
   mpc8640_out32(PIC_PRR, 0x00);
   printk("Negated...\n");
   PRINT_REG(PIC_PRR);
   delay(30);
   mpc8640_out32(PIC_PRR, core);
   printk("asserted again...\n");
   
}

void sreset_peer(void)
{
   unsigned core = 1<<(1-get_cpuid());
   printk("%s: to value[%d]\n", __FUNCTION__,core);

   printk("setting this register[0x%x]\n", PIC_PIR);
   PRINT_REG(PIC_PIR);
   
   mpc8640_out32(PIC_PIR, core);

   printk("asserted...\n");
   PRINT_REG(PIC_PIR);
   delay(30);
   
   mpc8640_out32(PIC_PIR, 0x00);
   printk("Negated...\n");
   PRINT_REG(PIC_PIR);
   delay(30);
   mpc8640_out32(PIC_PIR, core);
   printk("asserted again...\n");
}


void print_block(unsigned address,unsigned limit)
{
    const unsigned char *p = (const unsigned char *)address;
    unsigned j;

    do {
        printk("%08X:", (unsigned)p - address);
        for (j = 0; j < 32; j++) {
            if ((unsigned)p == limit) {
                break;
            }
            printk("%s%02X", ((j % 4) == 0) ? " " : "", *p);
            p++;
        }
        printk(" [%08X]", (unsigned)p);
        printk("\n");

        if ((unsigned)p > (address + 2048)) {
            while ((unsigned)p < (limit - 1023)) {
                p += 1024;
            }
        }

    } while ((unsigned)p != limit);
}

/*
 * Print data buffer in hex and ascii form to the terminal.
 *
 * data reads are buffered so that each memory address is only read once.
 * Useful when displaying the contents of volatile registers.
 *
 * parameters:
 *    addr: Starting address to display at start of line
 *    data: pointer to data buffer
 *    width: data value width.  May be 1, 2, or 4.
 *    count: number of values to display
 *    linelen: Number of values to print per line; specify 0 for default length
 */
#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer (unsigned long addr, char *data, unsigned width, unsigned count, unsigned linelen)
{
	unsigned char linebuf[MAX_LINE_LENGTH_BYTES];
	unsigned *uip = (unsigned*)linebuf;
	unsigned short *usp = (unsigned short*)linebuf;
	unsigned char *ucp = linebuf;
	int i;

	if (linelen*width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	while (count) {
		printk("%08lx:", addr);

		/* check for overflow condition */
		if (count < linelen)
			linelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < linelen; i++) {
			if (width == 4) {
				uip[i] = *(volatile unsigned *)data;
				printk(" %08x", uip[i]);
			} else if (width == 2) {
				usp[i] = *(volatile unsigned short *)data;
				printk(" %04x", usp[i]);
			} else {
				ucp[i] = *(volatile unsigned char *)data;
				printk(" %02x", ucp[i]);
			}
			data += width;
		}

		/* Print data in ASCII characters */
		printk("    ");
		for (i = 0; i < linelen * width; i++)
         {    
			printk("%c",isprint(ucp[i]) && (ucp[i] < 0x80) ? ucp[i] : '.');
         }
		printk("\n");

		/* update references */
		addr += linelen * width;
		count -= linelen;

	}

	return 0;
}

int cmd_get_data_size(char* arg, int default_size)
{
	/* Check for a size specification .b, .w or .l.
	 */
	int len = strlen(arg);
	if (len > 2 && arg[len-2] == '.') {
		switch(arg[len-1]) {
		case 'b':
			return 1;
		case 'w':
			return 2;
		case 'l':
			return 4;
		case 's':
			return -2;
		default:
			return -1;
		}
	}
	return default_size;
}



/* Display values from last command.
 * Memory modify remembered values are different from display memory.
 */
static unsigned	dp_last_addr = 0, dp_last_size = 1;
static unsigned	dp_last_length = 0x40;
static	unsigned long	base_address = 0;

/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN	16
int do_mem_md ( int flag, int argc, char *argv[])
{
	unsigned long	addr, length;
	int	size;
	int rc = 0;

	/* We use the last specified parameters, unless new ones are
	 * entered.
	 */
	addr = dp_last_addr;
	size = dp_last_size;
	length = dp_last_length;


	if ((flag & CMD_FLAG_REPEAT) == 0) {
		/* New command specified.  Check for a size specification.
		 * Defaults to long if no or incorrect specification.
		 */
		if ((size = cmd_get_data_size(argv[0], 1)) < 0)
			return 1;

		/* Address is specified since argc > 1
		*/
		if (argc>=2)addr = simple_strtoul(argv[1], NULL, 16);
		addr += base_address;

		/* If another parameter, it is the length to display.
		 * Length is the number of objects, not number of bytes.
		 */
		if (argc > 2)
			length = simple_strtoul(argv[2], NULL, 16);
	}

	/* Print the lines. */
	print_buffer(addr, (char *)addr, size, length, DISP_LINE_LEN/size);
	addr += size*length;

	dp_last_addr = addr;
	dp_last_length = length;
	dp_last_size = size;
	return (rc);
}


int do_mem_mm (int flag, int argc, char *argv[])
{
	return 0;
}

int duart_enable_interrupt(unsigned unit)
{
   int ret = 0;
   unsigned char lcr_bits = 0;
   const unsigned char DUART_ERDAI = 0x01;
   const unsigned bus_frequency = 400000000;
   int rate = 115200;
   unsigned d = (bus_frequency + rate * 8)/(rate*16);
   
   //printk("uart init...\n");

   lcr_bits = mpc8640_in8(ULCR(unit));

   lcr_bits &= (~0x80);
   
   mpc8640_out8(ULCR(unit),lcr_bits);

   mpc8640_out8(UFCR(unit),0x01);
   mpc8640_out8(UMCR(unit),0x02);

   mpc8640_out8(ULCR(unit),lcr_bits|0x80);
   mpc8640_out8(UAFR(unit),0x00);
   mpc8640_out8(UDLB(unit),d);
   mpc8640_out8(UDMB(unit),d>>8);
   mpc8640_out8(ULCR(unit),lcr_bits);


   mpc8640_out8(UIER(unit), DUART_ERDAI);

   return ret;
}

unsigned mpc8640_in32_func(unsigned a)
{
    return in_be32(immr_base+a);
}

void mpc8640_out32_func(unsigned a, unsigned d)
{
    out_be32(immr_base+a, d);
}

EXPORT_SYMBOL(mpc8640_in32_func);
EXPORT_SYMBOL(mpc8640_out32_func);

