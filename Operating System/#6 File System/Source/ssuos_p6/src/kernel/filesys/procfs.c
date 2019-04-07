#include <filesys/procfs.h>
#include <filesys/vnode.h>
#include <proc/proc.h>
#include <list.h>
#include <string.h>
#include <ssulib.h>

extern struct list p_list;
extern struct process *cur_process;

struct vnode *init_procfs(struct vnode *mnt_vnode)
{
	mnt_vnode->v_op.ls = proc_process_ls;
	mnt_vnode->v_op.mkdir = NULL;//change it later
	mnt_vnode->v_op.cd = proc_process_cd;

	return mnt_vnode;
}

pid_t atoi(char *string)
{
    int i, num = 0;
    int idx = 1;
    int len = strlen(string);
    for (i = 0; i < len; ++i) {
        num += (string[len-1-i]-'0') * idx;
        idx *= 10;
    }
    return num;
}
int proc_process_ls()
{
	int result = 0;
	struct list_elem *e;

	printk(". .. ");
	for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_all);

		printk("%d ", p->pid);
	}
	printk("\n");

	return result;
}

int proc_process_cd(char *dirname)
{
    struct list_elem *e;
    pid_t pid = atoi(dirname);

	for(e = list_begin (&p_list); e != list_end (&p_list); e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_all);
        if(p->pid == pid)
        {
            struct vnode* new_vnode = vnode_alloc();
            new_vnode->type = PROC_DIR_TYPE;
            new_vnode->v_parent = cur_process->cwd;
            new_vnode->v_op.ls = proc_process_info_ls;
            new_vnode->v_op.cat = proc_process_info_cat;
            new_vnode->v_op.cd = proc_process_info_cd;
            new_vnode->v_op.cat = proc_process_info_cat;
            memcpy(new_vnode->v_name,dirname,PROC_NAME_LEN);
            new_vnode->info = (void *) get_process(pid);

            cur_process->cwd = new_vnode;
            return 0;
        }
    }

	printk("%s is not a directory\n");
    return -1;

}

int proc_process_info_ls()
{
    int result = 0;

    printk(". .. ");
    printk("cwd root time stack");

    printk("\n");

    return result;
}

int proc_process_info_cd(char *dirname)
{
    struct vnode* new_vnode = vnode_alloc();

    new_vnode->type = PROC_DIR_TYPE;
    new_vnode->v_parent = cur_process->cwd;
    /* struct vnode *copy_vnode(struct vnode *ori_vnode, struct vnode *des_vnode) */
    new_vnode->v_op.ls = proc_link_ls;
    /* new_vnode->v_op.cat = proc_process_info_cat; */
    /* new_vnode->v_op.cd = proc_process_info_cd; */
    /* new_vnode->v_op.cat = proc_process_info_cat; */
    memcpy(new_vnode->v_name, dirname, PROC_NAME_LEN);

    if(strcmp(dirname, "cwd")==0)
        new_vnode->info = (void *) ((struct process *)cur_process->cwd->info)->cwd;
    else if(strcmp(dirname, "root")==0)
        new_vnode->info = (void *) cur_process->rootdir;

    cur_process->cwd = new_vnode;

}

int proc_process_info_cat(char *filename)
{
    if(strcmp(filename,"stack") == 0)
        printk("stack : %x\n", ((struct process *)cur_process->cwd->info)->stack);
    else if(strcmp(filename,"time") == 0)
        printk("time_used : %d\n", ((struct process *)cur_process->cwd->info)->time_used);
    else
        printk("no info for %s\n", filename);
}

int proc_link_ls()
{
    struct list_elem *e;
    struct vnode *cwd = cur_process->cwd->info;
    struct vnode *child;

    printk(". .. ");
    for(e = list_begin (&cwd->childlist); e != list_end (&cwd->childlist); e = list_next (e))
    {
        child = list_entry(e, struct vnode, elem);
        printk("%s ", child->v_name);
    }
    printk("\n");

}
