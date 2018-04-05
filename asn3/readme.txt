Current build only works for files within the root directory.

Invoke `make` to create  4 separate executables.


-----------
Usage guide
-----------

Part 1:
Displays file system information.

$ ./diskinfo test.img


Part 2:
Ouputs a list of the files contained in the root directory.
Only use two arguments, as a third specifying a subdirectory has not been
implemented yet.

$ ./disklist test.img


Part 3:
Gets a file from an image, writing it to the current working directory.

$ ./diskget test.img <file in image> <output file name>


Part 4:
Write a file to an image's root directory.

Although it was stated that the root directory will not need to be expanded,
if the root directory is full, a new block will be allocated by updating the
FAT along with the superblock. (I thought this was necessary for the base
assignment, bonus feature perhaps?)

$ ./diskput test.img <existing local file> <new file in image>


--------------
Implementation
--------------

In all steps files were initially opened and checked for any errors.
Memory reading/writing is done using memcpy.

diskinfo:

The superblock_t struct is used, given the starting address, and all the needed
information is printed out following the formatting specified.

The FAT start location is used to loop through each FAT entry (4 bytes at a
time) and each value is read. Depending on the value, one of three variables
for free blocks, reserved blocks, or allocated blocks are updated and printed.

0: Free block
1: Reserved block
>1: Allocated block


disklist:

Remnants of a subdirectory implementation can be found inside this method,
but the work for this method is actually in print_root and print_dir_entry.
Starting at the directory block, sections are looped over in 64 byte intervals.
If the status byte is 3 or 5, it is a file or directory, respectively.
That information is then passed to print_dir_entry, where all the actual
file information is acquired and printed in the same way as in diskinfo.
This loop continues until the end of the root directory block.


diskget:

Directory entries are looped over, and when a file is detected, its name is
compared to the input name given. If they are the same, then this is the file
to get. A new file is opened with the new placement name. The number of blocks
the file takes up in memory is recorded from the directory entry, along with
its starting block. A loop then goes to that position and copies the block
size amount of bytes to the new file. Each time a block is copied, the FAT is
checked to find the next block where information is stored. This happens until
the very last block is reached, where a quick modulo finds the remaining number
of bytes in the block, where another copy happens with that amount of bytes,
avoiding copying extra.

diskput:

All filesystem information is recorded as usual. The FAT is looped through to
find a free block. The memory block location is determined from the position
in the FAT, and using an fread and memcpy, one block from the input file is
copied to that block in the image file. A variable for the last FAT location
is kept to retroactively update the FAT to contain the next location. If the
amount of needed blocks is reached, then FFFFFFFF is written to the FAT to
signify the end of the file.

The next step is to update the root directory entry to contain our new file.
This is done in the put function, which loops over the directory area as usual
and looks for an free spot. Once a free spot has been found, all the information
is updated accordingly.  A tm struct is used with the input file to find its
modify time, which is then converted into years, months, etc, using localtime,
strftime, and sscanf.

put() will return an integer value: 0 is successfully put, >0 if the directory
entries contain no new space. In this case, the root directory is expanded by
going to the FAT, updating it with allocation to free block, and updating
values in the superblock. Now that the root directory has more free space,
put() is called again to place the file in the directory.
