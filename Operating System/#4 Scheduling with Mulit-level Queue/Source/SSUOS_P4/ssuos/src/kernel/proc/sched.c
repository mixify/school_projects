#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>
#include <device/console.h>

extern struct list level_que[QUE_LV_MAX];
extern struct list plist;
extern struct list slist;
extern struct process procs[PROC_NUM_MAX];
extern struct process *idle_process;

extern struct process *latest;

struct process* get_next_proc(struct list *rlist_target);
void proc_que_levelup(struct process *cur);
void proc_que_leveldown(struct process *cur);
struct process* sched_find_que(void);

int scheduling;

int total;
/*
   linux multilevelfeedback queue scheduler
   level 1 que policy is FCFS(First Come, First Served)
   level 2 que policy is RR(Round Robin).
   */

//sched_find_que find the next process from the highest queue that has proccesses.
struct process* sched_find_que(void) {
    int i,j;
    struct process * result = NULL;

    while(1)
    {

        proc_wake();

        if(list_empty(&level_que[1]))
        {
            if(list_empty(&level_que[2]))
            {
                continue;
            }
            else
            {
                result = get_next_proc(&level_que[2]);
            }
        }
        else
        {
            result = get_next_proc(&level_que[1]);
        }
        /*TODO :check the queue whether it is empty or not
          and find each queue for the next process.
          */
        return result;
    }
}

struct process* get_next_proc(struct list *rlist_target) {
    struct list_elem *e;

    /* if(cur_process->state == PROC_RUN) */
    /* { */
    /*     list_remove(&cur_process->elem_stat); */
    /*     list_push_back(&level_que[1], &cur_process->elem_stat); */
    /* } */

    for(e = list_begin (rlist_target); e != list_end (rlist_target);
            e = list_next (e))
    {
        struct process* p = list_entry(e, struct process, elem_stat);

        if(p->state == PROC_RUN)
        {
            /* printk("pid = %d\n", p->pid); */
            return p;
        }
    }
    return cur_process;
}

