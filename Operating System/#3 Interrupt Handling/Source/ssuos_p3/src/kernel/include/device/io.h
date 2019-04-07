#ifndef	   __IO_H__
#define	   __IO_H__

#include <stddef.h>
#include <stdint.h>

extern void outb(unsigned short port, unsigned char data);
extern unsigned char inb(unsigned short port);
extern void outl(unsigned short port, unsigned int data);
extern unsigned int inl(unsigned short port);
extern void iowait();

extern int read_cr0(void);
extern int write_cr0(unsigned data);
extern int read_cr3(void);
extern int write_cr3(unsigned data);
extern int read_cr4(void);
extern int write_cr4(unsigned data);

extern void dev_shutdown(void);

/* Reads CNT 16-bit (halfword) units from PORT, one after
   another, and stores them into the buffer starting at ADDR. */
static inline void
insw (uint16_t port, void *addr, size_t cnt)
{
  /* See [IA32-v2a] "INS". */
  asm volatile ("rep insw" : "+D" (addr), "+c" (cnt) : "d" (port) : "memory");
}

/* Writes to PORT each 16-bit unit (halfword) of data in the
   CNT-halfword buffer starting at ADDR. */
static inline void
outsw (uint16_t port, const void *addr, size_t cnt)
{
  /* See [IA32-v2b] "OUTS". */
  asm volatile ("rep outsw" : "+S" (addr), "+c" (cnt) : "d" (port));
}

#endif