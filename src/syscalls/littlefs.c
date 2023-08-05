// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <asimple_littlefs.h>
#include <syscalls_internal.h>

#include <lfs.h>

#include <fcntl.h>

#include <stdint.h>

#include <errno.h>
#undef errno
extern int errno;

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

struct syscalls_littlefs
{
	struct syscalls_base base;
	struct asimple_littlefs *fs;
	struct lfs_file files[10];
	struct lfs_file *active_files[10];
};

int littlefs_open_(void *context, const char *name, int flags, int mode)
{
	(void)mode; // We don't use mode as we don't have permissions
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	size_t max = ARRAY_SIZE(fs->files);
	size_t i;
	for (i = 0; i < max; ++i)
	{
		if (fs->active_files[i] == NULL)
			break;
	}

	if (i >= max)
	{
		errno = ENFILE;
		return -1;
	}

	int lfs_flags = 0;
	if (O_RDONLY & flags)
		lfs_flags |= LFS_O_RDONLY;
	if (O_WRONLY & flags)
		lfs_flags |= LFS_O_WRONLY;
	if (O_RDWR & flags)
		lfs_flags |= LFS_O_RDWR;
	if (O_WRONLY & flags)
		lfs_flags |= LFS_O_WRONLY;
	if (O_EXCL & flags)
		lfs_flags |= LFS_O_EXCL;
	if (O_TRUNC & flags)
		lfs_flags |= LFS_O_TRUNC;
	if (O_APPEND & flags)
		lfs_flags |= LFS_O_APPEND;
	if (O_CREAT & flags)
		lfs_flags |= LFS_O_CREAT;

	int result = lfs_file_open(&fs->fs->lfs, &fs->files[i], name, lfs_flags);
	if (result < 0)
	{
		//FIXME convert into errno
		errno = result;
		return -1;
	}

	fs->active_files[i] = &fs->files[i];
	return i;
}

int littlefs_read_(void *context, int file, char *ptr, int len)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	int result = lfs_file_read(&fs->fs->lfs, fs->active_files[file], ptr, len);
	if (result < 0)
	{
		//FIXME convert into errno
		errno = result;
		return -1;
	}
	return result;
}

int littlefs_write_(void *context, int file, char *ptr, int len)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	int result = lfs_file_write(&fs->fs->lfs, fs->active_files[file], ptr, len);
	if (result < 0)
	{
		//FIXME convert into errno
		errno = result;
		return -1;
	}
	return result;
}

int littlefs_lseek_(void *context, int file, int ptr, int dir)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	int lfs_dir;
	switch(dir)
	{
		case SEEK_SET:
			lfs_dir = LFS_SEEK_SET;
			break;
		case SEEK_CUR:
			lfs_dir = LFS_SEEK_CUR;
			break;
		case SEEK_END:
			lfs_dir = LFS_SEEK_END;
			break;
	}
	int result = lfs_file_seek(&fs->fs->lfs, fs->active_files[file], ptr, lfs_dir);
	if (result < 0)
	{
		// FIXME convert into errno
		errno = result;
		return -1;
	}
	return result;
}

int littlefs_close_(void *context, int file)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	if (!fs->active_files[file])
	{
		errno = EBADF;
		return -1;
	}
	int result = lfs_file_close(&fs->fs->lfs, fs->active_files[file]);
	//FIXME what happens in the case of an error??????????????

	if (result < 0)
	{
		// FIXME convert into errno
		errno = result;
		return -1;
	}
	fs->active_files[file] = NULL;
	return result;


}

static int littlefs_stat_(void *context, const char* filename, struct stat *stat)
{
	struct syscalls_littlefs *fs = (struct syscalls_littlefs*)context;
	if (!fs)
	{
		errno = ENOENT;
		return -1;
	}

	struct lfs_info info;
	int err = lfs_stat(&fs->fs->lfs, filename, &info);

	// FIXME any other errors we care to report?
	if (err != LFS_ERR_OK)
	{
		errno = ENOENT;
		return -1;
	}

	struct stat result = {
		.st_dev = 2, // FIXME some enum somewhere?
		.st_ino = 2, // FIXME I don't even know what this means for lfs
		.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO,
		.st_nlink = 1,
		.st_uid = 0,
		.st_gid = 0,
		.st_rdev = 2, // FIXME
		.st_size = info.size,
		.st_blksize = fs->fs->lfs.cfg->block_size, // FIXME is this right?
		.st_blocks = (info.size - 1) / fs->fs->lfs.cfg->block_size + 1, // FIXME is this right?
		.st_atim = {0}, // FIXME
		.st_mtim = {0}, // FIXME
		.st_ctim = {0}, // FIXME
	};
	*stat = result;
	return 0;
}

static struct syscalls_littlefs fs = {
	.base = {
		.open = littlefs_open_,
		.close = littlefs_close_,
		.read = littlefs_read_,
		.write = littlefs_write_,
		.lseek = littlefs_lseek_,
		.stat = littlefs_stat_,
	},
};

void syscalls_littlefs_init(struct asimple_littlefs *fs_)
{
	fs.fs = fs_;
	syscalls_register_fs(&fs);
}
