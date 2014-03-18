#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/mman.h>
#include<map>
#include<vector>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace std;

struct memFS_super {
	uint8_t		res1[3];
	char		oemname[8];
	uint16_t	bytes_per_sector;
	uint8_t		sectors_per_cluster;
	uint16_t	reserved_sectors;
	uint8_t		fat_count;
	uint16_t	root_max_entries;
	uint16_t	total_sectors_small;
	uint8_t		media_info;
	uint16_t	sectors_per_fat_small;
	uint16_t	sectors_per_track;
	uint16_t	head_count;
	uint32_t	fs_offset;
	uint32_t	total_sectors;
	uint32_t	sectors_per_fat;
	uint16_t	fat_flags;
	uint16_t	version;
	uint32_t	root_cluster;
	uint16_t	fsinfo_sector;
	uint16_t	backup_sector;
	uint8_t		res2[12];
	uint8_t		drive_number;
	uint8_t		res3;
	uint8_t		ext_sig;
	uint32_t	serial;
	char		label[11];
	char		type[8];
	char		res4[420];
	uint16_t	sig;
} __attribute__ ((__packed__));

struct memFS_direntry {
	union {
		struct {
			char		name[8];
			char		ext[3];
		};
		char			nameext[11];
	};
	uint8_t		attr;
	uint8_t		res;
	uint8_t		ctime_ms;
	uint16_t	ctime_time;
	uint16_t	ctime_date;
	uint16_t	atime_date;
	uint16_t	cluster_hi;
	uint16_t	mtime_time;
	uint16_t	mtime_date;
	uint16_t	cluster_lo;
	uint32_t	size;
} __attribute__ ((__packed__));

#define memFS_ATTR_DIR	0x10
#define memFS_ATTR_LFN	0xf
#define memFS_ATTR_INVAL	(0x80|0x40|0x08)

struct memFS_direntry_lfn {
	uint8_t		seq;
	uint16_t	name1[5];
	uint8_t		attr;
	uint8_t		res1;
	uint8_t		csum;
	uint16_t	name2[6];
	uint16_t	res2;
	uint16_t	name3[2];
} __attribute__ ((__packed__));

#define memFS_LFN_SEQ_START	0x40
#define memFS_LFN_SEQ_DELETED	0x80
#define memFS_LFN_SEQ_MASK	0x3f

struct memFS {
	const char	*dev;
	int		fs;
};

struct memFS_search_data {
	const char	*name;
	int		found;
	struct stat	*st;
};

struct memFS memFS_info, *f = &memFS_info;


iconv_t iconv_utf16;

uid_t mount_uid;
gid_t mount_gid;
time_t mount_time;
size_t pagesize;

struct memFS_stat {
     dev_t     st_dev;     /* ID of device containing file */
     ino_t     st_ino;     /* inode number */
     mode_t    st_mode;    /* protection */
     nlink_t   st_nlink;   /* number of hard links */
     uid_t     st_uid;     /* user ID of owner */
     gid_t     st_gid;     /* group ID of owner */
     dev_t     st_rdev;    /* device ID (if special file) */
     off_t     st_size;    /* total size, in bytes */
     blksize_t st_blksize; /* blocksize for file system I/O */
     blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
     time_t    st_atim;   /* time of last access */
     time_t    st_mtim;   /* time of last modification */
     time_t    st_ctim;   /* time of last status change */
     const char*     f_content; /* stores the file content , in case of file*/
     const char*     f_name;    /* Contains only file name*/
};

map < const char* , struct memFS_stat> stat_map;
map < const char* , vector<const char *> > link_map;
map < const char* , struct memFS_stat>::iterator sm_it;
map < const char* , vector<const char *> >::iterator lm_it;
vector < const char * >::iterator lvec_it;
/*  
static const char *file_str = "This is Assingment number 6.\n";
static const char *file_path = "/my_file";
static const char *fol_path = "/my_folder";
static const char *fol_file_str = "This is other file in folder.\n";
static const char *fol_file_path = "/my_folder/sec_file";
static const char *fol_fol_path = "/my_folder/sec_folder";
*/
static struct fuse_operations memFS_ops;


