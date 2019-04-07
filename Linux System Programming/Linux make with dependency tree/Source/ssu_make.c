#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TMP_SIZE 1024//generally used to handle char strings

#define S_OPTION flags[0]
#define H_OPTION flags[1]
#define M_OPTION flags[2]
#define T_OPTION flags[3]//option flags

#define MACRO_MODE 1
#define PARSE_MODE 2//mode for parse_make function

struct macro{
    char* var;
    char* value;
};//used to store macro values

struct target_file{
    char target_file_name[100];
    char* dependency_file[100];
    char* target_file_commands[200];
    bool dropped;//used to check circular loop
};//used to handle target and its dependency, command

int flags[4];//shmt ordered option flags
int target_count;//number of targets in Makefile
int macro_count;//number of macros in Makefile

void get_input(int argc, char* argv[],char* fileName, char* directoryPath, char* command_target[5],int* targetcount, struct macro command_Macro[5],int* macrocount);
void parse_make(char* filename,struct target_file* target, struct macro* Macro, struct macro command_macro[5], int command_macro_count,int);
int find_target(char* str,struct target_file* target);
void execute_commands(struct target_file* target, int num, bool isittarget);
void make_target(struct target_file* target, char* command_target[],int);
void str_replace(char *original_str, char* place, char *insert_str,int size);
char* remove_end_spaces(char* str);
void check_macro(char* str, struct macro *Macro, struct macro *command_macro, int command_macro_count,char* target_name);
char* find_macro(char* str, struct macro* macro, struct macro command_macro[5], int command_macro_count);
void print_macro(struct macro Macro[], struct macro command_Macro[], int command_macro_count);
void print_tree(struct target_file* target, int n, bool);

int main(int argc, char* argv[])
{
    int i, j;//iterartor

    char fileName[TMP_SIZE] = "Makefile";//default value
    char directoryPath[TMP_SIZE] = "./";//default value

    char* command_target[5];//targets get inputs by commands
    int command_target_count = 0;//count of them

    struct macro command_Macro[5];//macros get inputs by commands
    int command_macro_count = 0;//count of them

    struct target_file target[TMP_SIZE];//used to store targets from Makefile
    struct macro Macro[TMP_SIZE];//used to store macros from Makefile

    get_input(argc,argv,fileName,directoryPath,command_target,&command_target_count,command_Macro,&command_macro_count);
    //get inputs from commands

    target_count = -1;
    macro_count = 0;

    parse_make(fileName,target,Macro, command_Macro, command_macro_count, MACRO_MODE);
    //parse macro first to input correct value when parse makefile

    parse_make(fileName,target,Macro, command_Macro, command_macro_count, PARSE_MODE);
    //parse makefile and get targets, dependencies, commands

    if(M_OPTION)
        print_macro(Macro,command_Macro, command_macro_count);//print macros when -m option is on
    if(T_OPTION)
    {

        printf("----------------------------graph------------------------\n");
        for(i = 0 ; i<=target_count ; i++)
        {
            print_tree(target,i,true);
            for(j = 0 ; j<=target_count ; j++)
            {
                target[j].dropped = false;
            }
        }

    }//print file dependency tree of Makefile when -t option is on
    if(!M_OPTION && !T_OPTION)//when option -mt is off make targets
        make_target(target,command_target,command_target_count);//make target files

    exit(0);
}


void get_input(int argc, char* argv[],char* fileName, char* directoryPath,char* command_target[5],int* targetcount, struct macro command_Macro[5],int* macrocount)
{
    int option;
    int i = 0;
    int j = 0;
    char* place = NULL;
    while ((option = getopt(argc, argv, "f:c:shmt")) !=-1)
        switch(option)
        {
            case 'f':
                strcpy(fileName, optarg);//change file name used as Makefile
                if(access(fileName,F_OK) == -1)//check file existence
                {
                    fprintf(stderr, "make: %s: No such file or directory\n", fileName);
                    exit(1);
                }
                break;
            case 'c':
                strcpy(directoryPath, optarg);//change working directory
                strcat(directoryPath,"/");
                if(chdir(directoryPath)<0)//if changing directory fails print error
                {
                    fprintf(stderr, "make: %s: No such directory\n", optarg);
                    exit(1);
                }
                break;
            case 's':
                flags[0] = 1;
                break;
            case 'h':
                flags[1] = 1;
                printf("Usage : ssu_make [Target] [Macro] [Option]\n");
                printf("Option:\n");
                printf("-f <file>       Use <file> as a makefile.\n");
                printf("-c <directory>  Change to <directory> before reading the makefiles.\n");
                printf("-s              Do not print the commands as they are executed\n");
                printf("-h              print usage\n");
                printf("-m              print macro list\n");
                printf("-t              print tree\n");
                exit(0);//print help and exit program
                break;
            case 'm':
                flags[2] = 1;
                break;
            case 't':
                flags[3] = 1;
                break;
            default://option error
                fprintf(stderr,"there's only option with f,c,s,h,m,t\n");
                exit(1);
        }
    while((argv[optind] != NULL))
    {
         if((place = strchr(argv[optind],'='))!=NULL)
         {
             *place = '\0';
             command_Macro[j].var = argv[optind];
             command_Macro[j].value = place+1;
             j++;
         }//get macros from command
         else
             command_target[i++] = argv[optind];//get targets from command
         optind++;
    }
    *targetcount = i;
    if(i > 5)
    {
        fprintf(stderr, "make: Can't put targets more than 5\n");//target count error
        exit(1);
    }
    *macrocount = j;
    if(j > 5)
    {
        fprintf(stderr, "make: Can't put macros more than 5\n");//macro count error
        exit(1);
    }
}

