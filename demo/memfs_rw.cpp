#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/mman.h>
//#include<iostream>
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
#include <cstddef> 
#include <string>
//using namespace std;

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
     string    f_content; /* stores the file content , in case of file*/
     string    f_name;    /* Contains only file name*/
};

map < string , struct memFS_stat *> stat_map;
map < string , vector<string> > link_map;
map < string , struct memFS_stat*>::iterator sm_it;
map < string , vector<string> >::iterator lm_it;
vector < string >::iterator lvec_it;

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
		
	printf("I am rishi\n");
		
	struct memFS_stat *ms ;
	vector<string> mv;
	
	ms = new memFS_stat();
	ms->st_mode = S_IFDIR | 0755;		
	ms->st_nlink = 2;
	ms->f_name = "/";
	ms->st_uid = mount_uid;	
	ms->st_gid = mount_gid;
	ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
	mv.push_back(".");
	mv.push_back("..");
	mv.push_back("my_file");
	mv.push_back("my_folder");
	stat_map["/"] =  ms; 	
	link_map["/"] = mv;
	mv.clear();
		
	ms = new memFS_stat();
	ms->st_mode = S_IFREG | 0777;
	ms->st_nlink =	1; 
	ms->f_content = "This is Assingment number 6.\n";
	ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
	ms->f_name = "my_file";
	stat_map["/my_file"] = ms;

	
	ms = new memFS_stat();
	ms->st_mode = S_IFREG | 0777;
        ms->st_nlink =  1;
        ms->f_content = "This is other file in folder.\n";
	ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
        ms->f_name = "sec_file";
        stat_map["/my_folder/sec_file"] = ms;
	
		
	ms = new memFS_stat();
	ms->st_mode = S_IFDIR | 0755;
        ms->st_nlink = 3;
	ms->f_name = "my_folder";
        ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
	mv.push_back(".");
	mv.push_back("..");
	mv.push_back("sec_file");
	mv.push_back("sec_folder"); 
	stat_map["/my_folder"] =  ms;
	link_map["/my_folder"] = mv;	
	mv.clear();
	
	ms = new memFS_stat();;
        ms->st_mode = S_IFDIR | 0755;
        ms->st_nlink = 2;
	ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
	ms->f_name = "sec_folder";
	mv.push_back(".");
	mv.push_back("..");
        stat_map["/my_folder/sec_folder"] =  ms;
	link_map["/my_folder/sec_folder"] = mv;
	mv.clear();
			
}

string getString(const char* path){
	string spath(path);
return spath;	
}	

const char* getParentPath(const char* path){
        int len = strlen(path);
        const char * pch;
        pch = strrchr(path, '/');
        long int slen = pch - path;
        if(slen == 0)
                return "/";
        char *str = (char*)malloc(sizeof(char) * len);
        strncpy(str , path, slen);
        str[slen] = '\0';
        const char* mk_str = str;
return mk_str;
}

const char* getFileName(const char* path){
        long int len = strlen(path);
        const char* pch;
        long int i, j = 0;
        pch = strrchr(path , '/');
        long int slen = pch - path;
        char *strp = (char*)malloc(sizeof(char) * len);
        for(i  =  slen + 1 ; i < len ; i++){
                strp[j++] = path[i];
        }
        strp[j] = '\0';
        const char* fk_str = strp;
return fk_str;
}


static int
print_FS(const char *path){
	string spath = getString(path);
	printf("Stat Map data\n");
	struct memFS_stat *ts = stat_map[spath];
	if(ts == NULL)
		return -ENOENT;
	
	cout<<spath<<" "<<ts->f_name<<" "<<" "<<ts->f_content<<" "<<ts->st_mode<<endl;
	
	printf("Child map data\n");
	vector<string> mv = link_map[spath];	
	for(lvec_it = mv.begin() ; lvec_it != mv.end() ; lvec_it++){
              	cout<<"Inside the vector : "<<*lvec_it<<endl;
        }
	printf("End Printing");
return 0;
}

