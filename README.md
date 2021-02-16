# V6FileSystem
Objective

Unix V6 file system has limitation of 16MB on file size. Use single unused bit of the flags field to increase file size support to 32MB. Develop command line interface that will allow Unix user access to the file system of foreign operating system, which is modified version of Unix Version 6. In other words, create and maintain content of file system up to the size of 32MB and provide user interface to manipulate and access the content.

fsaccess program will read a series of commands from the user an execute them. It supports the following commands:

Compilation instructions:

gcc fsaccess.c -o a

Run time:

./a file_system_name

initfs - Initializes the file system. Args - name of file system, number of blocks, number of inodes
cpin - Create a new file called v6-file in the current directory and fill in the contents of newly created file with contents of enternal file Args - externalfile, v6-file
cpout - If v6-file exists, create externalfile and make the externalfile's contents equal to v6-file. Args - v6-file, externalfile
mkdir v6-dir - If v6-dir does not exist, create a directory. Args - v6-dir
rm v6-file - If v6-file exists, delete the file. Args - v6-file
q - Save all changes and quit
METHODS OF EXECUTION:

Upon entry into program enter commands of the following type

initfs v6filesystem 5000 400. // initialize file system with 5000 blocks and 400 i-nodes. 65535 is maximum number of blocks. See below for explanation
mkdir v6-dir // create directory with dir_name. 
cpin externalfile v6-file // copy content of external_file into internal_file. 
cpout v6-file externalfile //copy content of internal_file into external_file
Rm internal_file  //remove file and mark its blocks as free
q
