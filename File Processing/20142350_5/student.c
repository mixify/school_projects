#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "student.h"

void makeRuns(FILE *inputfp, char *inputbuf);
void internalsort(char *inputbuf, int n);
void kwaymerge(FILE *outputfp, char *inputbuf, char *outputbuf);
void pack(char *recordbuf, const STUDENT *s);
void unpack(const char *recordbuf, STUDENT *s);
void readChunk(FILE *runfp, char *inputbuf, int chunkid);
void writeOutputbuf(FILE *outputfp, const char *outputbuf, int n);

int get_min_record(const char *inputbuf);

int file_size;
int chunk_size;
int num_runs;
int *num_run_records;
int num_records_chunk;
int i;

typedef struct RUN_DATA_{
    int record_index;
    int run_records_num;
}run_data;

run_data *run_datas;

int compare(const void *firstbuf, const void *secondbuf)
{
    int id1, id2;
    STUDENT s1, s2;
    unpack((char *)firstbuf,&s1);
    unpack((char *)secondbuf,&s2);

    id1 = atoi(s1.id);
    id2 = atoi(s2.id);

    if(id1 > id2)
        return 1;
    if(id1 < id2)
        return -1;
    else
        return 0;

}


int main(int argc, char *argv[])
{
    struct stat st;

    int run_size;

    int record_data;
    FILE *inputfp, *outputfp;
    char inputbuf[INPUT_BUF_SIZE];
    char outputbuf[OUTPUT_BUF_SIZE];

    char run_name[10];

    if(strcmp(argv[1],"-as") == 0)
    {
        char header[4];
        stat(argv[2],&st);
        record_data = open(argv[2], O_RDONLY);
        outputfp=fopen(argv[3],"w+");
        read(record_data,header,HEADER_SIZE);
        fwrite(header,HEADER_SIZE,1,outputfp);
    }

    file_size = st.st_size - HEADER_SIZE;
    num_runs = (file_size / INPUT_BUF_SIZE) + ((file_size%INPUT_BUF_SIZE)?1:0);
    chunk_size = (RECORDS_NUM_INPUTBUF / num_runs) * RECORD_SIZE;
    num_records_chunk = chunk_size / RECORD_SIZE;

    num_run_records = (int *) malloc(sizeof(int) * num_runs);
    run_datas = (run_data *) malloc(sizeof(run_data) * num_runs);

    lseek(record_data,HEADER_SIZE, SEEK_SET);

    for (i = 0; i < num_runs; ++i) {
        sprintf(run_name,"run_%d",i);
        inputfp = fopen(run_name,"w+");
        run_size = read(record_data, inputbuf, INPUT_BUF_SIZE);
        run_datas[i].run_records_num = run_size/RECORD_SIZE;
        makeRuns(inputfp, inputbuf);
    }


    kwaymerge(outputfp, inputbuf, outputbuf);

    for (i = 0; i < num_runs; ++i)
    {
        sprintf(run_name,"run_%d",i);
        inputfp = fopen(run_name,"w+");
        remove(run_name);
    }
    free(num_run_records);
    free(run_datas);

    fclose(inputfp);
    fclose(outputfp);

    return 1;
}

//
//�־��� ���ڵ� ���Ͽ��� input buffer ũ�⸸ŭ ���ڵ带 �о� ���� ��
//internal sort ������� '�й�' Ű�� ���� ������ �Ͽ� �� �����
//run file�� �����Ѵ�. �̶� ���� ���� run file�� ������ �� �ִ�.
//
void makeRuns(FILE *inputfp, char *inputbuf)
{
    internalsort(inputbuf, run_datas[i].run_records_num);
    fwrite(inputbuf, RECORD_SIZE * run_datas[i].run_records_num, 1, inputfp);
    fclose(inputfp);
}

//
//makeRuns() ���ο��� ȣ���ϴ� ���� �Լ��μ� ���� ���, C ���̺귯������ �����ϴ�
//qsort()�Լ��� ȣ���ؼ� ������ �� ������ �̷� ��� �޵�� compare() �Լ���
//�����ؾ� �Ѵ�. input buffer�� ����Ǿ� �ִ� ���ڵ���� �й� Ű�� ���� ������������
//�����Ѵ�.
//�Ķ���� n�� inputbuf�� ����Ǿ� �ִ� ���ڵ��� ���� ��Ÿ����.
//
void internalsort(char *inputbuf, int n)
{
    qsort(inputbuf,n,RECORD_SIZE, compare);
}

