#include "cp_header.h"

int exclude_dot_folders(const struct dirent *entry)//scan all files but exclude "." and ".." directory
{
    if(strcmp(entry->d_name, ".") == 0)
        return 0;
    else if(strcmp(entry->d_name, "..") == 0)
        return 0;
    else
        return 1;
}
void cp_inside_dir(char* source, char* target, int src_len, int target_len, char* filename)//copy directory
{
    strcat(source,"/");
    strcat(source,filename);
    strcat(target,"/");
    strcat(target,filename);
    cp_file(source,target,false);
    source[src_len] = 0;
    target[target_len] = 0;
}

int type_sort(const struct dirent** file1, const struct dirent** file2)//sort by type(file, directory)
{
    struct stat st1, st2;
    strcat(src_path,"/");
    strcat(src_path,(*file1)->d_name);
    stat(src_path, &st1);
    src_path[src_path_len] = 0;
    strcat(src_path,"/");
    strcat(src_path,(*file2)->d_name);
    stat(src_path,&st2);
    src_path[src_path_len] = 0;
    return (S_ISDIR(st2.st_mode) - S_ISDIR(st1.st_mode));

}
