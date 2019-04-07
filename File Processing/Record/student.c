#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "student.h"

struct HEADER{
    short records;
    short head;
} header;



void pack(char *recordbuf, const STUDENT *s);

void unpack(const char *recordbuf, STUDENT *s);

void read(FILE *fp, char *recordbuf, int rrn);

void add(FILE *fp, const STUDENT *s);

int search(FILE *fp, const char *keyval);

void delete(FILE *fp, const char *keyval);

void printRecord(const STUDENT *s, int n);


int main(int argc, char *argv[])
{
	FILE *fp;
    STUDENT std;
    char recordbuf[RECORD_SIZE];
    int num;

    if((fp = fopen(argv[2], "r+")) == NULL)
        fp = fopen(argv[2], "w+");

    if(fread(&header,HEADER_SIZE,1,fp) != 1)
    {
        header.records = 0;
        header.head = -1;
    }



    if(strcmp("-a",argv[1]) == 0)
    {
        strcpy(std.id,argv[3]);
        strcpy(std.name,argv[4]);
        strcpy(std.dept,argv[5]);
        strcpy(std.year,argv[6]);
        strcpy(std.addr,argv[7]);
        strcpy(std.phone,argv[8]);
        strcpy(std.email,argv[9]);
        add(fp, &std);
    }
    else if(strcmp("-s", argv[1]) == 0)
    {
        num = search(fp, argv[3]);
        if(num == -1)
            return 0;
        else
        {
            read(fp, recordbuf,num);
            unpack(recordbuf,&std);
            printRecord(&std,1);
        }
    }
    else if(strcmp("-d", argv[1]) == 0)
    {
        delete(fp, argv[3]);
    }

    fclose(fp);

    return 1;
}

void pack(char *recordbuf, const STUDENT *s)
{
    sprintf(recordbuf,"%s#%s#%s#%s#%s#%s#%s", s->id,s->name,s->dept,s->year,s->addr,s->phone,s->email);
}

void unpack(const char *recordbuf, STUDENT *s)
{
    char tmp[RECORD_SIZE];
    char *place;
    strcpy(tmp,recordbuf);

    place = strtok(tmp,"#");
    strcpy(s->id,place);

    place = strtok(NULL,"#");
    strcpy(s->name,place);

    place = strtok(NULL,"#");
    strcpy(s->dept,place);

    place = strtok(NULL,"#");
    strcpy(s->year,place);

    place = strtok(NULL,"#");
    strcpy(s->addr,place);

    place = strtok(NULL,"#");
    strcpy(s->phone,place);

    place = strtok(NULL,"#");
    strcpy(s->email,place);
}
void read(FILE *fp, char *recordbuf, int rrn)
{
    fseek(fp,HEADER_SIZE+RECORD_SIZE*rrn,SEEK_SET);
    fread(recordbuf,RECORD_SIZE,1,fp);
}

void add(FILE *fp, const STUDENT *s)
{
    char recordbuf[RECORD_SIZE];
    pack(recordbuf,s);
    if(header.head==-1)
    {
        fseek(fp,HEADER_SIZE+RECORD_SIZE*header.records,SEEK_SET);
        fwrite(recordbuf,RECORD_SIZE,1,fp);
    }
    else
    {
        fseek(fp,HEADER_SIZE+RECORD_SIZE*header.head + 1,SEEK_SET);
        fread(&header.head, 2, 1, fp);
        fseek(fp,-3,SEEK_CUR);
        fwrite(recordbuf,RECORD_SIZE,1,fp);
    }

    header.records++;

    fseek(fp,0,SEEK_SET);
    fwrite((char *)&header, HEADER_SIZE, 1, fp);
}

int search(FILE *fp, const char *keyval)
{
    int i;
    STUDENT s;
    char recordbuf[RECORD_SIZE];
    int record_num;
    fseek(fp,0,SEEK_END);
    record_num = (ftell(fp)-HEADER_SIZE) / RECORD_SIZE;
    for (i = 0;i < record_num;++i) {
        read(fp, recordbuf, i);
        if(recordbuf[0] == '*')
            continue;
        unpack(recordbuf,&s);
        if(memcmp(s.id,keyval,strlen(keyval)) == 0)
            return i;
    }
    return -1;
}

void delete(FILE *fp, const char *keyval)
{
    int i;
    STUDENT s;
    char recordbuf[RECORD_SIZE];
    i = search(fp,keyval);
    if(i==-1)
        return;
    fseek(fp, HEADER_SIZE + i * RECORD_SIZE,SEEK_SET);
    fwrite("*",1,1,fp);
    fwrite(&header.head,2,1,fp);
    header.records--;
    header.head = i;
    fseek(fp,0,SEEK_SET);
    fwrite((char *)&header, HEADER_SIZE, 1, fp);
    return;

}

void printRecord(const STUDENT *s, int n)
{
    int i;

    for(i=0; i<n; i++)
    {
        printf("%s#%s#%s#%s#%s#%s#%s\n", s[i].id, s[i].name, s[i].dept, s[i].year, s[i].addr, s[i].phone, s[i].email);
    }
}
