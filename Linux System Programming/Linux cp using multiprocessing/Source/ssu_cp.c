#include "cp_header.h"//
int main(int argc, char *argv[])
{
    struct stat src_stat;
    struct stat target_stat;
    char target[PATH_MAX];
    char source[PATH_MAX];


    get_input(argc, argv, source, target);//get inputs

    printf("target : %s\n", target);
    printf("src : %s\n", source);
    stat(source,&src_stat);
    stat(target,&target_stat);
    if(src_stat.st_ino == target_stat.st_ino)
    {
        fprintf(stderr, "%s and %s is same file\n", source, target);
        print_usage();
        exit(1);
    }

    cp_file(source, target, true);//copy file or directory

}

void get_input(int argc, char *argv[], char* source, char* target)
{
    int option;
    int i;
    while((option = getopt(argc, argv, "silnprd:SILNPRD:")) != -1)//get options (case non-sensitive)
    {
        switch(option)
        {
            case 'S':
            case 's':
                if(S_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                for (i = 1; i < 7; ++i) {
                    if(flags[i])
                    {
                        fprintf(stderr, "%c option should be used independently\n", option);
                        print_usage();
                        exit(1);
                    }
                }
                printf("%c option is on\n",option);
                S_OPTION = true;
                break;
            case 'I':
            case 'i':
                if(I_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                if(S_OPTION)
                {
                    fprintf(stderr, "s option should be used independently\n");
                    print_usage();
                    exit(1);
                }
                printf("%c option is on\n",option);
                I_OPTION = true;
                break;
            case 'L':
            case 'l':
                if(L_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                if(S_OPTION)
                {
                    fprintf(stderr, "s option should be used independently\n");
                    print_usage();
                    exit(1);
                }
                printf("%c option is on\n",option);
                L_OPTION = true;
                break;
            case 'N':
            case 'n':
                if(N_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                if(S_OPTION)
                {
                    fprintf(stderr, "s option should be used independently\n");
                    print_usage();
                    exit(1);
                }
                printf("%c option is on\n",option);
                N_OPTION = true;
                break;
            case 'P':
            case 'p':
                if(P_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                if(S_OPTION)
                {
                    fprintf(stderr, "s option should be used independently\n");
                    print_usage();
                    exit(1);
                }
                printf("%c option is on\n",option);
                P_OPTION = true;
                break;
            case 'R':
            case 'r':
                if(R_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                if(S_OPTION)
                {
                    fprintf(stderr, "s option should be used independently\n");
                    print_usage();
                    exit(1);
                }
                if(D_OPTION)
                {
                    fprintf(stderr, "can't use %c option with D option\n",option );
                    print_usage();
                    exit(1);
                }
                printf("%c option is on\n",option);
                R_OPTION = true;
                break;
            case 'D':
            case 'd':
                if(D_OPTION)
                {
                    fprintf(stderr, "%c option overlapped\n", option);
                    print_usage();
                    exit(1);
                }
                if(S_OPTION)
                {
                    fprintf(stderr, "s option should be used independently\n");
                    print_usage();
                    exit(1);
                }
                if(R_OPTION)
                {
                    fprintf(stderr, "can't use %c option with r option\n",option );
                    print_usage();
                    exit(1);
                }
                printf("%c option is on\n",option);
                D_OPTION = true;
                n = atoi(optarg);
                if(n <= 0 || n>=10)
                {
                    fprintf(stderr, "%s [-d][N] -> N should be integer (1 <= N <= 10)\n",argv[0] );
                    print_usage();
                    exit(1);
                }
                break;

        }
    }
    while((argv[optind] != NULL))
    {
        if(source[0] == 0)
            strcpy(source, argv[optind]);
        else
            strcpy(target, argv[optind]);

        optind++;
    }
}

void cp_file(char* source, char* target, bool is_it_target)
{
    bool src_file;
    int pid;//process id
    int status;

    char cmd;
    char buf[BUFFER_SIZE];
    char t_buf[200];

    int src_len;
    int target_len;

    int fd1, fd2;
    int length;

    int i;
    int count;
    int dir_count = 0;
    struct dirent **namelist;

    struct stat statbuf;
    struct utimbuf timebuf;
    struct group *grp;
    struct passwd *pwd;
    struct tm *lt;

    if((fd1 = open(source, O_RDONLY)) < 0)// open source
    {
        fprintf(stderr, "ssu_cp: %s:No Such file or directory\n", source);
        print_usage();
        exit(1);
    }

    if(stat(source,&statbuf) < 0)//get source's stat
    {
        fprintf(stderr, "stat error : %s\n", source);
        exit(1);
    }

    if(P_OPTION && is_it_target)//p option(print target file or directory information only)
    {
        printf("****************file info******************\n");

        printf("file name : %s\n", source);

        lt = localtime(&statbuf.st_atime);//thinking about recursive(thread)
        strftime(t_buf,sizeof(t_buf), "%d.%m.%Y %H:%M:%S", lt);
        printf("last data read time : %s\n", t_buf);

        lt = localtime(&statbuf.st_mtime);//thinking about recursive(thread)
        strftime(t_buf,sizeof(t_buf), "%d.%m.%Y %H:%M:%S", lt);
        printf("last data modify time : %s\n", t_buf);

        lt = localtime(&statbuf.st_ctime);//thinking about recursive(thread)
        strftime(t_buf,sizeof(t_buf), "%d.%m.%Y %H:%M:%S", lt);
        printf("last data change time : %s\n", t_buf);

        pwd = getpwuid(statbuf.st_uid);
        printf("OWNER : %s\n", pwd->pw_name);

        grp = getgrgid(statbuf.st_gid);
        printf("GROUP : %s\n", grp->gr_name);

        printf("file size : %ld\n", statbuf.st_size);

    }

    if(S_ISREG(statbuf.st_mode))//regular file
    {
        if((R_OPTION || D_OPTION) && is_it_target)
        {
            fprintf(stderr, "%s is not a directory\n", source);
            print_usage();
            exit(1);
        }

        if(I_OPTION || N_OPTION)
        {
            if(access(target,F_OK) == 0)
            {
                if(N_OPTION)
                    cmd = 'n';
                else//I_OPTION
                {
                    printf("overwrite %s (y/n)?", target);
                    scanf("%c", &cmd);
                    getchar();
                }
                if(cmd == 'y')//overwrite file
                {
                    if(remove(target) < 0)
                    {
                        fprintf(stderr, "remove error for %s\n", target);
                        exit(1);
                    }

                }
                else//don't overwrite and finish copy
                    return;
            }
        }


        if(S_OPTION)//symlink option
        {
            remove(target);
            //directory check needed
            if(symlink(source,target) < 0)
            {
                fprintf(stderr, "symlink error, %s already exists\n",target);
                print_usage();
                exit(1);
            }
        }
        else//actually copy files
        {
            if((fd2 = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
            {
                fprintf(stderr, "open error for %s\n", target);
                print_usage();
                exit(1);
            }

            while((length = read(fd1, buf,BUFFER_SIZE)) > 0)//copy files
                write(fd2, buf, length);
        }

    }
    else//directory
    {
        if((!R_OPTION && !D_OPTION) && is_it_target)//r or d option -> source should be directory
        {
            fprintf(stderr, "%s is not a file\n", source);
            print_usage();
            exit(1);
        }

        strcpy(src_path, source);//copy path
        src_path_len = strlen(src_path);//path length

        if((count = scandir(source, &namelist, exclude_dot_folders, type_sort)) == -1)//scan source's directory
        {
            fprintf(stderr, "Directory Scan Error : %s\n", source );
            print_usage();
            exit(1);
        }

        mkdir(target,0777);

        src_len = strlen(source);
        target_len = strlen(target);

        if(R_OPTION || !is_it_target)//R-option and recursive call
        {
            for(i = 0; i < count ; i++)
            {
                cp_inside_dir(source, target, src_len, target_len, namelist[i]->d_name);//copy directory files
            }
            for(i = 0; i < count ; i++)
                free(namelist[i]);
            free(namelist);
        }
        else if(is_it_target)//d option
        {
            for(i = 0 ; i< count ; i++)
            {
                if(is_dir(source, namelist[i]->d_name))
                    dir_count++;//count directory
            }
            for(i = 0 ; i < n ; i++)
            {
                if(i >= dir_count)//N is higher than directory count
                {
                    pid = fork();
                    if(pid == -1)
                    {
                        fprintf(stderr, "fork error\n");
                        exit(1);
                    }
                    else if(pid == 0)//child process doesn't copy anything
                    {
                        printf("no directory copied , pid = %d\n", getpid());
                        exit(0);
                    }
                }
                else if(is_dir(source, namelist[i]->d_name))
                {
                    pid = fork();
                    if(pid == -1)
                    {
                        fprintf(stderr, "fork error\n");
                        exit(1);
                    }
                    else if(pid == 0)//child process copy directory
                    {
                        cp_inside_dir(source, target, src_len, target_len, namelist[i]->d_name);
                        printf("directory : %s, pid = %d(child process)\n", namelist[i]->d_name, getpid());
                        exit(0);
                    }
                }
                else
                {
                    cp_inside_dir(source, target, src_len, target_len, namelist[i]->d_name);//parent process copy files
                }
            }
            for(; i < count ; i++)//N is lower than directory count -> parent process copy directories
            {
                if(is_dir(source, namelist[i]->d_name))
                    printf("directory : %s, pid = %d(parent process)\n", namelist[i]->d_name, getpid());
                cp_inside_dir(source, target, src_len, target_len, namelist[i]->d_name);
            }
            while(wait(&status) > 0);//wait until all child processes get finished
            /* printf("mother (%d) is over\n", getpid()); */
        }
    }



    if(L_OPTION && is_it_target)//copy informations of file or directory
    {

        if(chown(target, statbuf.st_uid, statbuf.st_gid) < 0)//copy source file's uid and gid
        {
            fprintf(stderr, "chown error : %s\n", target);
            exit(1);
        }

        if(chmod(target, statbuf.st_mode) < 0)//copy source file's mode
        {
            fprintf(stderr, "chmod error : %s\n", target);
            exit(1);
        }

        timebuf.actime = statbuf.st_atime;
        timebuf.modtime = statbuf.st_mtime;

        if(utime(target, &timebuf) < 0)//copy source file's time information
        {
            fprintf(stderr, "utime error : %s\n", target);
            exit(1);
        }
    }

}

int is_dir(char *path ,char *file_name)//check directory
{
    struct stat st;
    char tmp[PATH_MAX];
    strcpy(tmp,path);
    strcat(tmp,"/");
    strcat(tmp,file_name);
    if(stat(tmp, &st) < 0)
    {
        fprintf(stderr, "stat error %s\n",tmp);
        exit(1);
    }

    if(S_ISDIR(st.st_mode))
        return 1;
    return 0;
}

void print_usage()//print usage of program
{
    printf("ssu_cp error\n");
    printf("usage : in case of file\n");
    printf("cp [-i/n][-l][-p]       [source][target]\n");
    printf("or cp [-s][source][target]\n");
    printf("in case of directory cp [-i/n][-l][-p][-r][-d][N]\n");
}
