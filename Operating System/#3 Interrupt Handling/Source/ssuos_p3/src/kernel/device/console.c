#include <interrupt.h>
#include <type.h>
#include <proc/proc.h>
#include <device/kbd.h>
#include <device/console.h>
#include <device/io.h>
#include <device/pit.h>
#include <stdarg.h>

char next_line[2]; //"\r\n";

#ifdef SCREEN_SCROLL
#endif

//위 전역변수를 사용하는 코드를 아래 변수를 사용하는 코드로 변경
struct Console console[MAX_CONSOLE_NUM];
/* extern struct process *cur_process; */
extern struct process *cur_process;//it self changes alot fucking
struct Console *cur_console;
struct Console *cur;

void init_console(void)
{
    int i;
    for (i = 0; i < MAX_CONSOLE_NUM; ++i) {

        console[i].used = FALSE;
        console[i].Glob_x = 0;
        console[i].Glob_y = 2;

#ifdef SCREEN_SCROLL
        int j;
        for (j = 0; j < SIZE_SCROLL; ++j) {
            console[i].buf_s[j] = 0;
        }
        console[i].buf_w = console[i].buf_s;
        console[i].buf_p = console[i].buf_s;
        console[i].a_s = TRUE;

        console[i].sum_y = 0;
#endif

    }
    /* cur_console = cur_process->console = get_console(); */
    cur_console = get_console();
    next_line[0] = '\r';
    next_line[1] = '\r';
}

void set_cursor(void)
{
    outb(0x3D4, 0x0F);
    outb(0x3D5, (cur_console->Glob_y*HSCREEN+cur_console->Glob_x)&0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (((cur_console->Glob_y*HSCREEN+cur_console->Glob_x)>>8)&0xFF));
}

void PrintCharToScreen(int x, int y, const char *pString)
{
    if(cur_process == NULL)
        cur = cur_console;
    else
        cur = cur_process->console;
    cur->Glob_x = x;
    cur->Glob_y = y;
    int i = 0;
    while(pString[i] != 0)
    {
        PrintChar(cur->Glob_x++, cur->Glob_y, pString[i++]);
    }
    cur->a_s = TRUE;
}

