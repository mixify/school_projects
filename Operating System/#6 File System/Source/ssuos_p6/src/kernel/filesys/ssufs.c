#include <filesys/ssufs.h>
#include <filesys/vnode.h>
#include <device/block.h>
#include <device/ata.h>
#include <proc/proc.h>
#include <ssulib.h>
#include <string.h>
#include <bitmap.h>

#define MIN(a, b)		(a<b?a:b)

static unsigned char bitmapblock[SSU_BLOCK_SIZE];
static struct ssufs_superblock ssufs_sb;
extern struct blk_dev ata1_blk_dev;
extern struct process *cur_process;

char tmpblock[SSU_BLOCK_SIZE];

struct vnode *init_ssufs(char *volname, uint32_t lba, struct vnode *mnt_root){
	int result;
	int i;
	char superblock[SSU_BLOCK_SIZE];

	ssufs_sb.blkdev = &ata1_blk_dev;
	result = ssufs_readblock(&ssufs_sb, lba, superblock);//seems like reading 1 block

	memcpy(&ssufs_sb, superblock, sizeof(ssufs_sb));//2nd block
	ssufs_sb.blkdev = &ata1_blk_dev;

	if(ssufs_sb.sb_magic != SSU_SB_MAGIC){//recognize ssuos file system
		ssufs_sb.sb_nblocks = (ssufs_sb.blkdev->blk_count / (SSU_BLOCK_SIZE / ssufs_sb.blkdev->blk_size));//allocating block count??
		ssufs_sb.lba = lba;
		// sb.sb->ssufs_info->sb_nblocks;
	}else{
        //no file system except ssu file system
    }

	//load or init bitmap block
	ssufs_load_databitmapblock(&ssufs_sb);//sb.blkmap
	ssufs_load_inodebitmapblock(&ssufs_sb);//sb.inodemap
	ssufs_sync(&ssufs_sb);

	//l0ad or init inode table
	ssufs_load_inodetable(&ssufs_sb);//read disk files from bitmap?? and allocate to inode

	return make_vnode_tree(&ssufs_sb, mnt_root);//mnt_root => root dir has files, cur_process->cwd == cur_vnode
    //vnode has child and it has all files of directory
}

int ssufs_load_inodetable(struct ssufs_superblock *sb)//load from hdd~~
{
	int result = 0;
	int i;
	struct ssufs_inode *root_inode;//check if inode has been initialized??
    //why root i node num is 2?
	struct dirent dirent;

	for(i=0; i<NUM_INODE_BLOCK; i++)//load all inodes block
		ssufs_readblock(sb, SSU_INODE_BLOCK(sb->lba) + i, ((char*)ssufs_inode_table) + (i * SSU_BLOCK_SIZE));

	//no root directory => restart should have root directroy
	if(!bitmap_test(sb->inodemap, INODE_ROOT)){//inode_root has not been initialized
		memset(ssufs_inode_table, 0x00, sizeof(struct ssufs_inode) * NUM_INODE);

		//Unvalid, Reserved
		bitmap_set(sb->inodemap, 0, true);//boot block?
		bitmap_set(sb->inodemap, 1, true);// reserved block

		bitmap_set(sb->inodemap, INODE_ROOT, true);

		root_inode = &ssufs_inode_table[INODE_ROOT];
		root_inode->i_no = INODE_ROOT;
		root_inode->i_size = 0;
		root_inode->i_type = SSU_DIR_TYPE;
		root_inode->i_refcount = 1;
		root_inode->ssufs_sb = sb;

		//set root dirent which is '/'
		dirent.d_ino = INODE_ROOT;
		dirent.d_type = SSU_DIR_TYPE;
        //original code
		/* memcpy(dirent.d_name, ".", sizeof("."));//if it is recoginized as file shoudn't it made as a file?? */
		/* memcpy(tmpblock, (char *)&dirent, sizeof(struct dirent)); */
		/* memcpy(dirent.d_name, "..", sizeof("..")); */
		/* memcpy(tmpblock+sizeof(struct dirent), (char *)&dirent, sizeof(struct dirent)); */
		/* ssufs_inode_write(root_inode, 0, tmpblock, sizeof(struct dirent)*2);//write to tmpblock */

		memcpy(dirent.d_name, ".", sizeof("."));//if it is recoginized as file shoudn't it made as a file??
		ssufs_inode_write(root_inode, 0,(char *)&dirent , sizeof(struct dirent));
		memcpy(dirent.d_name, "..", sizeof(".."));
		ssufs_inode_write(root_inode, root_inode->i_size, (char *)&dirent, sizeof(struct dirent));

		ssufs_sync_bitmapblock(sb);
		ssufs_sync_inodetable(sb);
		printk("initialize inodetable : %d\n", NUM_INODE);
	}else{
		printk("load inodetable : %d\n", NUM_INODE);
	}

	return result;
}

