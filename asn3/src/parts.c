#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct Block_info {
    int start_block;
    int end_block;
    char *sub_name;
    char *address;
} block;

struct __attribute__((__packed__)) superblock_t {
   uint8_t   fs_id [8];
   uint16_t block_size;
   uint32_t file_system_block_count;
   uint32_t fat_start_block;
   uint32_t fat_block_count;
   uint32_t root_dir_start_block;
   uint32_t root_dir_block_count;
};

struct __attribute__((__packed__)) dir_entry_timedate_t {
   uint16_t year;
   uint8_t month;
   uint8_t day;
   uint8_t hour;
   uint8_t minute;
   uint8_t second;
};

struct __attribute__((__packed__)) dir_entry_t {
   uint8_t status;
   uint32_t starting_block;
   uint32_t block_count;
   uint32_t size;
   struct dir_entry_timedate_t create_time;
   struct dir_entry_timedate_t modify_time;
   uint8_t filename[31];
   uint8_t unused[6];
};


int diskinfo(int argc, char *argv[]) {

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if (fstat(fd, &buffer) < 0) {
        perror("FSTAT ERROR");
        return 1;
    }

    printf("Super block information:\n");

    void* add = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb = (struct superblock_t*) add;
    printf("Block size: %d\n", htons(sb->block_size));
    printf("Block count: %d\n", ntohl(sb->file_system_block_count));
    printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
    printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
    printf("Root directory starts: %d\n", ntohl(sb->root_dir_start_block));
    printf("Root directory blocks: %d\n", ntohl(sb->root_dir_block_count));

    printf("\n");

    int fat_start_block = htons(sb->block_size) * ntohl(sb->fat_start_block);
    int fat_end_block = htons(sb->block_size) * ntohl(sb->fat_block_count);

    // getting FAT information
    int free_blocks = 0;
    int reserved_blocks = 0;
    int allocated_blocks = 0;
    int i;
    for (i = fat_start_block; i < fat_start_block + fat_end_block; i += 4) {
        int curr = 0;
        memcpy(&curr, add + i, 4);
        curr = htonl(curr);
        if (curr == 0) free_blocks++;
        else if (curr == 1) reserved_blocks++;
        else allocated_blocks++;
    }

    printf("FAT information:\n");
    printf("Free Blocks: %d\n", free_blocks);
    printf("Reserved blocks: %d\n", reserved_blocks);
    printf("Allocated Blocks: %d\n", allocated_blocks);

    munmap(add, buffer.st_size);
    close(fd);
    return 0;
}

void print_dir_entry(char *address, int i) {
   int size = 0, year = 0, month = 0, day = 0, hours = 0, minutes = 0, seconds = 0;

   struct dir_entry_t* de;
   de = (struct dir_entry_t*) (address+i);

   int status = de->status;
   if (status == 3) {
       status = 'F';
   } else if (status == 5) {
       status = 'D';
   }

   size = ntohl(de->size);
   unsigned char* name = de->filename;
   year = htons(de->modify_time.year);
   month = de->modify_time.month;
   day = de->modify_time.day;
   hours = de->modify_time.hour;
   minutes = de->modify_time.minute;
   seconds = de->modify_time.second;

    printf("%c %10d %30s %d/%02d/%02d %02d:%02d:%02d\n",
           status, size, name, year, month, day, hours, minutes, seconds);
}

void print_root(int root_start_block, int root_end_block, char *address, int fat_start, int fat_blocks, int block_size) {
   int root_block = root_start_block;

   while(1){
     int i;
     // iterate through directory block
       for (i = root_block; i < root_block + block_size; i += 64) {
           int curr = 0;
           memcpy(&curr, address + i, 1);
           // continue if not a file
           if (curr != 3 && curr != 5){ continue; }
           print_dir_entry(address, i);
       }

       // this style of while loop is used multiple times, iterating through
       // directory entries following the FAT, in case the directory is not
       // sequential
       int fat_dir_location = (root_block/block_size)*4 + (fat_start * block_size);
       memcpy(&root_block, address + fat_dir_location, 4);
       root_block = htonl(root_block);
       // break if at the end of the FAT
       if (root_block == -1){ break; }
       root_block = root_block * block_size;
    }
}

