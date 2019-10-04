#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>

extern struct list plist;
/* extern struct list rlist; */
extern struct list runq[RQ_NQS];

extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;
struct process *latest;

bool more_prio(const struct list_elem *a, const struct list_elem *b,void *aux);
int scheduling; 					// interrupt.c

void print_proc()
{
    struct list_elem *elem;
    bool passed = false;
    for(elem = list_begin(&plist); elem != list_end(&plist); elem = list_next(elem))
    {
        struct process *p = list_entry(elem, struct process, elem_all);

        if(p->state == PROC_RUN)
        {
            if(p->pid != 0)
            {
                passed = true;
                printk("# = %2d ",p->pid);
                printk("p= %2d ",p->priority);
                printk("c= %3d ",p->time_slice);
                printk("u= %3d ",p->time_used);
                printk(", ");
            }
        }
    }
    if(passed)
        printk("\n");
}

struct process* get_next_proc(void)
{
    bool found = false;
    struct process *next = NULL;
    struct list_elem *elem;

    if(cur_process->pid != 0)
        return idle_process;
    else
        print_proc();

    for(int i = 0 ; i < RQ_NQS ; i++)
        for(elem = list_begin(&runq[i]); elem != list_end(&runq[i]); elem = list_next(elem))
        {
            struct process *p = list_entry(elem, struct process, elem_stat);

            if(p->state == PROC_RUN)
            {
                if(p->pid!=0)
                    printk("Selected # = %d\n",p->pid);
                return p;
            }

        }

    return next;
}

void schedule(void)
{
    struct process *cur;
    struct process *next;

    intr_disable();
    scheduling = 1;

    /* You shoud modify this function.... */

    proc_wake();

    next = get_next_proc();
    cur = cur_process;
    cur_process = next;
    cur_process->time_slice = 0;

    scheduling = 0;
    /* printk("next => %d(%d)\n",next->pid,next->time_used); */
    intr_enable();
    switch_process(cur, next);

}
