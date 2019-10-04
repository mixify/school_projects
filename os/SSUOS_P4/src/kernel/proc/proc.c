#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <ssulib.h>
#include <interrupt.h>
#include <proc/sched.h>
#include <syscall.h>
#include <mem/palloc.h>

#define STACK_SIZE 512

struct list plist;				// All Process List
struct list slist;				// Sleep Process List
/* struct list rlist;				// Running Process List */
struct list runq[RQ_NQS];		// Priority array

struct process procs[PROC_NUM_MAX];
struct process *cur_process;
struct process *idle_process;
int pid_num_max;
uint32_t process_stack_ofs;
static int lock_pid_simple;
static int lately_pid;

bool more_prio(const struct list_elem *a, const struct list_elem *b, void *aux);
bool less_time_sleep(const struct list_elem *a, const struct list_elem *b, void *aux);
pid_t getValidPid(int *idx);
void proc_start(void);
void proc_end(void);
void proc_getpos(int, int *);

void kernel1_proc(void *aux);
void kernel2_proc(void *aux);
void kernel3_proc(void *aux);

/*
Hint : use 'printk' function to trace function call
*/

void init_proc()
{
	int i;
	process_stack_ofs = offsetof (struct process, stack);

	lock_pid_simple = 0;
	lately_pid = -1;

	list_init(&plist);
	/* list_init(&rlist); */
    for (i = 0; i < RQ_NQS; ++i) {
        list_init(&runq[i]);
    }
	list_init(&slist);

	for (i = 0; i < PROC_NUM_MAX; i++)
	{
		procs[i].pid = i;
		procs[i].state = PROC_UNUSED;
		procs[i].parent = NULL;
	}

	pid_t pid = getValidPid(&i);
	cur_process = (int *)&procs[0];
	idle_process = (int *)&procs[0];

	cur_process->pid = pid;
	cur_process->parent = NULL;
	cur_process->state = PROC_RUN;

	cur_process -> priority = 99;

	cur_process->stack = 0;
	cur_process->pd = (void*)read_cr3();
	cur_process -> elem_all.prev = NULL;
	cur_process -> elem_all.next = NULL;
	cur_process -> elem_stat.prev = NULL;
	cur_process -> elem_stat.next = NULL;

	/* You should modify this function... */
	list_push_back(&plist, &cur_process->elem_all);
	/* list_push_back(&rlist, &cur_process->elem_stat); */
	list_push_back(&runq[99/RQ_PPQ], &cur_process->elem_stat);
}

pid_t getValidPid(int *idx) {
	pid_t pid = -1;
	int i;

	while(lock_pid_simple);

	lock_pid_simple++;

	for(i = 0; i < PROC_NUM_MAX; i++)
	{
		int tmp = i + lately_pid + 1;
		if(procs[tmp % PROC_NUM_MAX].state == PROC_UNUSED) {
			pid = lately_pid + 1;
			*idx = tmp % PROC_NUM_MAX;
			break;
		}
	}

	if(pid != -1)
		lately_pid = pid;

	lock_pid_simple = 0;

	return pid;
}

pid_t proc_create(proc_func func, struct proc_option *opt, void* aux)
{
	struct process *p;
	int idx;

	enum intr_level old_level = intr_disable();

	pid_t pid = getValidPid(&idx);
	p = &procs[pid];
	p->pid = pid;
	p->state = PROC_RUN;

	if(opt != NULL)
		p -> priority = opt -> priority;
	else
		p -> priority = (unsigned char)99;

	p->time_used = 0;
	p->time_slice = 0;
	p->parent = cur_process;
	p->simple_lock = 0;
	p->child_pid = -1;
	p->pd = pd_create(p->pid);

	//init stack
	int *top = (int*)palloc_get_page();
	int stack = (int)top;
	top = (int*)stack + STACK_SIZE - 1;

	*(--top) = (int)aux;		//argument for func
	*(--top) = (int)proc_end;	//return address from func
	*(--top) = (int)func;		//return address from proc_start
	*(--top) = (int)proc_start; //return address from switch_process

	//process call stack :
	//switch_process > proc_start > func(aux) > proc_end

	*(--top) = (int)((int*)stack + STACK_SIZE - 1); //ebp
	*(--top) = 1; //eax
	*(--top) = 2; //ebx
	*(--top) = 3; //ecx
	*(--top) = 4; //edx
	*(--top) = 5; //esi
	*(--top) = 6; //edi

	p -> stack = top;
	p -> elem_all.prev = NULL;
	p -> elem_all.next = NULL;
	p -> elem_stat.prev = NULL;
	p -> elem_stat.next = NULL;

	/* You should modify this function... */
	list_push_back(&plist, &p->elem_all);
	/* list_push_back(&rlist, &p->elem_stat); */
	/* list_insert_ordered(&runq[p->priority/RQ_PPQ], &p->elem_stat, */
	/* 		more_prio, NULL); */
	list_push_back(&runq[p->priority/RQ_PPQ], &p->elem_stat);

	intr_set_level (old_level);
	return p->pid;
}

void* getEIP()
{
    return __builtin_return_address(0);
}

void recalculate_priority(void)
{
	int idx;
	struct process *p = cur_process;

    if(p->pid != 0 && p->time_slice % 30 == 0)
        p->priority += p->time_slice/10;

	list_remove(&cur_process->elem_stat);
	/* list_insert_ordered(&slist, &cur_process->elem_stat, */
	/* 		less_time_sleep, NULL); */
	list_push_back(&runq[p->priority/RQ_PPQ], &p->elem_stat);
	/* Your code goes here... */
}

void  proc_start(void)
{
	intr_enable ();
	return;
}