static int
memFS_fuse_getattr(const char *path, struct stat *stbuf)
{
	print_FS(path);
	string spath = getString(path);
	printf("I m calling in getattr    :");
	printf("Path : %s\n", path);
	
	int res = 0;
        memset(stbuf, 0, sizeof(struct stat));       	
	struct memFS_stat *ms = stat_map[spath];

	if(ms == NULL)
		return -ENOENT;
	
	stbuf->st_mode = ms->st_mode;			
	stbuf->st_nlink = ms->st_nlink;
	stbuf->st_uid = ms->st_uid;
	stbuf->st_gid = ms->st_gid;
	stbuf->st_blocks = 0;
	stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
	if(S_ISREG(ms->st_mode))
		stbuf->st_size = strlen(ms->f_content.c_str());
	else if(S_ISDIR(ms->st_mode))
		stbuf->st_size = 4096;

return res;
}

static int
memFS_fuse_readdir(const char *path, void *buf,
		  fuse_fill_dir_t filler, off_t offs, struct fuse_file_info *fi)
{
	printf("readdir\n");
        (void) offs;
        (void) fi;
	string spath = getString(path);
	vector < string > vp = link_map[spath];
	
	if(vp.empty())
		return -ENOENT;
	
	for(lvec_it = vp.begin() ; lvec_it != vp.end() ; lvec_it++){
		filler(buf, (*lvec_it).c_str() , NULL, 0);
	}
	printf("reading complete\n");
return 0;

}

static int
memFS_fuse_open(const char *path, struct fuse_file_info *fi){
	
	//printf("open\n");
	string spath = getString(path);
	
	struct memFS_stat *ms = stat_map[spath];
	
	if(ms == NULL)
		return -ENOENT;
	
	if(!S_ISREG(ms->st_mode))
		return -EACCES;
	
	if(!(ms->st_mode &&  S_IRUSR))
		return -EACCES;
	/*	
        if ((fi->flags & 3) != O_RDONLY)
                return -EACCES;
	*/
	//printf("opened\n");
        return 0;
}



static int
memFS_fuse_utimens(const char *path, const struct timespec tv[2]) {
	
	string spath = getString(path);
	struct memFS_stat *fs = stat_map[spath];
		
	fs->st_atim = tv[0].tv_sec;
	fs->st_mtim = tv[1].tv_sec;
return 0;
}

static int
memFS_fuse_read(const char *path, char *buf, size_t size, off_t offs,
	       struct fuse_file_info *fi)
{
	printf("read\n");
        size_t len;
        (void) fi;
	string spath = getString(path);	
	struct memFS_stat *ms = stat_map[spath];
	const char* f_str = ms->f_content.c_str();
	
	//printf("%s\n", f_str);	
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
memFS_fuse_write(const char *path, const char *buf, size_t size, off_t offs,
		  struct fuse_file_info *fi){

	printf("write");
	string spath = getString(path);
	string parent_path = getString(getParentPath(path));
        string filename = getString(getFileName(path));
	mount_uid = getuid();
        mount_gid = getgid();
        mount_time = time(NULL);

	
	size_t len;
	(void) fi; 
	
	if ( stat_map.find(spath) != stat_map.end() && (stat_map[spath]->st_mode && S_IFMT) == S_IFDIR) {
                return -ENOENT;
        }

	if ( stat_map.find(spath) == stat_map.end() ) {
        	struct memFS_stat *ms = new memFS_stat();

        	ms->st_mode = S_IFREG | 0777;
        	ms->st_nlink = 1;
        	ms->f_name = filename;
        	ms->st_uid = mount_uid;
        	ms->st_gid = mount_gid;
        	ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
        	ms->f_content = (string) buf;
		stat_map[spath] =  ms;
		
		vector<string> pv;
        	pv = link_map[parent_path];
        	pv.push_back(filename);
        	link_map[parent_path] = pv;
	}
	else {
		stat_map[spath]->f_content = stat_map[spath]->f_content.append ( (string) buf );
	}
		stat_map[spath]->st_size = strlen(stat_map[spath]->f_content.c_str());
	