int ssufs_load_databitmapblock(struct ssufs_superblock *sb)
{
	int result;
	int i;

	//load bitmap block from HDD
	result = ssufs_readblock(sb, SSU_BITMAP_BLOCK(sb->lba), bitmapblock);

	//load block bitmap
	sb->blkmap = (struct bitmap*)bitmapblock;
	if(sb->blkmap->bit_cnt != sb->sb_nblocks){//no blocks (should have first block???.. )
		sb->blkmap = bitmap_create_in_buf(ssufs_sb.sb_nblocks, bitmapblock, SSU_BLOCK_SIZE/2);
		for(i=0; i<SSU_BITMAP_BLOCK(sb->lba); i++)
		{
			bitmap_set(sb->blkmap, i, true);
		}

		printk("initialize block bitmap : %d bit\n", sb->blkmap->bit_cnt);
	}else{
		printk("load block bitmap : %d bit\n", sb->blkmap->bit_cnt);
	}

	return result;
}

int ssufs_load_inodebitmapblock(struct ssufs_superblock *sb)
{
	//load inode bitmap
	sb->inodemap = (struct bitmap*)(bitmapblock + SSU_BLOCK_SIZE/2);
	if(sb->inodemap->bit_cnt != NUM_INODE)//no inode
	{
		sb->inodemap = bitmap_create_in_buf(NUM_INODE, bitmapblock + SSU_BLOCK_SIZE/2, SSU_BLOCK_SIZE/2);
		printk("initialize inode bitmap : %d bit\n", sb->inodemap->bit_cnt);
	}
	else
		printk("load inode bitmap : %d bit\n", sb->inodemap->bit_cnt);
}

int ssufs_readblock(struct ssufs_superblock *sb, uint32_t blknum, char *buf)
{
	int result = 0;
	int startsec = blknum * SECTORCOUNT(sb->blkdev);
	int i;

	for(i=0; i < SECTORCOUNT(sb->blkdev); i++){
		DEVOP_READ(sb->blkdev, startsec + i, buf + (i * sb->blkdev->blk_size));
	}

	return result;
}

int ssufs_writeblock(struct ssufs_superblock *sb, uint32_t blknum, char *buf)
{
	int result = 0;
	int startsec = blknum * SECTORCOUNT(sb->blkdev);
	int i;

	for(i=0; i<SECTORCOUNT(sb->blkdev); i++)
		DEVOP_WRITE(sb->blkdev, startsec + i, buf + (i * sb->blkdev->blk_size));

	return result;
}

int ssufs_sync_superblock(struct ssufs_superblock *sb){
	int result = 0;

	ssufs_writeblock(sb, sb->lba, (char *)sb);

	return result;
}

int ssufs_sync_bitmapblock(struct ssufs_superblock *sb){
	int result = 0;

	ssufs_writeblock(sb, SSU_BITMAP_BLOCK(sb->lba), sb->blkmap);

	return result;
}

int ssufs_sync_inodetable(struct ssufs_superblock *sb){
	int result = 0;
	int i;

	for(i = 0 ; i < NUM_INODE_BLOCK; i++)
		ssufs_writeblock(sb, SSU_INODE_BLOCK(sb->lba)+i, ((char *)ssufs_inode_table+(i*SSU_BLOCK_SIZE)));

	return result;
}

int ssufs_sync(struct ssufs_superblock *sb){
	int result = 0;

	ssufs_sync_superblock(sb);

	ssufs_sync_bitmapblock(sb);
}

/********************************************************* inode start ************************************************************/

