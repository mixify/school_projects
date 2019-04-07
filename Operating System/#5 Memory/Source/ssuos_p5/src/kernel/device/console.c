#include <interrupt.h>
#include <device/console.h>
#include <type.h>
#include <device/kbd.h>
#include <device/io.h>
#include <device/pit.h>
#include <stdarg.h>

#define HSCREEN 80
#define VSCREEN 25
#define SIZE_SCREEN HSCREEN * VSCREEN
#define NSCROLL 100
#define SIZE_SCROLL NSCROLL * HSCREEN
#define VIDIO_MEMORY 0xB8000

#define IO_BASE 0x3F8               /* I/O port base address for the first serial port. */
#define FIRST_SPORT (IO_BASE)
#define LINE_STATUS (IO_BASE + 5)   /* Line Status Register(read-only). */
#define THR_EMPTY 0x20              /* Transmitter Holding Reg Empty. */

char next_line[2]; //"\r\n";

#ifdef SCREEN_SCROLL

#define buf_e (buf_w + SIZE_NSCROLL)
#define SCROLL_END buf_s + SIZE_SCROLL

char buf_s[SIZE_SCROLL]; 
char *buf_w;	
char *buf_p;	

int sum_y;

bool a_s = TRUE;
#endif

void init_console(void)
{
	Glob_x = 0;
	Glob_y = 2;

	next_line[0] = '\r';
	next_line[1] = '\r';

#ifdef SCREEN_SCROLL
	buf_w = buf_s;
	buf_p = buf_s;
	a_s = TRUE;

	sum_y = 0; 
#endif

}

void set_cursor(void)
{
    outb(0x3D4, 0x0F);
    outb(0x3D5, (Glob_y*HSCREEN+Glob_x)&0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (((Glob_y*HSCREEN+Glob_x)>>8)&0xFF));
}

void PrintCharToScreen(int x, int y, const char *pString) 
{
	Glob_x = x;
	Glob_y = y;
	int i = 0;
	while(pString[i] != 0)
	{
		PrintChar(Glob_x++, Glob_y, pString[i++]);
	}
	a_s = TRUE;
}

void PrintChar(int x, int y, const char String) 
{
#ifdef SCREEN_SCROLL
	if (String == '\n') {
		if((y+1) > VSCREEN) {
			scroll();
			y--;
		}
		Glob_x = 0;
		Glob_y = y+1;
		sum_y++; 
		return;
	}
	else if (String == '\b') {
		if(Glob_x == 0) return;
		Glob_x-=2;
		buf_w[y * HSCREEN + x - 1] = 0;
	}
	else {
		if ((y >= VSCREEN) && (x >= 0)) {
			scroll();
			x = 0;
			y--;
		}      	              	

		char* b = &buf_w[y * HSCREEN + x];
		if(b >= SCROLL_END)
			b-= SIZE_SCROLL;
		*b = String;

		if(Glob_x >= HSCREEN)
		{
			Glob_x = 0;
			Glob_y++;
			sum_y++; 
		}    
	}
#else
	CHAR *pScreen = (CHAR *)VIDIO_MEMORY;

	if (String == '\n') {
		if((y+1) > 24) {
			scroll();
			y--;
		}
		pScreen += ((y+1) * 80);
		Glob_x = 0;
		Glob_y = y+1;
	return;
	}
	else {
		if ((y > 24) && (x >= 0)) {
			scroll();
			x = 0; y--;
		}                       

		pScreen += ( y * 80) + x;
		pScreen[0].bAtt = 0x07;
		pScreen[0].bCh = String;

		if(Glob_x > 79)
		{
			Glob_x = 0;
			Glob_y++;
		}    
	}
#endif
}

void clrScreen(void) 
{
	CHAR *pScreen = (CHAR *) VIDIO_MEMORY;
	int i;

	for (i = 0; i < 80*25; i++) {
		(*pScreen).bAtt = 0x07;
		(*pScreen++).bCh = ' ';
	}   
	Glob_x = 0;
	Glob_y = 0;
}

