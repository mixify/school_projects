#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "main.h"


void set_file_name(const char *filename, char *b_filename, char *ab_path);
char* format_date(char *str, time_t val);
void alpha_sort(char str[][MAX_BACKUP_FILE_NAME],int count);
void copy_file(int fd1, char *b_filename);
//filename , period
int ssu_backup(char *filename, time_t *intertime)
{
    int fd1;

    struct stat st;

    char ab_path[MAX_FILE_NAME];//absolute path
    char b_filename[MAX_BACKUP_FILE_NAME];//filename that is converted to hex
    char time_str[100];//string for time
    char *log_name;//name that used with log


    struct tm t;
    time_t timer;

    int count;
    int i;

    if(strcmp(filename,"") == 0)//null filename => do nothing and return
        return 0;
    set_file_name(filename, b_filename,ab_path);//set backup filename and absolute path
    if((log_name = strrchr(filename,'/')) == NULL)//get only filename without path
        log_name = filename;
    else
        log_name++;

    timer = time(NULL);
    localtime_r(&timer, &t);

    char dir_str[MAX_BACKUP_FILE_NAME];
    char *dir_ptr;

    if((fd1 = open(filename, O_RDONLY)) < 0)//open original file
    {
        if(access(filename,F_OK) < 0)
        {
            fprintf(backup_log,"[%02d%02d %02d:%02d:%02d] %s is deleted\n",
                    t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, log_name );
            fflush(backup_log);
            return -1;
        }
    }


    i = 0;
    count = 0;



    if(N_OPTION || C_OPTION || R_OPTION)
    {
        struct dirent *dentry;
        DIR *dirp;
        char b_names[MAX_BACKUP_FILE_NUM][MAX_BACKUP_FILE_NAME];

        if((dirp = opendir("backup")) == NULL)
        {
            syslog(LOG_ERR, "Directory open error : %s\n", "backup");
            exit(1);
        }
        while((dentry = readdir(dirp)) != NULL)
        {
            if(strcmp(dentry->d_name, ".") && strcmp(dentry->d_name, ".."))//exclude ".", ".." directory
            {
                if(strncmp(dentry->d_name, b_filename + 7, strlen(ab_path)*2) == 0)//backup file of original file
                {
                    strcpy(b_names[count], dentry->d_name);
                    count++;
                }
            }
        }
        alpha_sort(b_names,count);//use alpha_sort to sort files with backup time
        if(C_OPTION)
        {
            if(count==0)
            {
                printf("backup file for %s doesn't exist\n",log_name);
                exit(1);
            }
            dir_ptr = strchr(b_names[count-1],'_');//it point to backup time
            printf("<Compare with backup file[%s%s]>\n",log_name, dir_ptr);
            strcpy(dir_str,"backup/");
            strcpy(dir_str+7,b_names[count-1]);
            if(fork() == 0)
                execlp("diff","diff",filename,dir_str,(char *) 0);//fork and use diff command
            exit(0);
        }
        else if(R_OPTION)
        {
            if(count==0)
            {
                printf("backup file for %s doesn't exist\n",log_name );
                exit(0);
            }
            int file_num;//chosen file
            printf("<%s backup list>\n", log_name);
            printf("0. exit\n");
            for (i = 0; i < count; ++i) {
                strcpy(dir_str,"backup/");
                strcpy(dir_str+7,b_names[i]);
                if(stat(dir_str, &st) < 0)
                {
                    syslog(LOG_ERR, "stat error : %s\n", dir_str);
                    exit(1);
                }
                dir_ptr = strchr(b_names[i],'_');
                printf("%d. %s%s [size:%ld]\n",i+1, log_name, dir_ptr,st.st_size);
            }
            printf("input : ");
            scanf("%d",&file_num);
            if(file_num==0)
                exit(0);//exit program
            file_num--;
            printf("Recovery backup file...\n");

            strcpy(dir_str,"backup/");
            strcpy(dir_str+7,b_names[file_num]);//chosen file
            if((fd1 = open(dir_str, O_RDONLY)) < 0)
            {
                syslog(LOG_ERR,"open error for %s",b_filename);
                exit(1);
            }
            printf("[%s]\n", filename);
            copy_file(fd1, filename);
            //recover and print it to terminal
            exit(0);
        }
        i = 0;
        while(count >= (M_OPTION?backup_file_count+1:backup_file_count))//handle N count of backup files
        {
            strcpy(dir_str,"backup/");
            strcpy(dir_str+7,b_names[i]);//old file
            remove(dir_str);
            stat(filename, &st);
            dir_ptr = strchr(b_names[i],'_');
            fprintf(backup_log,"[%02d%02d %02d:%02d:%02d] Delete backup [%s, size:%ld, btime:%.2s%.2s %.2s:%.2s:%.2s]\n",
                    t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, log_name,
                    st.st_size, dir_ptr+1, dir_ptr+3, dir_ptr+5, dir_ptr+7, dir_ptr+9
                   );
            i++;
            count--;
        }
    }

    if(stat(filename, &st) < 0)
    {
        syslog(LOG_ERR, "stat error : %s\n", filename);
        exit(1);
    }

    if(st.st_mtime != *intertime)//file get modified
    {
        fprintf(backup_log,"[%02d%02d %02d:%02d:%02d] %s is modified [size:%ld/mtime:%s]\n",
                t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, log_name,
                st.st_size, format_date(time_str,st.st_mtime));
        *intertime = st.st_mtime;
        copy_file(fd1,b_filename);
    }
    else//not modified
    {
        if(!M_OPTION)
        {
            fprintf(backup_log,"[%02d%02d %02d:%02d:%02d] %s [size:%ld/mtime:%s]\n",
                    t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, log_name,
                    st.st_size, format_date(time_str,st.st_mtime));
            copy_file(fd1, b_filename);
        }
    }


    close(fd1);
    fflush(backup_log);//flush log file buffer
    return 1;
}

