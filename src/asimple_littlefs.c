// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#include <lfs.h>

#include <asimple_littlefs.h>
#include <flash.h>

static int asimple_lfs_read(
	const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer,
	lfs_size_t size
)
{
	struct asimple_littlefs *fs = c->context;
	flash_wait_busy(fs->flash);
	flash_read_data(fs->flash, c->block_size * block + off, buffer, size);
	return 0;
}

static int asimple_lfs_prog(
	const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	const void *buffer, lfs_size_t size
)
{
	struct asimple_littlefs *fs = c->context;
	flash_wait_busy(fs->flash);
	flash_write_enable(fs->flash);
	flash_page_program(fs->flash, c->block_size * block + off, buffer, size);
	return 0;
}

static int asimple_lfs_erase(const struct lfs_config *c, lfs_block_t block)
{
	struct asimple_littlefs *fs = c->context;
	flash_wait_busy(fs->flash);
	flash_sector_erase(fs->flash, block * c->block_size);
	return 0;
}

static int asimple_lfs_sync(const struct lfs_config *c)
{
	struct asimple_littlefs *fs = c->context;
	flash_wait_busy(fs->flash);
	return 0;
}

void asimple_littlefs_init(struct asimple_littlefs *fs, struct flash *flash)
{
	fs->flash = flash;
	const struct lfs_config config = {
		.read = asimple_lfs_read,
		.prog = asimple_lfs_prog,
		.erase = asimple_lfs_erase,
		.sync = asimple_lfs_sync,
		.read_size = 1,
		.prog_size = 256,
		.block_size = 4096,
		.block_count = 512,
		.cache_size = 256,
		.lookahead_size = 8192,
		.block_cycles = 250,
		.context = fs,
	};
	fs->config = config;
}

int asimple_littlefs_format(struct asimple_littlefs *fs)
{
	return lfs_format(&fs->lfs, &fs->config);
}

int asimple_littlefs_mount(struct asimple_littlefs *fs)
{
	return lfs_mount(&fs->lfs, &fs->config);
}

int asimple_littlefs_unmount(struct asimple_littlefs *fs)
{
	return lfs_unmount(&fs->lfs);
}
