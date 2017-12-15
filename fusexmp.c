/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall fusexmp.c `pkg-config fuse --cflags --libs` -o fusexmp
*/

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#ifndef HAVE_UTIMENSAT
#define HAVE_UTIMENSAT
#endif

#ifndef HAVE_POSIX_FALLOCATE
#define HAVE_POSIX_FALLOCATE
#endif 

#ifndef HAVE_SETXATTR
#define HAVE_SETXATTR
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <linux/limits.h>


#define NUMOFDISK 2
#define STRIPESIZE 512

static struct {
  char driveA[STRIPESIZE];
  char driveB[STRIPESIZE];
} global_context;

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	char fullpaths[NUMOFDISK][PATH_MAX];
	int res;

	off_t tmpsize = 0;

	printf("In getattr!!!!!!!!!!!!!!!!!!!!!!!1\n");
	printf("PATH: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpaths 1: %s, fullpaths 2: %s\n", fullpaths[0], fullpaths[1]);
	
	for(int i = 0; i < NUMOFDISK; i++){
		res = lstat(fullpaths[i], stbuf);
		if (res == -1){
			printf("Error in getting attributes in %s!!!\n\n", fullpaths[0]);
			return -errno;
		}
		tmpsize += stbuf->st_size;
		stbuf->st_size = tmpsize;
	}
	printf("Out getattr!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_access(const char *path, int mask)
{
	char fullpaths[NUMOFDISK][PATH_MAX];
	int res;

	printf("num of disk: %d\n", sizeof(global_context));
	printf("In access!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11\n");
	printf("PATH: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpaths 1: %s 2: %s\n", fullpaths[0], fullpaths[1]);

	for(int i = 0; i < 2; i++){
		res = access(fullpaths[i], mask);
		if (res == -1){
			printf("Error in accessing %s!!!\n\n", fullpaths[i]);
			return -errno;
		}
	}
	printf("Out access!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	char fullpath[PATH_MAX];
	int res;

	printf("In readlink!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!111\n");
	printf("Path: %s\n", path);
	sprintf(fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, path);
	printf("fullpath: %s\n", fullpath);

	res = readlink(fullpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';

	printf("Out readlink!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11111\n\n\n");
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	char fullpath[PATH_MAX];

	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;

	printf("In readdir!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	printf("path: %s\n", path);
	sprintf(fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, path);
	printf("fullpath: %s\n", fullpath);

	dp = opendir(fullpath);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;

		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);

	printf("Out readdir!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n\n\n");
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In mknod!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s fullpath2: %s\n", fullpaths[0], fullpaths[1]);
	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	
	for (int i = 0; i < 2; ++i) {
		const char* fullpath = fullpaths[i];

    if (S_ISREG(mode)) {
      res = open(fullpath, O_CREAT | O_EXCL | O_WRONLY, mode);
      if (res >= 0)
        res = close(res);
    } else if (S_ISFIFO(mode))
      res = mkfifo(fullpath, mode);
    else
      res = mknod(fullpath, mode, rdev);
    if (res == -1)
      return -errno;
	}
	
	printf("Out mknod!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n\n\n");
	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In mkdir!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);

	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s fullpath2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		const char* fullpath = fullpaths[i];

		res = mkdir(fullpath, mode);
		if (res == -1)
    		return -errno;
	}

	printf("Out mkdir!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n\n\n");
	return 0;
}

static int xmp_unlink(const char *path)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In unlink!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s fullpath2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
    	const char* fullpath = fullpaths[i];
    	res = unlink(fullpath);
    	if (res == -1)
			return -errno;
	}

	printf("Out unlink!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n\n\n");
	return 0;
}