void set_file_name(const char *filename, char *b_filename, char *ab_path)
{
    int i;
    struct tm t;
    time_t timer;
    realpath(filename,ab_path);//get absolute path
    strcpy(b_filename,"backup/");
    for (i = 0; i < strlen(ab_path); ++i) {
        sprintf(b_filename+7+2*i,"%x",ab_path[i]);//get hexadecimal name
    }

    timer = time(NULL);
    localtime_r(&timer, &t);
    sprintf(b_filename+7 + 2*i,"_%02d%02d%02d%02d%02d",t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    //get timestamp
    if(strlen(b_filename)>261)//backup file name max exceeded
    {
        syslog(LOG_ERR, "filename is too long");
        exit(1);
    }
}
char* format_date(char *str, time_t val)
{
    strftime(str, 36, "%m%d %H:%M:%S", localtime(&val));
    return str;
}
void alpha_sort(char str[][MAX_BACKUP_FILE_NAME],int count)//sort alphabetically
{
    int i,j;
    char temp[255];
    for (i = 0; i < count; ++i) {
        for (j = i+1; j < count; ++j) {
            if(strcmp(str[i],str[j]) > 0)
            {
                strcpy(temp,str[i]);
                strcpy(str[i],str[j]);
                strcpy(str[j],temp);
            }
        }
    }
}
void copy_file(int fd1, char *b_filename)//copy file
{
    int fd2;
    int length;//length that used to copy file
    char buf[BUF_SIZ];//buf that used to copy file
    if((fd2 = open(b_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)//new backup file
    {
        syslog(LOG_ERR,"open error for %s",b_filename);
        exit(1);
    }
    while((length = read(fd1,buf,BUF_SIZ)) > 0)//copy files
    {
        write(fd2, buf, length);
        if(R_OPTION)
            write(STDOUT_FILENO,buf, length);//write to standard out
    }

}