	return stat_map[spath]->f_content.length();
		
	
}

static int
memFS_fuse_mkdir(const char *path , mode_t mod){
	printf("calling mkdir with path  : %s\n", path);

 	string spath = getString(path);
	string parent_path = getString(getParentPath(path));
	string filename = getString(getFileName(path));

	mount_uid = getuid();
        mount_gid = getgid();
        mount_time = time(NULL);	
	
	struct memFS_stat *ms = new memFS_stat();
        vector<string> mv;
	
	ms->st_mode = S_IFDIR | 0755;
	mod = S_IFDIR | 0755;
        ms->st_nlink = 2;
        ms->f_name = filename;
        ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
        mv.push_back(".");
        mv.push_back("..");
        stat_map[spath] =  ms;
        link_map[spath] = mv;
	mv.clear();
	
	vector<string> pv;
	pv = link_map[parent_path];
	pv.push_back(filename);
	link_map[parent_path] = pv;
	
	
	struct memFS_stat *ps = stat_map[parent_path];
	ps->st_nlink += 1;
	stat_map[parent_path] = ps;	
	
return 0;
}
static int
memFS_fuse_mknod(const char *path, mode_t mod, dev_t dev){
	
	printf("calling mknod with path  : %s\n", path);
        string spath = getString(path);
	string parent_path = getString(getParentPath(path));
        string filename = getString(getFileName(path));

	mount_uid = getuid();
        mount_gid = getgid();
        mount_time = time(NULL);

	struct memFS_stat *ts = stat_map[parent_path];
	if(!(ts->st_mode &&  S_IRUSR)){
		return -ENOENT;
	}

	struct memFS_stat *ms = new memFS_stat();
        vector<string> mv;

        ms->st_mode = S_IFREG | 0774;
        ms->st_nlink = 1;
        ms->f_name = filename;
        ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
        stat_map[spath] =  ms;
       	
	mod = S_IFREG | 0774;
 
	vector<string> pv;
        pv = link_map[parent_path];
        pv.push_back(filename);
        link_map[parent_path] = pv;

return 0;
}

static int
memFS_fuse_access(const char *path, int per){
	printf("calling access with path  : %s\n", path);
	string spath = getString(path);
	struct memFS_stat *ms = new memFS_stat();
	ms = stat_map[spath];
	if(ms == NULL)
		return -ENOENT;
	return 0;
}

static int
memFS_fuse_rename(const char *path , const char *npath){
	printf("calling rename with old path  : %s\n and new path is %s", path, npath);
	string spath = getString(path);
	string nspath = getString(npath);
	
	if( stat_map.find(spath) == stat_map.end())
		return -ENOENT;

		


return 0;
}
static int
memFS_fuse_truncate(const char *path , off_t offs){
	printf("calling truncate with path  %s", path );
        string spath = getString(path);
	
	if(stat_map.find(spath) == stat_map.end())
		return -ENOENT;
		
	
	if( (size_t) offs > strlen(stat_map[spath]->f_content.c_str()))
		return -ENOENT;
	
	
	if(!S_ISREG(stat_map[spath]->st_mode))
		return -ENOENT;
	
	stat_map[spath]->f_content = stat_map[spath]->f_content.substr(0, offs);
return 0;
}


static int
memFS_fuse_rmdir(const char* path){
	printf("calling rmdir with path  : %s\n", path);
        string spath = getString(path);
	string parent_path = getString(getParentPath(path));
	string filename = getString(getFileName(path));

	int r = stat_map.erase(spath);
	if(r != 1)
		return -ENOENT;
	int p = link_map.erase(spath);
	if(p != 1)
		return -ENOENT;
	printf("hi\n");
	int count = 0;
	vector<string> pv = link_map[parent_path];
	for(lvec_it = pv.begin() ; lvec_it != pv.end() ; lvec_it++){
         	cout<<*lvec_it<<endl;
		count++;
		if((*lvec_it).compare(filename) == 0){
			printf("p\n");
			break;
		}
        }
	pv.erase(pv.begin() + count - 1);	
	printf("%d\n", count);
	
	link_map[parent_path] = pv;
	
return 0;	
}
/*
static int
memFS_fuse_readlink(const ){}

*/
static int 
memFS_fuse_symlink(const char *opath, const char *npath){
	string ospath = getString(opath);
        string nspath = getString(npath);
	string parent_path = getString(getParentPath(npath));
        string filename = getString(getFileName(npath));
	mount_uid = getuid();
        mount_gid = getgid();
        mount_time = time(NULL);
	
	if((stat_map.find(nspath) != stat_map.end()) ) {
		printf("Exists");
		return -EEXIST;
	}

	struct memFS_stat *ms = new memFS_stat();

        ms->st_mode = S_IFLNK | 0775;
        ms->st_nlink = 1;
        ms->f_name = filename;
        ms->st_uid = mount_uid;
        ms->st_gid = mount_gid;
        ms->st_atim = ms->st_mtim = ms->st_ctim = mount_time;
        ms->f_content = ospath;
        ms->st_size = ospath.length();
	stat_map[nspath] =  ms;

        vector<string> pv;
        pv = link_map[parent_path];
        pv.push_back(filename);
        link_map[parent_path] = pv;

	
return 0;	
}

static int
memFS_fuse_unlink(const char *path){
	string spath = getString(path);
	string parent_path = getString(getParentPath(path));
        string filename = getString(getFileName(path));

	if((stat_map.find(spath) == stat_map.end()) ) {
		return -ENOENT;
	}
	
	struct memFS_stat *fs = stat_map[spath];
	
	if ( S_ISDIR(fs->st_mode) ) {
		return -EISDIR;
	}

	int r = stat_map.erase(spath);
        if(r != 1)
                return -EACCES;

	int count = 0;
        vector<string> pv = link_map[parent_path];
        for(lvec_it = pv.begin() ; lvec_it != pv.end() ; lvec_it++){
                cout<<*lvec_it<<endl;
                count++;
                if((*lvec_it).compare(filename) == 0){
                        printf("p\n");
                        break;
                }
        }
        pv.erase(pv.begin() + count - 1);
        printf("%d\n", count);

        link_map[parent_path] = pv;
		
return 0;
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
	
	memFS_ops.unlink = memFS_fuse_unlink;
	memFS_ops.symlink = memFS_fuse_symlink;
	//memFS_ops.readlink = memFS_fuse_readlink;
	memFS_ops.getattr = memFS_fuse_getattr; 
	memFS_ops.readdir = memFS_fuse_readdir;	
	memFS_ops.open = memFS_fuse_open;
	memFS_ops.read = memFS_fuse_read;
	memFS_ops.mkdir = memFS_fuse_mkdir;
	memFS_ops.rmdir = memFS_fuse_rmdir;
	memFS_ops.mknod = memFS_fuse_mknod;
	memFS_ops.access = memFS_fuse_access;
	memFS_ops.rename = memFS_fuse_rename;
	memFS_ops.truncate = memFS_fuse_truncate;
	memFS_ops.utimens = memFS_fuse_utimens;
	memFS_ops.write =  memFS_fuse_write;
	
	fuse_opt_parse(&args, NULL, NULL, memFS_opt_args);
	
	//printf("%s", argv);
	//if (!f->dev)
	//	errx(1, "missing file system parameter");

	memFS_init(f->dev);
	return (fuse_main(args.argc, args.argv, &memFS_ops, NULL));
}