int ssufs_inode_write(struct ssufs_inode *inode, uint32_t offset, char *buf, uint32_t len){
    //write buf data to inode
	int result = 0;
	uint32_t blkoff;
	uint32_t resoff;
	int block_index;
	int i;

	if(offset > inode->i_size || len <= 0 || buf == NULL)//offset out of limit or invalid
		return -1;

	for(i = offset; i < offset+len; i = ((blkoff+1)*SSU_BLOCK_SIZE))//start from offset
	{
		blkoff = i / SSU_BLOCK_SIZE;//block offset
		resoff = i % SSU_BLOCK_SIZE;//rest offset??

		memset(tmpblock, 0, SSU_BLOCK_SIZE);
		if(blkoff < NUM_DIRECT){//direct has data and data block
			if(inode->i_direct[blkoff] == 0){//no data in that block => should allocate
				block_index = bitmap_alloc(inode->ssufs_sb->blkmap);//alloc at superblock bitmap too
                //allocate datablock from start
				block_index += SSU_DATA_BLOCK(inode->ssufs_sb->lba);

				if(block_index == -1){
					return -1;
				}

				inode->i_direct[blkoff] = block_index;//i_node's data block allocated
			}else{
				ssufs_readblock(inode->ssufs_sb, inode->i_direct[blkoff], tmpblock);

				struct dirent dirent;
				memcpy(&dirent, tmpblock, sizeof(dirent));
			}

			memcpy(tmpblock+resoff, buf, len);
			ssufs_writeblock(inode->ssufs_sb, inode->i_direct[blkoff], tmpblock);
		}else{//indirect,. not used

		}
	}

	inode->i_size = offset+len;//file size committed

	ssufs_sync_bitmapblock(inode->ssufs_sb);
	ssufs_sync_inodetable(inode->ssufs_sb);

	return result;
}

int ssufs_inode_read(struct ssufs_inode *inode, uint32_t offset, char *buf, uint32_t len)
{
    //read inode and hold data to buf
	int result = 0;
	uint32_t blkoff;
	uint32_t resoff;
	int i;

	if(offset > inode->i_size)
		return -1;

	for(i=0; i < len; i+=(SSU_BLOCK_SIZE - resoff)){
		blkoff = offset / SSU_BLOCK_SIZE;
		resoff = offset % SSU_BLOCK_SIZE;

		if(blkoff < NUM_DIRECT){//direct
			ssufs_readblock(inode->ssufs_sb, inode->i_direct[blkoff], tmpblock);
			memcpy(buf, tmpblock+resoff, MIN(SSU_BLOCK_SIZE - resoff, len));
		}else{//indirect, #not used

		}
	}

	return result;
}

struct ssufs_inode *inode_alloc(uint32_t type){//should use mkdir
	struct ssufs_inode *cwd_inode = (struct ssufs_inode*)(cur_process->cwd->info);//cur process's inode
	struct ssufs_superblock *sb = cwd_inode->ssufs_sb;
	struct ssufs_inode *inode;
	int i;

	for(i = INODE_ROOT; i < NUM_INODE; i++){
		if(!bitmap_test(sb->inodemap, i)){
			inode = &ssufs_inode_table[i];
			inode->i_no = i;
			inode->i_size = 0;
			inode->i_type = type;
			inode->i_refcount = 1;
			inode->ssufs_sb = sb;

			bitmap_set(sb->inodemap, i, true);

			ssufs_sync_inodetable(sb);
			ssufs_sync_bitmapblock(sb);

			return inode;
		}
	}

	return NULL;
}

/********************************************************* inode end ************************************************************/

struct vnode *make_vnode_tree(struct ssufs_superblock *sb, struct vnode *mnt_root)
{
    int i, j;
    struct ssufs_inode *root_inode;//check if inode has been initialized??
    struct dirent dirent;


    struct list vnode_stack;
    list_init(&vnode_stack);

    root_inode = &ssufs_inode_table[INODE_ROOT];
    set_vnode(mnt_root, mnt_root, root_inode);