void PrintChar(int x, int y, const char String)
{
    if(cur_process == NULL)
        cur = cur_console;
    else
        cur = cur_process->console;
#ifdef SCREEN_SCROLL
    if (String == '\n') {
        if((y+1) > VSCREEN) {
            scroll();
            y--;
        }
        cur->Glob_x = 0;
        cur->Glob_y = y+1;
        cur->sum_y++;
        return;
    }
    else if (String == '\b') {
        if(cur->Glob_x == 0) return;
        cur->Glob_x-=2;
        cur->buf_w[y * HSCREEN + x - 1] = 0;
    }
    else {
        if ((y >= VSCREEN) && (x >= 0)) {
            scroll();
            x = 0;
            y--;
        }

        char* b = &cur->buf_w[y * HSCREEN + x];
        if(b >= SCROLL_END)
            b-= SIZE_SCROLL;
        *b = String;

        if(cur->Glob_x >= HSCREEN)
        {
            cur->Glob_x = 0;
            (cur->Glob_y)++;
            (cur->sum_y)++;
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
        cur->Glob_x = 0;
        cur->Glob_y = y+1;
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

        if(cur->Glob_x > 79)
        {
            cur->Glob_x = 0;
            cur->Glob_y++;
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
    cur_console->Glob_x = 0;
    cur_console->Glob_y = 0;
}

//Ctrl+l 화면 클리어 구현
void clearScreen(void)
{
    char * tmp_buf_p;

    tmp_buf_p = (char*)((int)cur_console->buf_p + HSCREEN * cur_console->Glob_y);
    /* if(tmp_buf_w > SCROLL_END) */
    /*     tmp_buf_w = (char *)((int)tmp_buf_w - SIZE_SCROLL); */

    cur_console->buf_p = tmp_buf_p;
    cur_console->buf_w = cur_console->buf_p;

    cur_console->Glob_y = 0;
    /* cur_console->sum_y = 0; */
    set_cursor();
    refreshScreen();
}

void scroll(void)
{
    if(cur_process == NULL)
        cur = cur_console;
    else
        cur = cur_process->console;
#ifdef SCREEN_SCROLL
    cur->buf_w += HSCREEN;
    cur->buf_p += HSCREEN;

    while(cur->buf_w > SCROLL_END)
        cur->buf_w -= SIZE_SCROLL;


    //clear line
    int i;
    char *buf_ptr = cur->buf_w + SIZE_SCREEN;
    for(i = 0; i < HSCREEN; i++)
    {
        if(buf_ptr > SCROLL_END)
            buf_ptr -= SIZE_SCROLL;
        *(buf_ptr++) = 0;
    }

    //
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
    cur->Glob_y--;

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

    if(cur_process == NULL)
        cur = cur_console;
    else
        cur = cur_process->console;

    va_start(args, fmt);
    len = vsprintk(buf, fmt, args);
    va_end(args);

#ifdef SERIAL_STDOUT
    printCharToSerial(buf);
#endif
    PrintCharToScreen(cur->Glob_x, cur->Glob_y, buf);

    return len;
}

#ifdef SCREEN_SCROLL
void scroll_screen(int offset)
{
    char * tmp_buf_p;
    char * tmp_buf_w;
    if(cur_console->a_s == TRUE && offset > 0 && cur_console->buf_p == cur_console->buf_w)
        return;

    cur_console->a_s = FALSE;

    tmp_buf_p = (char*)((int)cur_console->buf_p + (HSCREEN * offset));
    tmp_buf_w = cur_console->buf_w + SIZE_SCREEN;
    if(tmp_buf_w > SCROLL_END)
        tmp_buf_w = (char *)((int)tmp_buf_w - SIZE_SCROLL);

    if(cur_console->sum_y < NSCROLL && offset < 0 && cur_console->buf_p <= cur_console->buf_s && cur_console->buf_p > cur_console->buf_s) return;
    if(offset > 0 && tmp_buf_p > cur_console->buf_w && cur_console->buf_p <= cur_console->buf_w) return;
    else if(offset < 0 && tmp_buf_p <= tmp_buf_w && cur_console->buf_p > tmp_buf_w) return;

    cur_console->buf_p = tmp_buf_p;

    if(cur_console->buf_p >= SCROLL_END)
        cur_console->buf_p = (char*)((int)cur_console->buf_p - SIZE_SCROLL);
    else if(cur_console->buf_p < cur_console->buf_s)
        cur_console->buf_p = (char*)((int)cur_console->buf_p + SIZE_SCROLL);

    refreshScreen();
}

void set_fallow(void)
{
    cur_console->a_s = TRUE;
}

void refreshScreen(void)
{
    CHAR *p_s= (CHAR *) VIDIO_MEMORY;
    int i;

    if(cur_console->a_s)
        cur_console->buf_p = cur_console->buf_w;

    char* b = cur_console->buf_p;

    for(i = 0; i < SIZE_SCREEN; i++, b++, p_s++)
    {
        if(b >= SCROLL_END)
            b -= SIZE_SCROLL;
        p_s->bAtt = 0x07;
        p_s->bCh = *b;
    }
}

//콘솔 할당 함수
struct Console *get_console(void){
    int i;

    for(i = 0; i < MAX_CONSOLE_NUM; i++){
        if(console[i].used == FALSE){
            console[i].used = TRUE;
/*  */
/*             Glob_x = console[i].Glob_x; */
/*             Glob_y = console[i].Glob_y; */
/*  */
/* #ifdef SCREEN_SCROLL */
/*             buf_w = console[i].buf_w; */
/*             buf_p = console[i].buf_p; */
/*             a_s = console[i].a_s; */
/*  */
/*             sum_y = console[i].sum_y; */
/*  */
/*  */
/* #endif */
            /* cur_console = &console[i]; */
            return &console[i];
        }
    }

    return NULL;
}
#endif

