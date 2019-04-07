#include "main.h"
int main(int argc, char *argv[])
{
    pid_t pid;
    char filename[PATH_MAX] = {0,};

    get_input(argc, argv, filename);//get filename, period, options

    pid = get_process_id(argv[0]);//get existing process id
    if(C_OPTION || R_OPTION)
    {
        if(pid > 0)
        {
            printf("Send signal to %s process <%d>\n", argv[0]+2, pid );
            kill(pid,SIGUSR1);//send signal and kill working process
        }
        ssu_backup(filename,NULL);
    }

    if(period == 0)//when input dosen't include period
    {
        fprintf(stderr, "you should put period usage : %s <FILENAME> [PERIOD] [OPTION] \n", argv[0] );
        exit(1);
    }

    if(pid > 0)
        kill(pid,SIGUSR1);//send signal and kill working process
    if(ssu_daemon_init(filename) < 0)//make daemon process and backup
    {
        fprintf(stderr, "ssu_daemon_init failed\n");
        exit(1);
    }
    exit(0);
}

void get_input(int argc, char *argv[], char *filename)
{
    int option;
    int i;

    while((option = getopt(argc, argv, "mcrn:d")) != -1)//get options
    {
        switch(option)
        {
            case 'm':
                M_OPTION = 1;
                break;
            case 'c':
                C_OPTION = 1;
                break;
            case 'r':
                R_OPTION = 1;
                break;
            case 'n':
                N_OPTION = 1;
                if((backup_file_count = atoi(optarg)) <= 0)
                {
                    fprintf(stderr, "you should use natural number with -n option\n");
                    exit(1);
                }
                break;
            case 'd':
                D_OPTION = 1;
                break;
            case '?':
                if(optopt == 'n')
                    exit(1);
        }
    }

    while(argv[optind] != NULL)
    {
        if(filename[0] == 0)//get filename
        {
            struct stat st;
            strcpy(filename, argv[optind]);
            if(access(filename,F_OK) < 0)
            {
                fprintf(stderr, "file %s doesn't exists\n", filename);
                exit(1);
            }
            if(stat(filename, &st) < 0)
            {
                fprintf(stderr, "stat error for %s\n", filename);
                exit(1);
            }
            if(D_OPTION + M_OPTION + N_OPTION + C_OPTION + R_OPTION == 0)
            {
                if(!S_ISREG(st.st_mode))//without option it should be regular file
                {
                    fprintf(stderr, "%s should be Regular File\n", filename);
                    exit(1);
                }
            }
        }
        else if(period == 0)//get period
        {
            period = atoi(argv[optind]);
            if(period < 3 || period > 10)
            {
                fprintf(stderr, "period should be (3 <= period <= 10)\n");
                exit(1);
            }
        }
        optind++;
    }
}
int ssu_daemon_init(char *filename)
{
    pid_t pid;
    int fd, maxfd;
    time_t intertime;
    struct stat st;

    printf("Daemon process initialization.\n");
    if((pid = fork()) < 0)
    {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if(pid != 0)//kill parent process
        exit(0);
    pid = getpid();
    printf("process %d running as daemon\n", pid);
    setsid();//this process become independent
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGUSR1, already_process_handler);//handle sigusr1 signal
    maxfd = getdtablesize();

    for (fd = 0; fd < maxfd; ++fd)
        close(fd);
    umask(0);
    /* chdir("/"); */
    fd = open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    openlog("ssu_backup", LOG_PID, LOG_DAEMON);//open log for errors
    mkdir("backup",0777);//where backup files holded

    if((backup_log = fopen("backup_log.txt","a+")) == NULL)//open log for backups
    {
        syslog(LOG_ERR,"fopen error for %s", "backup_log.txt");
        exit(7);
    }

    if(stat(filename, &st) < 0)
    {
        syslog(LOG_ERR,"stat error for %s", filename);
        exit(1);
    }
    intertime = st.st_mtime;
    cur_count = -1;
    while(1)
    {
        sleep(period);
        if(D_OPTION)
        {
            backup_directory(filename);//backup directory files
            if(dir_file_count == 0 && cur_count == -1)
            {
                strcpy(thread_file_names[0],"");
                if(pthread_create(&tid[0],NULL,ssu_thread_backup,(void *) thread_file_names[0]) != 0)//make threads that works nothing
                {
                    syslog(LOG_ERR, "pthread Error : %s\n", thread_file_names[cur_count]);
                    exit(1);
                }
            }
            cur_count = dir_file_count;//max file count
            dir_file_count = 0;

        }
        else
            if(ssu_backup(filename, &intertime) < 0)
                break;
    }

    fclose(backup_log);
    closelog();
}