static void
memFS_init(const char *dev)
{
	iconv_utf16 = iconv_open("utf-8", "utf-16");
	mount_uid = getuid();
	mount_gid = getgid();
	mount_time = time(NULL);

	f->fs = open(dev, O_RDONLY);
	if (f->fs < 0)
		err(1, "open(%s)", dev);
		
	printf("I am rishi");
		
	struct memFS_stat ms;
	vector<const char*> mv;
	
	ms.st_mode = S_IFDIR | 0755;		
	ms.st_nlink = 3;
	ms.f_name = "/";
	ms.st_uid = mount_uid;	
	ms.st_gid = mount_gid;
	ms.st_atim = ms.st_mtim = ms.st_ctim = mount_time;
	mv.push_back(".");
	mv.push_back("..");
	mv.push_back("my_file");
	mv.push_back("my_folder");
	stat_map["/"] =  ms; 	
	link_map["/"] = mv;
	mv.clear();	
		
	//ms = NULL;
	ms.st_mode = S_IFREG | 0444;
	ms.st_nlink =	1; 
	ms.f_content = "This is Assingment number 6.\n";
	ms.st_uid = mount_uid;
        ms.st_gid = mount_gid;
        ms.st_atim = ms.st_mtim = ms.st_ctim = mount_time;
	ms.f_name = "my_file";
	stat_map["/my_file"] = ms;

	//ms = NULL;
	ms.st_mode = S_IFREG | 0444;
        ms.st_nlink =  1;
        ms.f_content = "This is other file in folder.\n";
	ms.st_uid = mount_uid;
        ms.st_gid = mount_gid;
        ms.st_atim = ms.st_mtim = ms.st_ctim = mount_time;
        //ms->st_size = strlen(ms->f_content);
        ms.f_name = "sec_file";
        stat_map["/my_folder/sec_file"] = ms;
	ms.f_content = NULL;
	
	//ms = NULL;
	ms.st_mode = S_IFDIR | 0755;
        ms.st_nlink = 3;
	ms.f_name = "my_folder";
        ms.st_uid = mount_uid;
        ms.st_gid = mount_gid;
        ms.st_atim = ms.st_mtim = ms.st_ctim = mount_time;
	mv.push_back(".");
	mv.push_back("..");
	mv.push_back("sec_file");
	mv.push_back("sec_folder"); 
	stat_map["/my_folder"] =  ms;
	link_map["/my_folder"] = mv;	
	mv.clear();

	//ms = NULL;
        ms.st_mode = S_IFDIR | 0755;
        ms.st_nlink = 2;
	ms.st_uid = mount_uid;
        ms.st_gid = mount_gid;
        ms.st_atim = ms.st_mtim = ms.st_ctim = mount_time;
	ms.f_name = "sec_folder";
	mv.push_back(".");
	mv.push_back("..");
        stat_map["/my_folder/sec_folder"] =  ms;
	link_map["/my_folder/sec_folder"] = mv;
	mv.clear();
}	

/* XXX add your	 code here */
/*
static int
memFS_readdir(// XXX add your code here//, fuse_fill_dir_t filler, void *fillerdata)
{
	struct stat st;
	void *buf = NULL;
	struct memFS_direntry *e;
	char *name;

	memset(&st, 0, sizeof(st));
	st.st_uid = mount_uid;
	st.st_gid = mount_gid;
	st.st_nlink = 1;

	 //XXX add your code here 
}

static int
memFS_search_entry(void *data, const char *name, const struct stat *st, off_t offs)
{
	struct memFS_search_data *sd = (struct memFS_search_data *)data;

	if (strcmp(sd->name, name) != 0)
		return (0);

	sd->found = 1;
	*sd->st = *st;

	return (1);
}

static int
memFS_resolve(const char *path, struct stat *st)
{
	struct memFS_search_data sd;

	// XXX add your code here 
}
*/
static int
memFS_fuse_getattr(const char *path, struct stat *stbuf)
{
	printf("I m calling in getattr    :");
	printf("Path : %s\n", path);
	
	int res = 0;

        memset(stbuf, 0, sizeof(struct stat));       
		
	for(sm_it = stat_map.begin() ; sm_it != stat_map.end() ; sm_it++){
		const char *key_path = sm_it->first;
		struct memFS_stat ms = sm_it->second;
		if(strcmp(path ,key_path ) == 0){
			stbuf->st_mode = ms.st_mode;
			stbuf->st_nlink = ms.st_nlink;
			stbuf->st_uid = ms.st_uid;
			stbuf->st_gid = ms.st_gid;
			stbuf->st_blocks = 0;
			stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
			if(S_ISREG(ms.st_mode))
				stbuf->st_size = strlen(ms.f_content);
		}	
	}	 
	
        return res;
}