void parse_make(char* filename,struct target_file* target, struct macro* Macro, struct macro command_macro[5], int command_macro_count, int mode)
{
    FILE *pFile = NULL;
    char line[TMP_SIZE];
    char tmp_line[TMP_SIZE] = {'\0',};
    char* macro_line;
    char* place = NULL;
    char* save_ptr;
    char target_str[TMP_SIZE] = {0,};
    int i = 0;
    int j = 0;
    int length;
    int line_count = 0;

    if((pFile = fopen(filename, "r")) == 0){
        fprintf(stderr,"make: %s: No such file or directory\n",filename);
        exit(1);
    }//file open check

    while(true)
    {
        if(line[0] != 0)
        {
            memset(line,0,1024);//used to initialize line array
        }
        if(fgets(line,TMP_SIZE,pFile) < 0)
        {
            fprintf(stderr,"make: read error %s",filename);
            exit(1);
        }

        line_count++;//line count used to guide where Makefile has missing seperator

        if(feof(pFile))//break while(1) if read end of file
            break;
        if(tmp_line[0] != '\0')//append commands that has saved when \ appears
        {
            strcat(tmp_line,line);
            strcpy(line,tmp_line);
            tmp_line[0] = '\0';
        }

        length = strlen(line);

        if(*line == ' ')//can't start components with empty space
        {
            for(j = 0 ; line[j] == 0 ; j++)
            {
                if(line[j] != ' ')
                {
                    fprintf(stderr, "%s:%d: *** missing separator. Stop.\n", filename,line_count);
                    exit(1);
                }
            }
            continue;
        }

        if((place = strchr(line,'#'))!=NULL)//comment should be at first word
        {
            if(line[0] == '#')
                continue;
            else
            {
                fprintf(stderr, "%s:%d: *** missing separator. Stop.\n", filename,line_count);
                exit(1);
            }
        }


        if((place = strchr(line,'\\'))!=NULL)//long line are saved and attached to new line
        {
            *place = '\0';
            strcpy(tmp_line,line);
            if(*(place+1) != '\n')
            {
                fprintf(stderr, "%s:%d: *** missing separator. Stop.\n", filename,line_count);
                exit(1);
            }
            continue;
        }
        if(mode == PARSE_MODE)//change macros to actual values before parse into targets
        {
            check_macro(line,Macro,command_macro,command_macro_count,target_str);
        }

        if(line[0] == '\t' && mode == PARSE_MODE)//parse commands
        {
            if(target_count == -1)//commands without target
            {
                fprintf(stderr, "%s:%d: *** missing separator. Stop.\n", filename,line_count);
                exit(1);
            }
            target[target_count].target_file_commands[i]= malloc(TMP_SIZE);
            strcpy(target[target_count].target_file_commands[i++], line + 1);
            continue;
        }

        else if((place = strchr(line,':'))!=NULL && mode == PARSE_MODE)//parse target and dependencies
        {
            i=0;
            target_count++;
            strcpy(target[target_count].target_file_name,place = remove_end_spaces(strtok(line,":")));
            strcpy(target_str,place);
            while(place = strtok(NULL," \t"))//get dependencies
            {
                if(strcmp(remove_end_spaces(place), "") == 0)//don't get dependencies with empty space
                    break;
                target[target_count].dependency_file[i] = malloc(TMP_SIZE);
                strcpy(target[target_count].dependency_file[i] ,remove_end_spaces(place));
                i++;
            }
            i=0;

        }

        else if((place = strchr(line,'='))!=NULL && mode == MACRO_MODE)//parse macros
        {
             if(*(place-1) == '?')//used to handle ?= macro
             {
                 *(place-1) = '\0';
                 if(find_macro(remove_end_spaces(line), Macro, command_macro, command_macro_count) != NULL)
                     continue;
                 else
                 {
                    *place = '\0';
                    Macro[macro_count].var = malloc(length);
                    Macro[macro_count].value = malloc(length);

                    strcpy(Macro[macro_count].var ,remove_end_spaces(line));
                    strcpy(Macro[macro_count].value , remove_end_spaces(place+1));

                    macro_count++;
                 }
             }
             else
             {
                *place = '\0';

                Macro[macro_count].var = malloc(length);
                Macro[macro_count].value = malloc(length);

                strcpy(Macro[macro_count].var ,remove_end_spaces(line));
                strcpy(Macro[macro_count].value , remove_end_spaces(place+1));

                macro_count++;
             }
        }

        else if(strncmp(line,"include",7) == 0)//include files
        {
            place = strtok_r(line," \t", &save_ptr);
            while(place = strtok_r(NULL," \t", &save_ptr))//get includes using recursive
            {
                parse_make(remove_end_spaces(place),target,Macro,command_macro,command_macro_count,mode);
            }

        }
        else//error
        {
            if(MACRO_MODE)
                continue;
            if(line[0] == '\n')
                continue;
            fprintf(stderr, "%s:%d: *** missing separator. Stop.\n", filename,line_count);
            exit(1);
        }
    }
    fclose(pFile);

    if(target_count == -1 && mode == PARSE_MODE)//no target error
    {
        fprintf(stderr, "make: *** No targets. Stop\n");
        exit(1);
    }
}