static void already_process_handler(int signo)
{
    char buf[512];
    time_t timer;
    struct tm t;
    timer = time(NULL);
    localtime_r(&timer, &t);
    sprintf(buf,"[%02d%02d %02d:%02d:%02d] %s<pid:%d> exit\n",
            t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, "ssu_backup",getpid());
    fputs(buf,backup_log);
    fclose(backup_log);
    closelog();
    exit(0);
}
void backup_directory(char *pathname)
{
    int i;
    int count;
    int len;

    struct stat statbuf;
    struct dirent **namelist;

    bool exists = false;


    if((count = scandir(pathname,&namelist,exclude_dot_folders,NULL)) == -1)
    {
        syslog(LOG_ERR, "Directory Scan Error : %s\n", pathname);
        exit(1);
    }

    strcat(pathname,"/");
    len = strlen(pathname);

    for(i = 0; i < count ; i++)
    {
        strcat(pathname,namelist[i]->d_name);

        if(stat(pathname,&statbuf) < 0)
        {
            syslog(LOG_ERR, "stat Error in directory : %s\n", pathname);
            exit(1);
        }
        if(S_ISDIR(statbuf.st_mode))//recursive call if it's directory
            backup_directory(pathname);
        else
        {
            if(cur_count == -1)//first time => initializing threads
            {
                strcpy(thread_file_names[dir_file_count], pathname);
                if(pthread_create(&tid[dir_file_count],NULL,ssu_thread_backup,(void *) thread_file_names[dir_file_count]) != 0)
                {
                    syslog(LOG_ERR, "pthread Error : %s\n", thread_file_names[dir_file_count]);
                    exit(1);
                }
            }
            else//later time => check new files
            {
                if(strcmp(thread_file_names[dir_file_count], pathname) != 0)//files in directory changed
                {
                    int j;
                    for (j = 0; j < cur_count; ++j) {
                        if(strcmp(thread_file_names[j],pathname) == 0)
                        {
                            exists = true;//file already exists
                            break;
                        }
                    }
                    if(!exists)//new file
                    {
                        if(cur_count == dir_file_count)//new file and nth file
                        {
                            if(cur_count == 0)//empty directory now has new file
                                strcpy(thread_file_names[cur_count],pathname);//overwrite null filename
                            else//new file is located at last of scandir
                            {
                                strcpy(thread_file_names[cur_count],thread_file_names[cur_count-1]);
                                if(pthread_create(&tid[cur_count],NULL,ssu_thread_backup,(void *) thread_file_names[cur_count]) != 0)
                                {
                                    syslog(LOG_ERR, "pthread Error : %s\n", thread_file_names[cur_count]);
                                    exit(1);
                                }
                                strcpy(thread_file_names[cur_count-1],pathname);
                            }
                        }
                        else//new file is located at middle of scandir
                        {
                            strcpy(thread_file_names[cur_count],thread_file_names[dir_file_count]);
                            if(pthread_create(&tid[cur_count],NULL,ssu_thread_backup,(void *) thread_file_names[cur_count]) != 0)
                            {
                                syslog(LOG_ERR, "pthread Error : %s\n", thread_file_names[cur_count]);
                                exit(1);
                            }
                            strcpy(thread_file_names[dir_file_count], pathname);//old thread changed
                        }
                        cur_count++;
                    }
                    else
                        exists = false;
                }
            }
            pthread_detach(tid[dir_file_count]);//make thread independent
            dir_file_count++;//total file count in directory
        }
        pathname[len] = 0;
    }
    for (i = 0; i < count; ++i) {
        free(namelist[i]);
    }
    free(namelist);
    pathname[len-1] = 0;
}
int exclude_dot_folders(const struct dirent *entry)//scan all files but exclude "." and ".." directory
{
    if(strcmp(entry->d_name, ".") == 0)
        return 0;
    else if(strcmp(entry->d_name, "..") == 0)
        return 0;
    else
        return 1;
}
pid_t get_process_id(char *processname)//get process id of that have worked already
{
    FILE *cmdresult;
    char cmd[100];
    pid_t pid;

    strcpy(cmd,"pidof ");
    strcat(cmd,processname);//use pidof

    if((cmdresult = popen(cmd,"r")) == NULL)//get result of pidof processname
    {
        fprintf(stderr, "popen error\n");
        exit(1);
    }
    while(fscanf(cmdresult, "%d", &pid) > 0)
    {
        if(pid == getpid())//this proces(should not be killed)
            continue;
        else
        {
            pclose(cmdresult);
            return pid;//already existed process id
        }
    }
    pclose(cmdresult);
    return -1;
}
void *ssu_thread_backup(void *arg)
{
    int i;
    struct stat st;
    time_t intertime;

    if(strcmp((char *)arg,"") != 0)//null filename
        if(stat((char *)arg , &st) < 0)
        {
            syslog(LOG_ERR, "stat Error : %s\n", (char *)arg);
            exit(1);
        }
    intertime = st.st_mtime;//get mtime of current file
    while(1)
    {
        if(strcmp(arg,"/") == 0)
            pthread_exit(NULL);
        if(ssu_backup((char *) arg, &intertime) < 0)
        {//file deleted
            if(strcmp(arg,thread_file_names[cur_count]) == 0)//last file
            {
                if(cur_count ==0)//no file left
                {
                    strcpy(arg,"");
                    continue;
                }
                else
                    pthread_exit(NULL);
            }
            else
                strcpy(arg,thread_file_names[cur_count]);//middle of file
            if(stat((char *)arg , &st) < 0)
            {
                syslog(LOG_ERR, "stat Error : %s\n", (char *)arg);
                exit(1);
            }
            intertime=st.st_mtime;//reset mtime
            strcpy(thread_file_names[cur_count],"/");
        }
        sleep(period);
    }
    return NULL;
}