void proc_free(void)
{
	uint32_t pt = *(uint32_t*)cur_process->pd;
	cur_process->parent->child_pid = cur_process->pid;
	cur_process->parent->simple_lock = 0;

	cur_process->state = PROC_ZOMBIE;

	palloc_free_page(cur_process->stack);
	palloc_free_page((void*)pt);
	palloc_free_page(cur_process->pd);

	list_remove(&cur_process->elem_stat);
	list_remove(&cur_process->elem_all);
}

void proc_end(void)
{
	proc_free();
	schedule();
	return;
}

void proc_wake(void)
{
	struct process* p;
	int idx;
	int old_level;
	unsigned long long t = get_ticks();

    while(!list_empty(&slist))
	{
		p = list_entry(list_front(&slist), struct process, elem_sleep);
		if(p->time_sleep > t)
			break;

		/* You should modify this function... */
		list_remove(&p->elem_sleep);
        list_push_back(&runq[p->priority/RQ_PPQ], &p->elem_stat);
		/* list_push_back(&rlist, &p->elem_stat); */
		p->state = PROC_RUN;
	}
}

void proc_sleep(unsigned ticks)
{
	unsigned long cur_ticks = get_ticks();

	/* You should modify this function... */
    intr_disable();
    printk("Proc %d I/O at %d\n",cur_process->pid ,cur_process->time_used);

	cur_process->time_sleep = ticks + cur_ticks;
	cur_process->state = PROC_STOP;
	cur_process->time_slice = 0;

	list_remove(&cur_process->elem_stat);
	/* list_remove(&cur_process->elem_stat); */
	/* list_push_back(&runq[p->priority/RQ_PPQ], &p->elem_stat); */
	list_insert_ordered(&slist, &cur_process->elem_sleep,
			less_time_sleep, NULL);
    intr_enable();
	schedule();
}

void proc_block(void)
{
	cur_process->state = PROC_BLOCK;
	schedule();
}

void proc_unblock(struct process* proc)
{
	enum intr_level old_level;
	int idx;

	/* You shoud modify this function... */
	/* list_push_back(&rlist, &proc->elem_stat); */
	list_push_back(&runq[proc->priority/RQ_PPQ], &proc->elem_stat);
	proc->state = PROC_RUN;
}

bool less_time_sleep(const struct list_elem *a, const struct list_elem *b,void *aux)
{
	struct process *p1 = list_entry(a, struct process, elem_stat);
	struct process *p2 = list_entry(b, struct process, elem_stat);

	return p1->time_sleep < p2->time_sleep;
}

bool more_prio(const struct list_elem *a, const struct list_elem *b,void *aux)
{
	struct process *p1 = list_entry(a, struct process, elem_stat);
	struct process *p2 = list_entry(b, struct process, elem_stat);

	return p1->priority > p2->priority;
}

void kernel1_proc(void* aux)
{
	int passed = 0;
	while(1)
	{
        if(cur_process->time_used >= 140 && (!passed))
        {
            proc_sleep(60);
            passed = 1;
        }

        if(cur_process->time_used >= 200)
        {
            proc_end();
        }
		/* Your code goes here... */
	}
}

void kernel2_proc(void* aux)
{
	int passed = 0;
	while(1)
	{
        if(cur_process->time_used >= 100 && (!passed))
        {
            proc_sleep(60);
            passed = 1;
        }

        if(cur_process->time_used >= 120)
            proc_end();
	}
}

void kernel3_proc(void* aux)
{
	int passed1 = 0, passed2 = 0;

	while(1)
	{
        if(cur_process->time_used >= 50 && (!passed1))
        {
            proc_sleep(60);
            passed1 = 1;
        }

        /* printk("%d\n",cur_process->time_used); */
        if(cur_process->time_used >= 100 && (!passed2))
        {
            proc_sleep(60);
            passed2 = 1;
        }


        if(cur_process->time_used >= 150)
            proc_end();

		/* Your code goes here... */
	}
}

/*
Let's say RQ_NQS is 5 and RQ_PPQ is 4,
then the location of process with priority 7 is :
7/4=1
0 [ ]
1 [*]
2 [ ]
3 [ ]
4 [ ]
*/

// idle process, pid = 0
void idle(void* aux)
{
	struct proc_option opt1 = { .priority = 50 };
	struct proc_option opt2 = { .priority = 50 };
	struct proc_option opt3 = { .priority = 30 };

	proc_create(kernel1_proc, &opt1, NULL);
	proc_create(kernel2_proc, &opt2, NULL);
	proc_create(kernel3_proc, &opt3, NULL);


	while(1) {
		schedule();
	}
}

void proc_getpos(int priority, int *idx)
{
	/* Your code goes here... */
}

void proc_print_data()
{
	int a, b, c, d, bp, si, di, sp;

	//eax ebx ecx edx
	__asm__ __volatile("mov %%eax ,%0": "=m"(a));

	__asm__ __volatile("mov %ebx ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(b));

	__asm__ __volatile("mov %ecx ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(c));

	__asm__ __volatile("mov %edx ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(d));

	//ebp esi edi esp
	__asm__ __volatile("mov %ebp ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(bp));

	__asm__ __volatile("mov %esi ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(si));

	__asm__ __volatile("mov %edi ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(di));

	__asm__ __volatile("mov %esp ,%eax");
	__asm__ __volatile("mov %%eax ,%0": "=m"(sp));

	printk(	"\neax %o ebx %o ecx %o edx %o"\
			"\nebp %o esi %o edi %o esp %o\n"\
			, a, b, c, d, bp, si, di, sp);
}

void hexDump (void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    if (len == 0) {
        printk("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printk("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                printk ("  %s\n", buff);

            printk ("  %04x ", i);
        }

        printk (" %02x", pc[i]);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        printk ("   ");
        i++;
    }

    printk ("  %s\n", buff);
}


