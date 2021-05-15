/**
 * @file sdcard.h
 * @author hust-os/xv6-k210 url:https://github.com/HUST-OS/xv6-k210
 * @brief copy for temporal usage; also fix the format of comment
 * @version 0.1
 * @date 2021-05-10
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef __SDCARD_H
#define __SDCARD_H

void sdcard_init(void);

void sdcard_read_sector(uint8 *buf, int sectorno);

void sdcard_write_sector(uint8 *buf, int sectorno);

void test_sdcard(void);

#endif 
