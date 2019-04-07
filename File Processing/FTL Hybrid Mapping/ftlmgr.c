//#define PRINT_FOR_DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include "blkmap.h"

BlockMapTbl blkmaptbl;
extern FILE *devicefp;

/****************  prototypes ****************/
void ftl_open();
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);
void initialize_flash_memory();
void print_block(int pbn);
void print_blkmaptbl();
void check_block(int pbn, int ilsn);
int get_empty_block();
int get_empty_page(int pbn);
void copy_block(int pbn, int new_pbn);
int find_page(int pbn, int ilsn);
void dd_write(int ppn, char* pagebuf);
void dd_read(int ppn, char* pagebuf);
void dd_erase(int pbn);

//
// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다
//

void ftl_open()
{
	int i;

	// initialize the address mapping table
	for(i = 0; i < DATABLKS_PER_DEVICE; i++)
	{
		blkmaptbl.entry[i].pbn = -1;
		blkmaptbl.entry[i].first_free_page_offset = 0;
	}

	//
	// 추가적으로 필요한 작업이 있으면 수행할 것
	//

	return;
}

void ftl_write(int lsn, char *sectorbuf)
{
    int i;
    int lbn;
    int pbn = 0;
    int perv_pbn;
    char pagebuf[PAGE_SIZE];

    SpareData *sdata;
    SpareData *tmp_sdata;

    sdata = (SpareData *) malloc(SPARE_SIZE);

    lbn = lsn/PAGES_PER_BLOCK;

    memcpy(pagebuf, sectorbuf,SECTOR_SIZE);

    sdata->lsn = lsn;
    sdata->invalid = -1;

    memcpy(pagebuf+SECTOR_SIZE,(char *)sdata, SPARE_SIZE);

    if(blkmaptbl.entry[lbn].first_free_page_offset < PAGES_PER_BLOCK)//has remaining block
    {
        if(blkmaptbl.entry[lbn].pbn == -1)
        {
            pbn = blkmaptbl.entry[lbn].pbn = get_empty_block();
        }
        else
        {
            pbn = blkmaptbl.entry[lbn].pbn;
            check_block(pbn, lsn);
        }
    }

    else
    {
        perv_pbn = blkmaptbl.entry[lbn].pbn;
        pbn = blkmaptbl.entry[lbn].pbn = get_empty_block();
        check_block(perv_pbn, lsn);
        copy_block(perv_pbn,blkmaptbl.entry[lbn].pbn);
        dd_erase(perv_pbn);
    }

    blkmaptbl.entry[lbn].first_free_page_offset = get_empty_page(pbn);

    dd_write(pbn*PAGES_PER_BLOCK + blkmaptbl.entry[lbn].first_free_page_offset,pagebuf);

    blkmaptbl.entry[lbn].first_free_page_offset = get_empty_page(pbn);

    free(sdata);

#ifdef PRINT_FOR_DEBUG				// 필요 시 현재의 block mapping table을 출력해 볼 수 있음
	print_blkmaptbl();
#endif

	return;
}

void ftl_read(int lsn, char *sectorbuf)
{
    char pagebuf[PAGE_SIZE];
    int lbn;
    int pbn;
    int ppn;

    lbn = lsn/PAGES_PER_BLOCK;
    pbn = blkmaptbl.entry[lbn].pbn;

    ppn = find_page(pbn,lsn);

    if(ppn == -1)
        fprintf(stderr, "Can't find data %d\n", lsn);
    else
    {
        dd_read(ppn, pagebuf);
        memcpy(sectorbuf,pagebuf,SECTOR_SIZE);
    }


#ifdef PRINT_FOR_DEBUG				// 필요 시 현재의 block mapping table을 출력해 볼 수 있음
    print_blkmaptbl();
#endif

	return;
}

//
// initialize flash memory where each byte are set to 'OxFF'
//
void initialize_flash_memory()
{
	char *blockbuf;
	int i;

	blockbuf = (char *)malloc(BLOCK_SIZE);
	memset(blockbuf, 0xFF, BLOCK_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
		fwrite(blockbuf, BLOCK_SIZE, 1, devicefp);
	}

	free(blockbuf);

	return;
}