void make_target(struct target_file* target, char* command_target[],int target_count)
{
    int i;
    int num;

    for(i = 0 ; i<target_count ; i++)//make targets that inputted by commands
    {
        if((num = find_target(command_target[i], target)) < 0)//check target existence on makefile
        {
            fprintf(stderr,"No rule to make target '%s'. Stop\n" , command_target[i]);
            exit(1);
        }
        execute_commands(target, num, true);
    }

    if(i == 0)//if no targets get inputted by commands make first target
    {
        num = 0;
        execute_commands(target, num, true);
    }
}

int find_target(char* str, struct target_file* target)//find target with str name
{
    int i = 0;
    for(i = 0 ; i <= target_count ; i++)
    {
        if(strcmp(str,target[i].target_file_name ) == 0)
            return i;
    }

    return -1;
}

char* find_macro(char* str, struct macro* macro, struct macro command_macro[5], int command_macro_count)//find macro with str name
{
    int i;
    for(i = command_macro_count-1 ; i >= 0 ; i--)
    {
        if(strcmp(str,command_macro[i].var) == 0)
            return command_macro[i].value;
    }
    for(i = macro_count-1 ; i >= 0 ; i--)
    {
        if(strcmp(str,macro[i].var) == 0)
            return macro[i].value;
    }

    return NULL;
}

void execute_commands(struct target_file* target, int num, bool isittarget)//make target file using recursive
{
    int i;
    int exists;
    char* cmd;
    char* d_file;

    struct stat target_statbuf;
    struct stat depend_statbuf;
    exists = stat(target[num].target_file_name, &target_statbuf);// if nofile -1 else 0

    target[num].dropped = true;//used to check circular loop

    for(int i = 0 ; target[num].dependency_file[i] !=NULL  ; i++)
    {
        if(find_target(target[num].dependency_file[i], target) < 0)
        {
            if(access(target[num].dependency_file[i], F_OK) == -1)//doesn't exist both on target and file
            {
                fprintf(stderr, "make: *** No rule to make target '%s', needed by '%s' Stop\n",target[num].dependency_file[i], target[num].target_file_name);
                exit(1);
            }
        }
        if(target[find_target(target[num].dependency_file[i],target)].dropped == true)
        {
            if(find_target(target[num].dependency_file[i],target) >= 0)//circular loop
            {
                printf("make: Circular %s <- %s dependency dropped.\n", target[num].target_file_name, target[num].dependency_file[i]);
                target[num].dropped= false;
                continue;
            }
        }
        execute_commands(target, find_target(target[num].dependency_file[i], target), false);//recursive
        if(exists + stat(target[num].dependency_file[i], &depend_statbuf) == 0)
        {
            if(difftime(target_statbuf.st_mtime, depend_statbuf.st_mtime) >= 0)
                continue;
        }
        exists--;//if no files modified it dosen't change
    }

    if(exists == 0)
    {
        if(isittarget)//when it's target and no dependecies are modified
            printf("make: '%s' is up to date\n", target[num].target_file_name);
        return;
    }


    i = 0;

    while((cmd = target[num].target_file_commands[i++] )!= NULL)
    {
        if(!S_OPTION)//don't print commands when -s option
            printf("%s",cmd);
        system(cmd);//execute commands
    }
}