block *find_dir(block *temp) {
    // unused method that was a first idea for implementing sub directory
    // features.
    printf("%d\n", temp->start_block);
    int i;
    for (i = temp->start_block; i < temp->start_block + temp->end_block; i += 64) {
        int curr = 0;
        memcpy(&curr, temp->address + i, 1);
        if (curr == 5) {

            // test if the current directories name is equal to the name we're searching for
            char name[31];
            memcpy(&name, temp->address + i + 27, 31);
            if (!strcmp(name, temp->sub_name)) {

                // find its start block and length, return a new block* with that information
                block *new;
                new = (block *) malloc(sizeof(block));

                // find new directory's block information
                int block_size = 0, root_start = 0, root_blocks = 0;

                memcpy(&block_size, temp->address + 8, 2);
                block_size = htons(block_size);

                memcpy(&root_start, temp->address + 24, 2);
                root_start = htons(root_start);

                memcpy(&root_blocks, temp->address + 28, 2);
                root_blocks = htons(root_blocks);

                new->start_block = root_start * block_size;
                new->end_block = root_blocks * block_size;
                new->address = temp->address;
                return new;
            }
        }
    }

    printf("Found no matching subdirectories %s\n", temp->sub_name);

    return temp;
}

int disklist(int argc, char *argv[]) {

    // if argc == 2, just print out root directory
    // else we will assume that the third arguement contains a a subdirectory path
    int index = 0;
    char *tokens[256];
    if (argc == 3) {
        // tokenize argv[2] based on slashes
        char *arguments = strtok(argv[2], "/");
        while (arguments) {
            tokens[index] = arguments;
            index++;
            arguments = strtok(NULL, "/");
        }
        tokens[index] = NULL;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if (fstat(fd, &buffer) < 0) {
        perror("FSTAT ERROR");
        return 1;
    }

    void* address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb = (struct superblock_t*) address;
    int fat_start = ntohl(sb->fat_start_block);
    int fat_blocks = ntohl(sb->fat_block_count);

    int block_size = 0, root_start = 0, root_blocks = 0;

    memcpy(&block_size, address + 8, 2);
    block_size = htons(block_size);

    memcpy(&root_start, address + 24, 2);
    root_start = htons(root_start);

    memcpy(&root_blocks, address + 28, 2);
    root_blocks = htons(root_blocks);

    int root_start_block = root_start * block_size;
    int root_end_block = root_blocks * block_size;

    // just print out the root dir if no subdirectory path is given
    if (argc == 2) {
        print_root(root_start_block, root_end_block, address, fat_start, fat_blocks, block_size);
    }

    // unused in submitted assignment
    else {
        block *temp;
        temp = (block *) malloc(sizeof(block));
        temp->start_block = root_start_block;
        temp->end_block = root_end_block;
        temp->sub_name = tokens[0];
        temp->address = address;
        int count = 0;

        while (count < index) {
            printf("%d  %d  %s\n", temp->start_block, temp->end_block, temp->sub_name);
            temp = find_dir(temp);
            count++;
            temp->sub_name = tokens[count];
        }
    }
    munmap(address, buffer.st_size);
    close(fd);
    return 0;
}

int diskget(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Error: Incorrect usage. Needs 4 arguments\n");
        return 0;
    }
    char *file_name;
    char *file_placement;
    file_name = argv[2];
    file_placement = argv[3];

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if (fstat(fd, &buffer) < 0) {
        perror("FSTAT ERROR");
        return 1;
    }
    char *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int block_size = 0, root_start = 0, root_blocks = 0, fat_start = 0, fat_blocks = 0;
    memcpy(&block_size, address + 8, 2);
    block_size = htons(block_size);
    memcpy(&root_start, address + 24, 2);
    root_start = htons(root_start);
    memcpy(&root_blocks, address + 28, 2);
    root_blocks = htons(root_blocks);
    memcpy(&fat_start, address + 16, 2);
    fat_start = htons(fat_start);
    memcpy(&fat_blocks, address + 20, 2);
    fat_blocks = htons(fat_blocks);

    int root_block = root_start * block_size;

    while(1){
        int i;
        for (i = root_block; i < root_block + block_size; i += 64) {
           int curr = 0;
           memcpy(&curr, address + i, 1);

           // if we found a file entry
           if (curr == 3) {
               char name[31];
               memcpy(&name, address + i + 27, 31);

               // compare names, if the same, the file has been found
               if (!strcmp(name, file_name)) {
                   FILE *fp;
                   fp = fopen(file_placement, "wb");

                   int current_block = 0, num_blocks = 0, fat_address = 0;

                   // find how many blocks it needs
                   memcpy(&num_blocks, address + i + 5, 4);
                   num_blocks = htonl(num_blocks);

                   // printf("Current block address: %x\n", i);
                   memcpy(&current_block, address + i + 1, 4);
                   current_block = htonl(current_block);

                   int file_size;
                   memcpy(&file_size, address+i+9, 4);
                   file_size = htonl(file_size);
                   int j;
                   // for each needed block -1, copy block contents in block_size intervals
                   for (j = 0; j < num_blocks-1; j++) {
                       fwrite(address + (block_size * current_block), 1, block_size, fp);
                       fat_address = fat_start * block_size + current_block * 4;

                       memcpy(&curr, address + fat_address, 4);
                       curr = htonl(curr);
                       current_block = curr;
                   }

                   // determine remaining size left to copy
                   int remaining = file_size % block_size;

                   // if the remaining is 0, then it's a full block_size multiple
                   if (remaining == 0) {
                      // to copy the last contents
                      remaining = block_size;
                   }

                   // write remaining information
                   fwrite(address + (block_size * current_block), 1, remaining, fp);
                   fat_address = fat_start * block_size + current_block * 4;
                   memcpy(&curr, address + fat_address, 4);

                   munmap(address, buffer.st_size);
                   close(fd);
                   printf("Getting %s from %s and writing as %s.\n", file_name, argv[1], file_placement);
                   return 0;
               }
           }
        }
        int fat_dir_location = (root_block/block_size)*4 + (fat_start * block_size);
        memcpy(&root_block, address + fat_dir_location, 4);
        root_block = htonl(root_block);
        if (root_block == -1){ break; }
        root_block = root_block * block_size;
    }

    printf("File not found.\n");
    munmap(address, buffer.st_size);
    close(fd);
    return 0;
}