static int xmp_rmdir(const char *path)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In rmdir!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s fullpath2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		const char* fullpath = fullpaths[i];
		res = rmdir(fullpath);
		if (res == -1)
			return -errno;
	}
	printf("Out rmdir!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n\n\n");

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	char read_fullpath[PATH_MAX];
	char write_fullpaths[2][PATH_MAX];
	int res;

	printf("In symlink!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	printf("from: %s\n", from);
	printf("to: %s\n", to);
	sprintf(read_fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, from);
	printf("read fullpath: %s\n", read_fullpath);

	sprintf(write_fullpaths[0], "%s%s", global_context.driveA, to);
	sprintf(write_fullpaths[1], "%s%s", global_context.driveB, to);
	printf("write full path1: %s, 2: %s\n", write_fullpaths[0], write_fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		res = symlink(read_fullpath, write_fullpaths[i]);
		if (res == -1)
		return -errno;
	}

	printf("Out symlink!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n\n\n");
	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	char read_fullpath[PATH_MAX];
	char write_fullpaths[2][PATH_MAX];
	int res;

	printf("In rename!!!!!!!!!!!!!!!!!!!1\n");
	printf("from: %s\n", from);
	printf("to: %s\n", to);
	sprintf(read_fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, from);
	printf("read fullpath: %s\n", read_fullpath);
	sprintf(write_fullpaths[0], "%s%s", global_context.driveA, to);
	sprintf(write_fullpaths[1], "%s%s", global_context.driveB, to);
	printf("write full path1: %s, 2: %s\n", write_fullpaths[0], write_fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		res = rename(read_fullpath, write_fullpaths[i]);
		if (res == -1)
			return -errno;
	}

	printf("Out rename!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	char read_fullpath[PATH_MAX];
	char write_fullpaths[2][PATH_MAX];
	int res;

	printf("In link!!!!!!!!!!!!!!!!!!!1\n");
	printf("from: %s\n", from);
	printf("to: %s\n", to);
	sprintf(read_fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, from);
	printf("read fullpath: %s\n", read_fullpath);

	sprintf(write_fullpaths[0], "%s%s", global_context.driveA, to);
	sprintf(write_fullpaths[1], "%s%s", global_context.driveB, to);

	for (int i = 0; i < 2; ++i) {
		res = link(read_fullpath, write_fullpaths[i]);
		if (res == -1)
			return -errno;
	}

	printf("Out link!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In chmod!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s, mode: %d\n", path, mode);

	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpaths 1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		res = chmod(fullpaths[i], mode);
		if (res == -1)
			return -errno;
	}

	printf("Out getattr!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In chown!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
	res = lchown(fullpaths[i], uid, gid);
		if (res == -1)
			return -errno;
	}
	
	printf("Out chown!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In truncate!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		res = truncate(fullpaths[i], size);
		if (res == -1)
			return -errno;
	}

	printf("Out truncate!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	char fullpaths[2][PATH_MAX];
	int res;

	printf("In utimens!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
    /* don't use utime/utimes since they follow symlinks */
		res = utimensat(0, fullpaths[i], ts, AT_SYMLINK_NOFOLLOW);
		if (res == -1)
			return -errno;
	}

	printf("Out utimens!!!!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}
#endif

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
	char fullpaths[NUMOFDISK][PATH_MAX];
	int res;

	printf("In open!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	//printf("fullpath: %s\n", fullpath);

	for(int i = 0; i < NUMOFDISK; i++){
		res = open(fullpaths[i], fi->flags);
		if (res == -1)
			return -errno;

		close(res);
	}
	printf("Out open!!!!!!!!!!!!!!!!!\n\n\n");
  	return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  	char fullpath[PATH_MAX];
  	int fd;
  	int res;

	printf("In read!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, path);
	printf("fullpath: %s\n", fullpath);
  	(void) fi;
  	fd = open(fullpath, O_RDONLY);
	printf("offset: %lld\n", offset);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	printf("Out read!!!!!!!!!!!!!!!!\n\n\n");
  	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
    off_t offset, struct fuse_file_info *fi)
{
	char fullpaths[NUMOFDISK][PATH_MAX];
	int fd;
	int res;

	int block_num = 0;
	int written_size = offset % STRIPESIZE;
	int writable_size = 0;
	int write_size = 0;
	size_t bufflen = size;
	int write_point = 0;
	
	(void) fi;

	printf("In write!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	printf("Buf: %s\n", buf);
	printf("Size: %d\n", size);
	printf("Offset: %lld\n", offset);

	block_num = offset / STRIPESIZE;
	writable_size = STRIPESIZE - written_size;
	if(bufflen > writable_size)
		write_size = writable_size;
	else
		write_size = bufflen;

	write_point = block_num / NUMOFDISK * STRIPESIZE + written_size;

	printf("Writable size: %d, write_size: %d, writepoint: %d\n\n", writable_size, write_size, write_point);
	
	fd = open(fullpaths[block_num % 2], O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, write_size, write_point);
	if (res == -1)
		res = -errno;
	close(fd);

	printf("Out write!!!!!!!!!!!!!!!!\n\n\n");
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	char fullpath[PATH_MAX];
	int res;

	printf("In statfs!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, path);
	printf("fullpath: %s\n", fullpath);
	res = statvfs(fullpath, stbuf);
	if (res == -1)
		return -errno;

	printf("Out statfs!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{
  /* Just a stub.	 This method is optional and can safely be left
     unimplemented */

	printf("In release!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	(void) path;
	(void) fi;

	printf("Out release!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
    struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left unimplemented */

	printf("In fsync!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	(void) path;
	(void) isdatasync;
	(void) fi;

	printf("Out fsync!!!!!!!!!!!!!!!!\n\n\n");
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
    off_t offset, off_t length, struct fuse_file_info *fi)
{
	char fullpaths[2][PATH_MAX];
	int fd;
	int res;

	(void) fi;

	printf("In fallocate!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	if (mode)
		return -EOPNOTSUPP;

	for (int i = 0; i < 2; ++i) {
		const char* fullpath = fullpaths[i];

		fd = open(fullpath, O_WRONLY);
		if (fd == -1)
			return -errno;

		res = -posix_fallocate(fd, offset, length);

		close(fd);
	}

	printf("Out fallocate!!!!!!!!!!!!!!!!\n\n\n");
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
    size_t size, int flags)
{
	char fullpaths[2][PATH_MAX];

	printf("In setxattr!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		const char* fullpath = fullpaths[i];
		int res = lsetxattr(fullpath, name, value, size, flags);
		if (res == -1)
			return -errno;
	}

	printf("Out setxattr!!!!!!!!!!!!!!!!!!!1\n");
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value, size_t size)
{
	char fullpath[PATH_MAX];

	printf("In getxattr!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, path);
	printf("fullpath: %s\n", fullpath);

	int res = lgetxattr(fullpath, name, value, size);
	if (res == -1)
		return -errno;
	
	printf("Out getxattr!!!!!!!!!!!!!!!!!!!1\n");
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	char fullpath[PATH_MAX];

	printf("In listxattr!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpath, "%s%s", rand() % 2 == 0 ? global_context.driveA : global_context.driveB, path);
	printf("fullpath: %s\n", fullpath);

	int res = llistxattr(fullpath, list, size);
	if (res == -1)
		return -errno;

	printf("Out listxattr!!!!!!!!!!!!!!!!!!!1\n");
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	char fullpaths[2][PATH_MAX];

	printf("In removexattr!!!!!!!!!!!!!!!!!!!1\n");
	printf("path: %s\n", path);
	sprintf(fullpaths[0], "%s%s", global_context.driveA, path);
	sprintf(fullpaths[1], "%s%s", global_context.driveB, path);
	printf("fullpath1: %s, 2: %s\n", fullpaths[0], fullpaths[1]);

	for (int i = 0; i < 2; ++i) {
		const char* fullpath = fullpaths[i];
		int res = lremovexattr(fullpath, name);
		if (res == -1)
			return -errno;
	}

	printf("Out removexattr!!!!!!!!!!!!!!!!!!!1\n");
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations xmp_oper = {
  .getattr	= xmp_getattr,
  .access		= xmp_access,
  .readlink	= xmp_readlink,
  .readdir	= xmp_readdir,
  .mknod		= xmp_mknod,
  .mkdir		= xmp_mkdir,
  .symlink	= xmp_symlink,
  .unlink		= xmp_unlink,
  .rmdir		= xmp_rmdir,
  .rename		= xmp_rename,
  .link		= xmp_link,
  .chmod		= xmp_chmod,
  .chown		= xmp_chown,
  .truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
  .utimens	= xmp_utimens,
#endif
  .open		= xmp_open,
  .read		= xmp_read,
  .write		= xmp_write,
  .statfs		= xmp_statfs,
  .release	= xmp_release,
  .fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
  .fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
  .setxattr	= xmp_setxattr,
  .getxattr	= xmp_getxattr,
  .listxattr	= xmp_listxattr,
  .removexattr	= xmp_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	if (argc < 4) {
		fprintf(stderr, "usage: ./myfs <mount-point> <drive-A> <drive-B>\n");
		exit(1);
	}

	strcpy(global_context.driveB, argv[--argc]);
	strcpy(global_context.driveA, argv[--argc]);

	srand(time(NULL));

	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
