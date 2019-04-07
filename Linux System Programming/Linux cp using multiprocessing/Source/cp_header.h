#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>

// option flags
#define S_OPTION flags[0]
#define I_OPTION flags[1]
#define L_OPTION flags[2]
#define N_OPTION flags[3]
#define P_OPTION flags[4]
#define R_OPTION flags[5]
#define D_OPTION flags[6]

// buffer size
#define BUFFER_SIZE 1024

int n;// value of d option
bool flags[7];//option flags array
char src_path[PATH_MAX];//used to get directory stat
int src_path_len;//used to get directory stat

void get_input(int argc, char *argv[], char* source, char* target);
void cp_file(char* source, char* target, bool is_it_target);
int exclude_dot_folders(const struct dirent *entry);
int folder_select(const struct dirent *entry);
int is_dir(char *target,char *file_name);
void cp_inside_dir(char* source, char* target, int src_len, int target_len, char* filename);
int type_sort(const struct dirent** file1, const struct dirent** file2);
void print_usage();
