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
//주어진 레코드 파일에서 input buffer 크기만큼 레코드를 읽어 들인 후
//internal sort 방식으로 '학번' 키에 대해 정렬한 하여 그 결과를
//run file에 저장한다. 이때 여러 개의 run file이 생성될 수 있다.
//
void makeRuns(FILE *inputfp, char *inputbuf)
{
    internalsort(inputbuf, run_datas[i].run_records_num);
    fwrite(inputbuf, RECORD_SIZE * run_datas[i].run_records_num, 1, inputfp);
    fclose(inputfp);
}

//
//makeRuns() 내부에서 호출하는 정렬 함수로서 예를 들면, C 라이브러리에서 제공하는
//qsort()함수를 호출해서 구현할 수 있으며 이런 경우 받드시 compare() 함수를
//제공해야 한다. input buffer에 저장되어 있는 레코드들을 학번 키에 따라 오름차순으로
//정렬한다.
//파라미터 n은 inputbuf에 저장되어 있는 레코드의 수를 나타낸다.
//
void internalsort(char *inputbuf, int n)
{
    qsort(inputbuf,n,RECORD_SIZE, compare);
}

//
//run file들을 대상으로 k-way merge sort를 수행한다.
//이때 반드시 주어진 input buffer와 output buffr를 사용해야 한다.
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
//과제4에서 구현한 함수로서 필요하면 사용한다. (그렇지 않으면 굳이 사용할 필요가 없음)
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
//K-way merge sort에서 run으로부터 해당 chunk를 input buffer로 읽어 들일 때 사용한다.
//이때 읽어들인 chunk는 input buffer에서 자신의 위치에 저장한다.
//주어진 run이 두 번째라면 읽어들인 chunk는 input buffer에서 두 번째 chunk 위치에
//저장한다.
//
void readChunk(FILE *runfp, char *inputbuf, int chunkid)
{
    if(run_datas[chunkid].run_records_num >= num_records_chunk)
        fread(inputbuf+chunk_size*chunkid,chunk_size,1,runfp);
    else
        fread(inputbuf+chunk_size*chunkid,run_datas[chunkid].run_records_num * RECORD_SIZE,1,runfp);
}

//
//k-way merge sort의 중간 결과 레코드가 output buffer에 완전히 차면 새로운 레코드 파일에
//output buffer의 레코드들을 저장한다. 물론 sort의 마지막 부분에서 output buffer가
//차지 않아도 파일에 저장해야 할 경우도 발생할 수 있으며 'n' 파라미터가 필요하다.
//
void writeOutputbuf(FILE *outputfp, const char *outputbuf, int n)
{
    fwrite(outputbuf, RECORD_SIZE * n, 1, outputfp);
}