//
//run file���� ������� k-way merge sort�� �����Ѵ�.
//�̶� �ݵ�� �־��� input buffer�� output buffr�� ����ؾ� �Ѵ�.
//
void kwaymerge(FILE *outputfp, char *inputbuf, char *outputbuf)
{
    int index;
    int n = 0;
    char run_name[10];
    FILE *runfp[num_runs];

    int count;

    char tmpbuf[INPUT_BUF_SIZE];
    memset(tmpbuf, 0xFF, INPUT_BUF_SIZE);
    for (i = 0; i < num_runs; ++i) {
        sprintf(run_name,"run_%d",i);
        runfp[i] = fopen(run_name,"r+");
        readChunk(runfp[i], inputbuf, i);
        run_datas[i].record_index = 0;
    }

    while(1)
    {
        count = 0;

        index = get_min_record(inputbuf);

        memcpy(outputbuf+n*RECORD_SIZE, inputbuf+chunk_size*index + run_datas[index].record_index * RECORD_SIZE, RECORD_SIZE);
        n++;

        run_datas[index].record_index++;
        run_datas[index].run_records_num--;

        if(run_datas[index].record_index == num_records_chunk)
        {
            readChunk(runfp[index], inputbuf, index);
            run_datas[index].record_index = 0;
        }

        if(n==9)
        {
            STUDENT s;
            writeOutputbuf(outputfp,outputbuf,n);
            memset(outputbuf, 0, OUTPUT_BUF_SIZE);
            n=0;
        }

        for (i = 0; i < num_runs; ++i)
            if(run_datas[i].run_records_num != 0)
                count++;

        if(count == 0)
            break;
    }
    writeOutputbuf(outputfp,outputbuf,n);
    for (i = 0; i < num_runs; ++i)
        fclose(runfp[i]);
}

int get_min_record(const char *inputbuf)
{
    int i;
    STUDENT s;
    int min_record = 99999999, min_index = 0;
    int id;

    for (i = 0; i < num_runs; ++i) {
        if(run_datas[i].run_records_num == 0)
            continue;
        unpack(inputbuf+i*chunk_size + RECORD_SIZE * run_datas[i].record_index,&s);
        id = atoi(s.id);
        if(id < min_record)
        {
            min_record = id;
            min_index = i;
        }
    }
    return min_index;

}
//
//����4���� ������ �Լ��μ� �ʿ��ϸ� ����Ѵ�. (�׷��� ������ ���� ����� �ʿ䰡 ����)
//
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

//
//K-way merge sort���� run���κ��� �ش� chunk�� input buffer�� �о� ���� �� ����Ѵ�.
//�̶� �о���� chunk�� input buffer���� �ڽ��� ��ġ�� �����Ѵ�.
//�־��� run�� �� ��°��� �о���� chunk�� input buffer���� �� ��° chunk ��ġ��
//�����Ѵ�.
//
void readChunk(FILE *runfp, char *inputbuf, int chunkid)
{
    if(run_datas[chunkid].run_records_num >= num_records_chunk)
        fread(inputbuf+chunk_size*chunkid,chunk_size,1,runfp);
    else
        fread(inputbuf+chunk_size*chunkid,run_datas[chunkid].run_records_num * RECORD_SIZE,1,runfp);
}

//
//k-way merge sort�� �߰� ��� ���ڵ尡 output buffer�� ������ ���� ���ο� ���ڵ� ���Ͽ�
//output buffer�� ���ڵ���� �����Ѵ�. ���� sort�� ������ �κп��� output buffer��
//���� �ʾƵ� ���Ͽ� �����ؾ� �� ��쵵 �߻��� �� ������ 'n' �Ķ���Ͱ� �ʿ��ϴ�.
//
void writeOutputbuf(FILE *outputfp, const char *outputbuf, int n)
{
    fwrite(outputbuf, RECORD_SIZE * n, 1, outputfp);
}
