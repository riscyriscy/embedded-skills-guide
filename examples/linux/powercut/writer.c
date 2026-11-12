/* Saves a record of four 4 KiB pages again and again, with one of four methods, so that a power cut can be
 * tried against each. Every page carries the generation number, and the last four bytes hold a CRC32 of
 * the rest, so a record that mixes two generations or lost a page can be recognised.
 * Usage: writer method directory [count]     method: inplace, truncate, rename or durable */
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PAGE 4096
#define PAGES 4
#define SIZE (PAGE * PAGES)

static uint32_t crc32(const unsigned char *p, size_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++)
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
    }
    return ~c;
}

static void make_record(unsigned char *rec, unsigned gen)
{
    for (int k = 0; k < PAGES; k++) {
        unsigned char *page = rec + k * PAGE;
        int n = snprintf((char *)page, 32, "PAGE gen=%08u k=%d\n", gen, k);
        for (int i = n; i < PAGE; i++)
            page[i] = (unsigned char)(gen + k + i);
    }
    uint32_t crc = crc32(rec, SIZE - 4);
    memcpy(rec + SIZE - 4, &crc, 4);
}

static void die(const char *what) { perror(what); exit(1); }

static void save(const char *method, const char *dir, const unsigned char *rec)
{
    char path[256], tmp[256];
    snprintf(path, sizeof path, "%s/state.bin", dir);
    snprintf(tmp, sizeof tmp, "%s/state.tmp", dir);
    if (!strcmp(method, "inplace")) {
        int fd = open(path, O_WRONLY);
        if (fd < 0) die("open");
        for (int k = 0; k < PAGES; k++)
            if (pwrite(fd, rec + k * PAGE, PAGE, k * PAGE) != PAGE) die("pwrite");
        close(fd);
    } else if (!strcmp(method, "truncate")) {
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0 || write(fd, rec, SIZE) != SIZE) die("write");
        close(fd);
    } else {
        int durable = !strcmp(method, "durable");
        int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0 || write(fd, rec, SIZE) != SIZE) die("write");
        if (durable && fsync(fd) < 0) die("fsync");
        close(fd);
        if (rename(tmp, path) < 0) die("rename");
        if (durable) {
            int dfd = open(dir, O_RDONLY | O_DIRECTORY);
            if (dfd < 0 || fsync(dfd) < 0) die("fsync directory");
            close(dfd);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: writer method directory [count]\n"); return 2; }
    unsigned count = argc > 3 ? (unsigned)atoi(argv[3]) : 0;
    unsigned char rec[SIZE];

    make_record(rec, 0);                 /* generation 0 is written with the durable method, whatever the test */
    save("durable", argv[2], rec);
    printf("start\n");
    fflush(stdout);
    for (unsigned gen = 1; count == 0 || gen <= count; gen++) {
        make_record(rec, gen);
        save(argv[1], argv[2], rec);
        printf("saved %u\n", gen);       /* the console line is the acknowledgement that a user of the product sees */
        fflush(stdout);
    }
    return 0;
}