void schedule(void)
{
    struct process *cur;
    struct process *next;
    struct process *tmp;
    struct list_elem *ele;
    int i = 0, printed = 0;

    /* intr_enable(); */
    scheduling = 1;
    cur = cur_process;
    /*TODO : if current process is idle_process(pid 0), schedule() choose the next process (not pid 0).
      when context switching, you can use switch_process().
      if current process is not idle_process, schedule() choose the idle process(pid 0).
      complete the schedule() code below.
      */
    if ((cur -> pid) != 0) { // pid is not 0
        next = idle_process;
        /* printk("pid is not 0 cur = %d, next = %d\n", cur->pid, next->pid); */
        //sched_find_que();
        cur_process = idle_process;
        scheduling = 1;
        /* intr_disable(); */
        switch_process(cur,next);
        /* intr_enable(); */
        return;
    }

    int before = 0;
    for (ele = list_begin(&plist); ele != list_end(&plist); ele = list_next(ele)) {
        tmp = list_entry (ele, struct process, elem_all);
        if ((tmp -> state == PROC_ZOMBIE) ||
                //(tmp -> state == PROC_BLOCK) ||
                //	(tmp -> state == PROC_STOP) ||
                (tmp -> pid == 0)) 	continue;

        before += tmp->time_used;
    }

    /* printk("%d",latest->pid); */
    switch (latest -> que_level){
        case 0:
            break;
            /* case 0://is idle process */
            /*     next = sched_find_que(); */
            /*     latest = next; */
            /*     #<{(| printk("latest idle : %d\n",cur->pid); |)}># */
            /*     switch_process(latest, next); */
            /*     break; */
            /*  */
        case 1://FIFO
            /* time_used 200 kernel1 gets finished so, should wait for this  */
            /* if(latest->state == PROC_RUN) */
            /*     break; */
            next = sched_find_que();
            cur = cur_process;
            cur_process = next;
            scheduling = 0;
            /* printk("Q-LEVEL 1 : latest = %d switching cur = %d, next = %d\n",latest->pid, cur->pid, next->pid); */
            //       cur_process->time_used+=40;
            /* intr_disable(); */
            switch_process(cur, next);
            proc_que_leveldown(next);
            /* intr_enable(); */


            /* printk("after latest = %d, next = %d\n", latest->pid, next->pid); */
            break;

        case 2://ROUND AND ROBIN
            next = sched_find_que();
            cur = cur_process;
            cur_process = next;
            scheduling = 0;
            /* printk("Q-LEVEL 1 : latest = %d switching cur = %d, next = %d\n",latest->pid, cur->pid, next->pid); */
            //       cur_process->time_used+=40;
            list_remove(&next->elem_stat);
            list_push_back(&level_que[latest->que_level], &next->elem_stat);
            switch_process(cur, next);


            break;

    }
    /* intr_disable(); */

    /* printk("Queue-Level-1 Status\n"); */
    /* for (ele = list_begin(&level_que[1]); ele != list_end(&level_que[1]); ele = list_next(ele)) { */
    /*     tmp = list_entry (ele, struct process, elem_stat); */
    /*     printk("%d ", tmp->pid); */
    /* } */
    /*  */
    /* printk("\nQueue-Level-2 Status\n"); */
    /* for (ele = list_begin(&level_que[2]); ele != list_end(&level_que[2]); ele = list_next(ele)) { */
    /*     tmp = list_entry (ele, struct process, elem_stat); */
    /*     printk("%d ", tmp->pid); */
    /* } */
    /* printk("\n"); */

    /* printk("\nQueue-sleep Status\n"); */
    /* for (ele = list_begin(&slist); ele != list_end(&slist); ele = list_next(ele)) { */
    /*     tmp = list_entry (ele, struct process, elem_stat); */
    /*     printk("%d ", tmp->pid); */
    /* } */
    /* printk("\n"); */
    /*  */


    proc_wake(); //wake up the processes
    //print the info of all 10 proc.
    /* printk("ticks = %d\n", get_ticks()); */
    int after = 0;
    intr_disable();
    for (ele = list_begin(&plist); ele != list_end(&plist); ele = list_next(ele)) {
        tmp = list_entry (ele, struct process, elem_all);
        if ((tmp -> state == PROC_ZOMBIE) ||
                //(tmp -> state == PROC_BLOCK) ||
                //	(tmp -> state == PROC_STOP) ||
                (tmp -> pid == 0)) 	continue;
        if (!printed) {
            printk("#=%2d t=%3d u=%3d ", tmp -> pid, tmp -> time_slice, tmp -> time_used);
            printk("q=%3d\n", tmp->que_level, tmp->state);
            printed = 1;
        }
        else {
            printk("#=%2d t=%3d u=%3d ", tmp -> pid, tmp -> time_slice, tmp->time_used);
            printk("q=%3d\n", tmp->que_level, tmp->state);
        }
        after += tmp->time_used;
    }
    /* for (ele = list_begin(&plist); ele != list_end(&plist); ele = list_next(ele)) { */

    /* int diff = after-before; */
    /* printk("diff = %d\n\n", after-before); */

    /* total+=diff; */

    /* latest->time_slice = 0; */
    if (printed)
        printk("\n");

    if ((next = sched_find_que()) != NULL) {
        printk("Selected process : %d\n", next -> pid);
        latest = next;
        next->time_slice = 0;
        /* int level = latest->que_level; */
        /* if (level == 1) */
        /*     total += 40; */
        /* if (level == 2) */
        /*     total += 80; */

        /* printk("total = %d\n", total); */
        intr_enable();
        return;
    }
    intr_enable();
    return;
}

void proc_que_levelup(struct process *cur)
{
    /*TODO : change the queue lv2 to queue lv1.*/
}

void proc_que_leveldown(struct process *cur)
{

    /* printk("%d process queue level down\n", cur->pid); */
    if(cur->state != PROC_RUN)
        return;
    printk("proc%d change the queue(1->2)\n", cur->pid);
    cur->que_level = 2;
    list_remove(&cur->elem_stat);
    /* list_pop_front(&level_que[1]); */
    list_push_back(&level_que[2], &cur->elem_stat);
    /*TODO : change the queue lv1 to queue lv2.*/
}