void check_macro(char* str, struct macro *Macro, struct macro *command_macro, int command_macro_count, char* target_name)//check macros in string and replace it
{
    char* place;
    char* closing_place;
    char* macro_str;
    char* str_p = str;
    char tmp_str[1024] = {0,};
    char tmp_str2[1024] = {0,};
    while((place = strchr(str_p, '$')) != NULL)
    {
        if(*(place+1) == '*')//target macro without extension
        {
            strcpy(tmp_str, target_name);
            *strchr(tmp_str,'.') = 0;
            str_replace(str,place,tmp_str,2);
        }
        else if(*(place+1) == '@')//target macro
        {
            strcpy(tmp_str, target_name);
            str_replace(str,place,tmp_str,2);
        }
        else if(*(place+1) == '(')
        {
            if((closing_place = strchr(str_p,')')) != NULL)//macro
            {
                strncpy(tmp_str, place,closing_place-place+1);
                strncpy(tmp_str2, place+2,closing_place-place-2);
                macro_str=find_macro(tmp_str2,Macro,command_macro, command_macro_count);
                if(macro_str != NULL)
                    str_replace(str,place,macro_str,closing_place - place + 1);
                else
                {
                    str_replace(str,place,"",closing_place-place+1);//when there's no macro replace it with empty space
                }
            }
        }

        else
        {
            str_p = place+1;//$ without proper components
        }

    }

}
void str_replace(char *original_str, char* place, char *insert_str,int size)//replace macro
{
    original_str[place-original_str] = 0;
    char* tail;
    tail = (char *) malloc(512);

    strcpy(tail,place + size);
    memcpy(place,insert_str,strlen(insert_str)+1);
    strcat(original_str,tail);

}


bool is_space(char str)//check is it space
{
    switch(str)
    {
        case '\t':
            return true;
        case '\n':
            return true;
        case ' ':
            return true;
        default:
            return false;
    }
}

char* remove_end_spaces(char* str)//remove new line, tab, space
{
    char *end;

    while(is_space(*str)) str++;

    if(*str == 0)
        return str;

    end = str + strlen(str) -1;
    while(end > str && is_space(*end)) end--;

    *(end+1) = 0;

    return str;

}


void print_macro(struct macro Macro[], struct macro command_Macro[], int command_macro_count)//print macros
{
    int i, j;
    char* already_macro[TMP_SIZE];//macros that already printed
    int already_macro_count = 0;
    bool has_macro = false;
    printf("----------------macro list--------------------\n");
    for(i = command_macro_count-1 ; i >= 0 ; i--)//print macros from command
    {
        for(j = 0 ; j < already_macro_count ; j++)
        {
            if(strcmp(command_Macro[i].var,already_macro[j]) == 0)
            {
                has_macro = true;
                break;
            }
        }
        if(has_macro)
        {
            has_macro = false;
            continue;
        }
        printf("%s = %s\n", command_Macro[i].var, command_Macro[i].value);
        already_macro[already_macro_count++] = command_Macro[i].var;
    }

    for(i = macro_count-1 ; i >= 0 ; i--)//print macros from Makefile
    {
        for(j = 0 ; j < already_macro_count ; j++)
        {
            if(strcmp(Macro[i].var,already_macro[j]) == 0)
            {
                has_macro = true;
                break;
            }
        }

        if(has_macro)
        {
            has_macro = false;
            continue;
        }
        printf("%s = %s\n", Macro[i].var, Macro[i].value);
        already_macro[already_macro_count++] = Macro[i].var;
    }
}

void print_tree(struct target_file* target, int n, bool first)//print tree
{
    int i;
    static int word_count = 0;//stack printf's return value
    static int t_word_count = 0;
    int tmp;
    static bool lf = false;//it becomes true when print new line
    if(lf)
    {
        printf("%*s",word_count-1,"");
        lf=false;
    }
    if(first)
    {
        word_count++;
        first = false;
        if(target[n].dependency_file[0] == NULL)
        {
            printf("%s\n", target[n].target_file_name);
            return;
        }
    }
    else
        word_count += printf(">");
    word_count += printf("%s", target[n].target_file_name);
    t_word_count = word_count-1;
    if(target[n].dropped == true)
    {
        printf("\n");
        target[n].dropped= false;
        return;
    }

    target[n].dropped = true;//check circular loop

    for(i = 0 ; target[n].dependency_file[i] != NULL ; i++)
    {
        if(i==0)
            tmp = t_word_count;
        if(find_target(target[n].dependency_file[i],target) == -1)//it's not target
        {
            if(lf == false)
            {
                printf(">%s\n",target[n].dependency_file[i]);
                lf = true;
            }
            else
            {
                printf("%*s%s\n", word_count,">", target[n].dependency_file[i]);
            }
        }
        else//its target
        {
            print_tree(target, find_target(target[n].dependency_file[i], target ), first);
            word_count=tmp+1;
            lf = true;
        }
    }
    word_count = 0;
    if(i==0)
    {
        printf("\n");
        lf= true;
    }
    else
        lf = false;
}
