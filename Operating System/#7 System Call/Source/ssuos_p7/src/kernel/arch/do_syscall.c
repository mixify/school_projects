#include <proc/sched.h>
#include <proc/proc.h>
#include <device/device.h>
#include <interrupt.h>
#include <device/kbd.h>
#include <filesys/file.h>

pid_t do_fork(proc_func func, void* aux1, void* aux2)
{
    pid_t pid;
    struct proc_option opt;

    opt.priority = cur_process-> priority;
    pid = proc_create(func, &opt, aux1, aux2);

    return pid;
}

void do_exit(int status)
{
    cur_process->exit_status = status; 	//종료 상태 저장
    proc_free();						//프로세스 자원 해제
    do_sched_on_return();				//인터럽트 종료시 스케줄링
}

pid_t do_wait(int *status)
{
    while(cur_process->child_pid != -1)
        schedule();
    //SSUMUST : schedule 제거.

    int pid = cur_process->child_pid;
    cur_process->child_pid = -1;

    extern struct process procs[];
    procs[pid].state = PROC_UNUSED;

    if(!status)
        *status = procs[pid].exit_status;

    return pid;
}

void do_shutdown(void)
{
    dev_shutdown();
    return;
}

int do_ssuread(void)
{
    return kbd_read_char();
}

int do_open(const char *pathname, int flags)
{
    struct inode *inode;
    struct ssufile **file_cursor = cur_process->file;
    int fd;

    for(fd = 0; fd < NR_FILEDES; fd++)
        if(file_cursor[fd] == NULL) break;

    if (fd == NR_FILEDES)
        return -1;

    if ( (inode = inode_open(pathname)) == NULL)
        return -1;

    if (inode->sn_type == SSU_TYPE_DIR)
        return -1;

    fd = file_open(inode,flags,0);

    return fd;
}

int do_read(int fd, char *buf, int len)
{
    return generic_read(fd, (void *)buf, len);
}
int do_write(int fd, const char *buf, int len)
{
    return generic_write(fd, (void *)buf, len);
}
int do_lseek(int fd, int offset, int whence, int option)
{
    struct ssufile *cursor;
    uint16_t *pos =  &(cur_process->file[fd]->pos);
    uint16_t old_pos = *pos;

    if( (cursor = cur_process->file[fd]) == NULL)
        return -1;

    if (~cursor->flags & O_RDONLY)
        return -1;

    uint32_t file_size = cursor->inode->sn_size;
    switch(whence)
    {
        case SEEK_SET:
            *pos = 0;
            break;

        case SEEK_CUR:
            break;

        case SEEK_END:
            *pos = file_size;
            break;
    }

    if(option == APPEND_OPTION)
    {
        uint16_t tmp_pos = *pos;
        uint16_t resoff = file_size - *pos;
        char tmp_buf[resoff];
        char buf[offset];

        memset(buf,'0',offset);
        generic_read(fd,tmp_buf,resoff);

        *pos = tmp_pos;
        generic_write(fd,buf,offset);
        tmp_pos = *pos;
        generic_write(fd,tmp_buf,resoff);

        *pos = tmp_pos;
        return *pos;
    }

    if(*pos + offset < 0 || *pos+offset > file_size)
    {

        if(option == EXPAND_OPTION)
        {
            uint16_t resoff = *pos + offset - file_size;
            /* printk("pos = %d, offset = %d, file_size = %d\n", pos, offset, file_size); */
            /* printk("resoff = %d\n"); */
            *pos = file_size;
            char buf[resoff];
            memset(buf,'0',resoff);
            generic_write(fd, (void *)buf, resoff);
            *pos = file_size + resoff;
            return *pos;
        }
        else if(option == REVERSE_OPTION)
        {
            uint16_t resoff = -offset - *pos;

            char buf[resoff];
            memset(buf,'0',resoff);

            char tmp_buf[file_size];
            *pos = 0;
            generic_read(fd,tmp_buf,file_size);
            *pos = 0;

            generic_write(fd,buf,resoff);
            generic_write(fd,tmp_buf,file_size);
            *pos = 0;

            return *pos;
        }
        else if(option == CIRCULAR_OPTION)
        {
            if(offset > 0)
                *pos = (offset + *pos) % file_size;
            else
            {
                *pos = file_size - (-(*pos + offset) % file_size);
            }
            return *pos;
        }
        else
        {
            /* printk("Error => EOF! pos at %d\n", *pos); */
            *pos = old_pos;
            return -1;//EOF
        }
    }



    *pos += offset;

    return *pos;

}