int put(int root_block, int block_size, void* address, struct stat buf, char* file_placement, int file_size, int needed_blocks, int starting_block, int fat_start_block, int fat_dir_location) {
   int stat = 3;
   int i;
   int endf = 0xFFFFFFFF;
   int endff = 0xFFFF;
   while(1){
     // go through directory
      for (i = root_block; i < root_block + block_size; i += 64) {
         int curr = 0;
         memcpy(&curr, address + i, 1);

         // find open spot
         if (curr == 0){
            memcpy(address+i, &stat, 1); // status

            starting_block = ntohl(starting_block);
            memcpy(address+i+1, &starting_block, 4);

            needed_blocks = htonl(needed_blocks);
            memcpy(address+i+5, &needed_blocks, 4);

            file_size = htonl(file_size);
            memcpy(address+i+9, &file_size, 4);

            // create time & modify time
            struct tm *tm;
            char buff[10];
            tm = localtime(&buf.st_mtime);
            int k;

            // Create time and modify time the same
            strftime(buff, sizeof(buff), "%Y", tm);
            sscanf(buff, "%d", &k);
            k = htons(k);
            memcpy(address+i+20, &k, 2);
            memcpy(address+i+13, &k, 2);

            strftime(buff, sizeof(buff), "%m", tm);
            sscanf(buff, "%d", &k);
            memcpy(address+i+22, &k, 1);
            memcpy(address+i+15, &k, 1);

            strftime(buff, sizeof(buff), "%d", tm);
            sscanf(buff, "%d", &k);
            memcpy(address+i+23, &k, 1);
            memcpy(address+i+16, &k, 1);

            strftime(buff, sizeof(buff), "%H", tm);
            sscanf(buff, "%d", &k);
            memcpy(address+i+24, &k, 1);
            memcpy(address+i+17, &k, 1);

            strftime(buff, sizeof(buff), "%M", tm);

            sscanf(buff, "%d", &k);
            memcpy(address+i+25, &k, 1);
            memcpy(address+i+18, &k, 1);

            strftime(buff, sizeof(buff), "%S", tm);
            sscanf(buff, "%d", &k);
            memcpy(address+i+26, &k, 1);
            memcpy(address+i+19, &k, 1);

            strncpy(address+i+27, file_placement, 31);

            memcpy(address+i+58, &endf, 4);
            memcpy(address+i+62, &endff, 2);

            return 0;
          }
      }
      fat_dir_location = (root_block/block_size)*4 + (fat_start_block);
      memcpy(&root_block, address + fat_dir_location, 4);
      root_block = htonl(root_block);
      if (root_block == -1){ break; }
      root_block = root_block * block_size;
   }

   // return nonzero value if no entry was added, which is the case where the
   // directory is full
   return fat_dir_location;
}