    /* for(i = INODE_ROOT; i < NUM_INODE; i++){ */
    /*     if(bitmap_test(sb->inodemap, i)){ */
    struct ssufs_inode *inode = root_inode;
    do{

        struct vnode* parent_vnode;

        /* if(inode->i_no == 0) */
        /*     continue; */

        if(list_empty(&vnode_stack))
            parent_vnode = mnt_root;
        else
        {
            struct list_elem *e;
            e = list_pop_front(&vnode_stack);
            parent_vnode = list_entry(e, struct vnode, elem_stack);
            inode = parent_vnode->info;
        }

        size_t ndir = inode->i_size / sizeof(struct dirent);
        for (j = 2; j < ndir; ++j) {
            struct vnode* new_vnode;
            struct ssufs_inode* new_inode;
            ssufs_inode_read(inode, j * sizeof(struct dirent), (char *)&dirent, sizeof(struct dirent));

            if(dirent.d_ino==inode->i_no)
                continue;//access only childs

            new_vnode = vnode_alloc();
            list_push_back(&vnode_stack, &new_vnode->elem_stack);
            new_inode = &ssufs_inode_table[dirent.d_ino];

            /* printk("parent vnode %s put %s as a child\n", parent_vnode->v_name, dirent.d_name); */
            set_vnode(new_vnode, parent_vnode, new_inode);
            list_push_back(&parent_vnode->childlist, &new_vnode->elem);
        }

    }while(!list_empty(&vnode_stack));


    return mnt_root;
}

static int num_direntry(struct ssufs_inode *inode)
{
    if(inode->i_size % sizeof(struct dirent) != 0 || inode->i_type != SSU_DIR_TYPE)//it's not directory
        return -1;

    return inode->i_size / sizeof(struct dirent);
}

void set_vnode(struct vnode *vnode, struct vnode *parent_vnode, struct ssufs_inode *inode)
{
    struct dirent dirent;
    struct ssufs_inode *parent_inode;
    int i, ndir;

    get_curde(inode, &dirent);
    memcpy(vnode->v_name, dirent.d_name, LEN_VNODE_NAME);//copy file(dir) name

    vnode->v_parent = parent_vnode;
    vnode->type = inode->i_type;

    vnode->v_op.mkdir = ssufs_mkdir;
    vnode->v_op.ls = NULL;

    vnode->info = (void *)inode;//each vnode has original inode's address
}

int get_curde(struct ssufs_inode *cwd, struct dirent * de)
{
    struct ssufs_inode *pwd;
    int i, ndir;

    //get parent dir
    ssufs_inode_read(cwd, sizeof(struct dirent), (char*)de, sizeof(struct dirent));
    pwd = &ssufs_inode_table[de->d_ino];
    ndir = num_direntry(pwd);//num of dir

    for(i=0; i<ndir; i++)
    {
        ssufs_inode_read(pwd, i*sizeof(struct dirent), (char*)de, sizeof(struct dirent));
        if(de->d_ino == cwd->i_no)
            return 0;
    }
    return -1;
}

//**************************************************     vnode operation      *****************************************************/
int ssufs_mkdir(char *dirname){//should handle parent precisely

    struct vnode* cur_vnode = cur_process->cwd;
    struct ssufs_inode* cur_inode = cur_process->cwd->info;
    struct dirent dirent;
    struct ssufs_inode* new_inode = inode_alloc(SSU_DIR_TYPE);
    struct vnode* new_vnode = vnode_alloc();

    dirent.d_ino = 0;
    dirent.d_type = SSU_DIR_TYPE;

    memcpy(dirent.d_name, ".", sizeof("."));
    ssufs_inode_write(new_inode, 0,(char *)&dirent , sizeof(struct dirent));

    dirent.d_ino = cur_inode->i_no;
    dirent.d_type = SSU_DIR_TYPE;
    memcpy(dirent.d_name, "..", sizeof(".."));
    ssufs_inode_write(new_inode, new_inode->i_size, (char *)&dirent, sizeof(struct dirent));


    dirent.d_ino = new_inode->i_no;
    dirent.d_type = SSU_DIR_TYPE;
    memcpy(dirent.d_name, dirname, FILENAME_LEN);

    ssufs_inode_write(new_inode, new_inode->i_size, (char *)&dirent, sizeof(struct dirent));
    ssufs_inode_write(cur_inode, cur_inode->i_size, (char *)&dirent, sizeof(struct dirent));

    set_vnode(new_vnode, cur_vnode, new_inode);
    list_push_back(&cur_vnode->childlist, &new_vnode->elem);

    return 0;
}
