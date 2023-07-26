// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Gabriel Marcano, 2023

#ifndef ASIMPLE_LITTLEFS_H_
#define ASIMPLE_LITTLEFS_H_

#include <flash.h>
#include <lfs.h>

struct asimple_littlefs
{
	struct flash *flash;
	lfs_t lfs;
	struct lfs_config config;
};

void asimple_littlefs_init(struct asimple_littlefs *fs, struct flash *flash);
int asimple_littlefs_format(struct asimple_littlefs *fs);
int asimple_littlefs_mount(struct asimple_littlefs *fs);
int asimple_littlefs_unmount(struct asimple_littlefs *fs);

#endif//ASIMPLE_LITTLEFS_H_