int diskput(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Error: Incorrect usage. Needs 4 arguments\n");
        return 0;
    }

    char *file_name;
    char *file_placement;
    file_name = argv[2];
    file_placement = argv[3];

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if (fstat(fd, &buffer) < 0) {
        perror("FSTAT ERROR");
        return 1;
    }

    struct stat buf;
    int f = open(file_name, O_RDONLY);
    if (fstat(f, &buf) < 0) {
        printf("File not found.\n");
        return 1;
    }

    // int mod_time = buf.st_mtime;
    int file_size = buf.st_size;

    close(f);

    FILE *fp;
    fp = fopen(file_name, "rb");

    printf("Putting %s into %s as %s.\n", file_name, argv[1], file_placement);

    void* address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    struct superblock_t* sb;
    sb = (struct superblock_t*) address;

    int block_size = htons(sb->block_size);
    int fat_start_block = ntohl(sb->fat_start_block) * block_size;
    int fat_block_count = ntohl(sb->fat_block_count);
    int fat_end_block = fat_start_block + fat_block_count * block_size;
    int root_dir_start_block = ntohl(sb->root_dir_start_block) * block_size;

    // going through FAT
    int needed_blocks = file_size / block_size;
    if (file_size % block_size != 0){
      needed_blocks++;
    }

    int blocks_used = 0;
    int end = 0xFFFFFFFF;
    int mem_location = 0, last_FAT = 0, starting_block = 0;
    char file_buffer[512];
    int i;
    for (i = fat_start_block; i < fat_end_block; i += 4) {
        int curr = -1;
        memcpy(&curr, address + i, 4);
        curr = htonl(curr);
        // found a free block
        if (curr == 0) {
           if (blocks_used == 0) {
               // on first iteration, don't update FAT but record our location
               starting_block = (i-fat_start_block)/4;
               last_FAT = i;
               blocks_used++;

               fread(file_buffer, block_size, 1, fp);
               //printf("Write memory to %x\n", starting_block*block_size)
               memcpy(address+(starting_block*block_size), &file_buffer, block_size);

               continue;
           }
           mem_location = (i-fat_start_block)/4;

           // read in and write information
           fread(file_buffer, block_size, 1, fp);
           memcpy(address+(mem_location*block_size), &file_buffer, block_size);

           int loc = htonl(mem_location);
           // copy current location to the last FAT entry, creating the linked list
           memcpy(address+last_FAT, &loc, 4);
           last_FAT = i;
           blocks_used++;

           if (blocks_used == needed_blocks){
               memcpy(address+i, &end, 4);
               break;
           }
        }
    }


    // go to root dir to find open spot to add entry
    int root_block = root_dir_start_block;
    int fat_dir_location = 0;

    // int root_block, int block_size, void* address, struct stat buf, char* file_placement, int file_size, int needed_blocks, int starting_block, int fat_start_block, int fat_dir_location)
    fat_dir_location = put(root_block, block_size, address, buf, file_placement, file_size, needed_blocks, starting_block, fat_start_block, fat_dir_location);

    // on successful retrun, exit
    if (!fat_dir_location) { return 0; }


    // if we get here, then the root directory must be full and therefor
    //  we need to allocate more space
    for (i = fat_dir_location; i < fat_end_block; i += 4) {
        int curr = -1;
        memcpy(&curr, address + i, 4);
        curr = htonl(curr);
        // found an empty location in the FAT
        if (curr == 0) {
           // write new location to old FAT entry
           int new = htonl((i - fat_start_block)/4);
           memcpy(address + fat_dir_location, &new, 4);

           // set new FAT location to 0xFFFFFFFF
           memcpy(address + i, &end, 4);

           int root_blocks = ntohl(sb->root_dir_block_count) + 1;
           root_blocks = ntohl(root_blocks);
           memcpy(address+26, &root_blocks, 4);
           break;
        }
    }
    put(root_block, block_size, address, buf, file_placement, file_size, needed_blocks, starting_block, fat_start_block, fat_dir_location);

    return 0;
}

int main(int argc, char *argv[]) {
  #if defined(PART1)
      diskinfo(argc, argv);
  #elif defined(PART2)
      disklist(argc, argv);
  #elif defined(PART3)
      diskget(argc, argv);
  #elif defined(PART4)
      diskput(argc,argv);
  #else
  # error "PART[1234] must be defined"
  #endif

  return 0;
}