void scroll(void) 
{
#ifdef SCREEN_SCROLL
	buf_w += HSCREEN;
	buf_p += HSCREEN;
	
	while(buf_w > SCROLL_END)
		buf_w -= SIZE_SCROLL;

	int i;
	char *buf_ptr = buf_w + SIZE_SCREEN;
	for(i = 0; i < HSCREEN; i++)
	{
		if(buf_ptr > SCROLL_END)
			buf_ptr -= SIZE_SCROLL;
		*(buf_ptr++) = 0;
	}
		
#else
	CHAR *pScreen = (CHAR *) VIDIO_MEMORY;
	CHAR *pScrBuf = (CHAR *) (VIDIO_MEMORY + 2*80);
	int i;
	for (i = 0; i < 80*24; i++) {
		    (*pScreen).bAtt = (*pScrBuf).bAtt;
		        (*pScreen++).bCh = (*pScrBuf++).bCh;
	}   
	for (i = 0; i < 80; i++) {
		    (*pScreen).bAtt = 0x07;
		        (*pScreen++).bCh = ' ';
	} 
#endif
	Glob_y--;

}

#ifdef SERIAL_STDOUT
void printCharToSerial(const char *pString)
{
	int i;
	enum intr_level old_level = intr_disable();
	for(;*pString != NULL; pString++)
	{
		if(*pString != '\n'){
			while((inb(LINE_STATUS) & THR_EMPTY) == 0)
				continue;
			outb(FIRST_SPORT, *pString);

		}
		else{
			for(i=0; i<2; i++){
				while((inb(LINE_STATUS) & THR_EMPTY) == 0)
					continue;
				outb(FIRST_SPORT, next_line[i]);
			}
		}
	}
	intr_set_level (old_level);
}
#endif


int printk(const char *fmt, ...)
{
	char buf[1024];
	va_list args;
	int len;

	va_start(args, fmt);
	len = vsprintk(buf, fmt, args);
	va_end(args);
	
#ifdef SERIAL_STDOUT
	printCharToSerial(buf);
#endif
	PrintCharToScreen(Glob_x, Glob_y, buf);

	return len;
}

#ifdef SCREEN_SCROLL
void scroll_screen(int offset)
{
	char * tmp_buf_p; 
	char * tmp_buf_w;
	if(a_s == TRUE && offset > 0 && buf_p == buf_w)
		return;

	a_s = FALSE;
	tmp_buf_p = (char*)((int)buf_p + (HSCREEN * offset));
	tmp_buf_w = buf_w + SIZE_SCREEN;
	if(tmp_buf_w > SCROLL_END)
		tmp_buf_w = (char *)((int)tmp_buf_w - SIZE_SCROLL);

	if(sum_y < NSCROLL && offset < 0 && tmp_buf_p <= buf_s && buf_p > buf_s) return;
	if(offset > 0 && tmp_buf_p > buf_w && buf_p <= buf_w) return;
	else if(offset < 0 && tmp_buf_p <= tmp_buf_w && buf_p > tmp_buf_w) return;

	buf_p = tmp_buf_p;

	if(buf_p >= SCROLL_END)
		buf_p = (char*)((int)buf_p - SIZE_SCROLL);
	else if(buf_p < buf_s)
		buf_p = (char*)((int)buf_p + SIZE_SCROLL);

	refreshScreen();
}

void set_fallow(void)
{
	a_s = TRUE;
}

void refreshScreen(void)
{
	CHAR *p_s= (CHAR *) VIDIO_MEMORY;
	int i;

	if(a_s)
		buf_p = buf_w;

	char* b = buf_p;

	for(i = 0; i < SIZE_SCREEN; i++, b++, p_s++)
	{
		if(b >= SCROLL_END)
			b -= SIZE_SCROLL;
		p_s->bAtt = 0x07;
		p_s->bCh = *b;
	}
}
#endif

