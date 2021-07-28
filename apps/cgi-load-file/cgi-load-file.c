/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

void block_event(int fd)
{
    int val;
    /* Blocking read */
    int result = read(fd, &val, sizeof(val));
    if (result < 0) {
        printf("Error: %s\n", strerror(errno));
    }

}

void emit_event(char *emit)
{
    emit[0] = 1;
}

void memcpy_byte(void *dst, void *src, size_t size)
{
    char *dst_c = dst;
    char *src_c = src;
    for (int i = 0; i < size; i++) {
        *dst_c = *src_c;
        dst_c++;
        src_c++;
    }
}

int main(int argc, char *argv[])
{

    /* The requested file name is in PATH_INFO */
    char *query_string = getenv("PATH_INFO");

    /* Setup connection with native component via uio0 device file */
    char *dataport_name = "/dev/uio0";
    int length = 4096;

    int fd = open(dataport_name, O_RDWR);
    assert(fd >= 0);

    void *dataport;
    if ((dataport = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 1 * getpagesize())) == (void *) -1) {
        printf("mmap failed\n");
        close(fd);
    }

    char *emit;
    if ((emit = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 * getpagesize())) == (void *) -1) {
        printf("mmap failed\n");
        close(fd);
    }

    /* Write file name into data port */
    size_t file_name = query_string ? strnlen(query_string, 4095) + 1 : 0;
    memcpy_byte(dataport, query_string, file_name);
    emit_event(emit);

    /* Wait for response and print out the file */
    block_event(fd);
    for (char *chr = dataport; *chr != 0; chr++) {
        putchar(*chr);
    }
    fflush(stdout);
    munmap(dataport, length);
    munmap(emit, length);
    close(fd);

    return 0;
}
