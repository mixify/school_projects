#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#define MAX_NTHREADS 128
#define MAX_FILE_NAME 255
#define MAX_BACKUP_FILE_NAME MAX_FILE_NAME+7//length of "backup/"
#define MAX_BACKUP_FILE_NUM 1024//used to n option
#define BUF_SIZ 1024

void get_input(int argc, char *argv[], char *filename);//get inputs
int ssu_daemon_init(char *filename);//make daemon process and backup
int ssu_backup(char *filename,time_t *time);//backup file and handle options
void backup_directory(char *pathname);//backup directory files using thread
int exclude_dot_folders(const struct dirent *entry);//scan all files but exclude "." and ".." directory
void *ssu_thread_backup(void *arg);//thread's work
static void already_process_handler(int signo);//handle signal "SIGUSR1"
pid_t get_process_id(char *processname);//get pid of existing process

int period;//interval for backup
FILE *backup_log;//log_file used at backup

int backup_file_count;//N_OPTION file count

int dir_file_count;//total files in directory
int cur_count;//previous total files in directory

int M_OPTION;
int N_OPTION;
int D_OPTION;
int C_OPTION;
int R_OPTION;

char thread_file_names[MAX_NTHREADS][255];
pthread_t tid[MAX_NTHREADS];
// struct t_data{
//     char filename[PATH_MAX];
//     time_t intertime;
// };
// struct t_data **filedata;