static int
memFS_fuse_readdir(const char *path, void *buf,
		  fuse_fill_dir_t filler, off_t offs, struct fuse_file_info *fi)
{
	printf("readdir\n");
        (void) offs;
        (void) fi;

	for(lm_it = link_map.begin() ; lm_it != link_map.end(); lm_it++){
		const char *key_path = lm_it->first;
		vector< const char * > vp = lm_it->second;	
		if(strcmp(path, key_path) == 0){
			for(lvec_it = vp.begin() ; lvec_it != vp.end() ; lvec_it++){
				filler(buf, *lvec_it , NULL, 0);
			}
		}
	}
return 0;

}

static int
memFS_fuse_open(const char *path, struct fuse_file_info *fi){
	
	printf("open\n");
	int flag = 0;
	for(sm_it = stat_map.begin() ; sm_it != stat_map.end() ; sm_it++){
               const char *key_path = sm_it->first;
               struct memFS_stat ms = sm_it->second;
		//printf("Mode:                     %lo (octal)\n",
                 //  (unsigned long) ms.st_mode);

               if(strcmp(path ,key_path) == 0 && S_ISREG(ms.st_mode)){
        		flag = 1;
			if(!(ms.st_mode && S_IRUSR)){
				flag = 0;
			}	         
		}
      	}
	if(flag == 0)
		return -EACCES;	

        if ((fi->flags & 3) != O_RDONLY)
                return -EACCES;
	
	
	printf("opened\n");
        return 0;
}


static int
memFS_fuse_read(const char *path, char *buf, size_t size, off_t offs,
	       struct fuse_file_info *fi)
{
	printf("read\n");
        size_t len;
        (void) fi;
	int flag = 0;

	const char * f_str;
		
	for(sm_it = stat_map.begin() ; sm_it != stat_map.end() ; sm_it++){
                const char *key_path = sm_it->first;
                struct memFS_stat ms = sm_it->second; 
                if(strcmp(path ,key_path) == 0 ){
                         flag = 1;
          		 f_str  = ms.f_content;          	 
                 }
         }
	 if (flag == 0)
		return -ENOENT;
	
	len = strlen(f_str);
	
	if (offs < len) {
                if (offs + size > len)
                        size = len - offs;
                memcpy(buf, f_str + offs, size);
        } else{
                size = 0;
	}
	 
	return size;
}

static int
memFS_opt_args(void *data, const char *arg, int key, struct fuse_args *oargs)
{
	if (key == FUSE_OPT_KEY_NONOPT && f->dev == NULL) {
		f->dev = strdup(arg);
		return (1);
	}
	return (1);
}




int
main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	memFS_ops.getattr = memFS_fuse_getattr; 
	memFS_ops.readdir = memFS_fuse_readdir;
	memFS_ops.open = memFS_fuse_open;
	memFS_ops.read = memFS_fuse_read;
	
	fuse_opt_parse(&args, NULL, NULL, memFS_opt_args);
	
	//printf("%s", argv);
	//if (!f->dev)
	//	errx(1, "missing file system parameter");

	memFS_init(f->dev);
	return (fuse_main(args.argc, args.argv, &memFS_ops, NULL));
}