void print_block(int pbn)
{
	char *pagebuf;
	SpareData *sdata;
	int i;

	pagebuf = (char *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	printf("Physical Block Number: %d\n", pbn);

	for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
	{
        dd_read(i,pagebuf);
        memcpy(sdata, pagebuf+SECTOR_SIZE, SPARE_SIZE);
        printf("\t %5d-[%5d][%5d]\n", i, sdata->lsn, sdata->invalid);
	}

	free(pagebuf);
	free(sdata);

	return;
}

void check_block(int pbn, int ilsn)
{
	char *pagebuf;
	SpareData *sdata;
    int i;

	pagebuf = (char *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
	{
        dd_read(i,pagebuf);
        memcpy(sdata, pagebuf+SECTOR_SIZE,SPARE_SIZE);
        if(sdata->invalid == 1)
            continue;
        if(sdata->lsn == ilsn)
        {
            sdata->invalid=1;
            memcpy(pagebuf+SECTOR_SIZE, sdata, SPARE_SIZE);
            dd_write(i,pagebuf);
            free(pagebuf);
            free(sdata);
            return;
        }
	}

	free(pagebuf);
	free(sdata);

	return;
}
int find_page(int pbn, int ilsn)
{
	char *pagebuf;
	SpareData *sdata;
    int i;

	pagebuf = (char *)malloc(PAGE_SIZE);
	sdata = (SpareData *)malloc(SPARE_SIZE);

	for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
	{
        dd_read(i,pagebuf);
        memcpy(sdata, pagebuf+SECTOR_SIZE,SPARE_SIZE);
        if(sdata->invalid == 1)
            continue;
        if(sdata->lsn == ilsn)
        {
            free(pagebuf);
            free(sdata);
            return i;
        }
	}

	free(pagebuf);
	free(sdata);

	return -1;
}
int get_empty_page(int pbn)
{
	char *pagebuf;
    int i;
    char tmpbuf[PAGE_SIZE];
    memset(tmpbuf, 0xFF, PAGE_SIZE);
	pagebuf = (char *)malloc(PAGE_SIZE);
    for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
    {
        dd_read(i, pagebuf);
        if(memcmp(pagebuf,tmpbuf,SECTOR_SIZE) == 0)
        {
            free(pagebuf);
            return i%PAGES_PER_BLOCK;
        }
    }
    free(pagebuf);
    return PAGES_PER_BLOCK;
}

void copy_block(int pbn, int new_pbn)
{
    char *sectorbuf;
    char *pagebuf;
    SpareData *sdata;
    int i;

    pagebuf = (char *)malloc(PAGE_SIZE);
    sectorbuf = (char *)malloc(SECTOR_SIZE);
    sdata = (SpareData *)malloc(SPARE_SIZE);

	for(i = pbn*PAGES_PER_BLOCK; i < (pbn+1)*PAGES_PER_BLOCK; i++)
	{
        dd_read(i,pagebuf);
        memcpy(sdata, pagebuf+SECTOR_SIZE,SPARE_SIZE);
        if(sdata->invalid == 1)
            continue;
        else
        {
            memcpy(pagebuf+SECTOR_SIZE, sdata, SPARE_SIZE);
            dd_write(new_pbn*PAGES_PER_BLOCK + i%PAGES_PER_BLOCK,pagebuf);
        }
	}

	free(pagebuf);
	free(sectorbuf);
	free(sdata);

	return;
}

int get_empty_block()
{
	char *blockbuf;
    char *tmpbuf;
	int i;

	blockbuf = (char *)malloc(BLOCK_SIZE);
    tmpbuf = (char *)malloc(BLOCK_SIZE);
	memset(tmpbuf, 0xFF, BLOCK_SIZE);

	for(i = 0; i < BLOCKS_PER_DEVICE; i++)
	{
        fseek(devicefp,i * BLOCK_SIZE,SEEK_SET);
		fread(blockbuf, BLOCK_SIZE, 1, devicefp);
		if(memcmp(blockbuf,tmpbuf, BLOCK_SIZE) == 0)
        {
            free(blockbuf);
            free(tmpbuf);
            return i;
        }
	}

    free(blockbuf);
    free(tmpbuf);
    return -1;
}

void print_blkmaptbl()
{
    int i;

    printf("Block Mapping Table: \n");
    for(i = 0; i < DATABLKS_PER_DEVICE; i++)
    {
		if(blkmaptbl.entry[i].pbn >= 0)
		{
			printf("[%d %d %d]\n", i, blkmaptbl.entry[i].pbn, blkmaptbl.entry[i].first_free_page_offset);
		}
	}
}

