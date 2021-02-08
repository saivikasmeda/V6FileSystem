//ProjectMates :  Saivikas Meda - sxm190011              - 2021486546
//                Nirmal Aagash Chandramohan- nxc180026  - 2021482360

// Class :  CS5348.001
//Project:  Project3 V6 filesystem modifications

/***********************************************************************
 This program allows user to do two things:
   1. initfs: Initializes the file system and redesigning the Unix file system to accept large
      files of up to 4GB, expands the free array to 152 elements, expands the i-node array to
      200 elements, doubles the i-node size to 64 bytes and other new features as well.
   2. Quit: save all work and exit the program.

 User Input:
     - initfs (file path) (# of total system blocks) (# of System i-nodes)
     - cpin  <extsourcefilepath> <v6destfilepath>
     - cpout  <v6sourcefilepath> <extdestfilepath>
     - mkdir  <directoryName>
     - rm  <filename>
     - ls
     - cd <directorypath>
     - rmdir <directoryName>
     - open <filesystemfileName>
     - help
     - q
 File name is limited to 12 characters.
 ***********************************************************************/

//Including all the required header files
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include <stdbool.h>

//Defining the sizes of all arrays
#define FREE_SIZE 124  // total free blocks available
#define I_SIZE 124 // Total free inodes possible
#define BLOCK_SIZE 1024 // Size of each block.
#define ADDR_SIZE 11 // Determine the size of the size.
#define INPUT_SIZE 256 // Maximum size of entered input


// Superblock Structure

typedef struct {
  unsigned int isize;  //Number of blocks allocated to i-nodes
  unsigned int fsize; //First free_block that is not available for allocation to a file
  unsigned int free[FREE_SIZE]; //Number of free blocks
  unsigned int nfree;  //Number of blocks that are available to allocate
  unsigned int ninode; //Number of ninodes
  unsigned int inode[I_SIZE]; //Contains the number of blocks devoted to i-list
  unsigned short flock;  //flag maintained in the file system
  unsigned short ilock;//flag maintained in the file system
  unsigned int fmod; //to show that the superblock has cchanged and should be copied to disk
  unsigned int time[2]; //takes note of the time starting from Jan 1970 in double precision representation of number of seconds.
} superblock_type;

superblock_type superBlock;

// I-Node Structure

typedef struct {
    unsigned short flags;
    unsigned short nlinks;  //number of links to the file
    unsigned short uid; //User ID
    unsigned short gid; //Group ID
    unsigned int size;  //byte of 24-bit
    unsigned int addr[ADDR_SIZE];   //block numbers/device numbers with addr_size as 11
    unsigned int actime;   //last access time
    unsigned int modtime;  //last modified time} inode_type;
} inode_type;
inode_type inode;

//Directory Structure
typedef struct {
    unsigned int inode;   //Takes note  if inode is allocated or not
     char filename[12]; //File name or the directory name} dir_type;
}dir_type;


dir_type root;
unsigned int currentInodeNumber;
bool filesystemExists = false;
char presentWorkingDir[1000];
int fileDescriptor ;		//file descriptor
const unsigned short inode_alloc_flag = 0100000;    //inode is allocated (BUG FIXED : 0 cannot be in front of the 1 since it is 16bits long.)
const unsigned short dir_flag = 040000;             //directory flags
const unsigned short dir_large_file = 010000;       //large file
const unsigned short dir_access_rights = 000777;    // User, Group, & World have all access privileges (read,write and execute)
const unsigned short INODE_SIZE = 64; // inode has been doubled (2*32 bytes)
int zeroDirType[16];


inode_type getCurInode();
void add_block_to_freeList( unsigned int blocknumber , unsigned int *empty_buffer );
void createDirectory(char *directoryName, unsigned int inodeNum);
unsigned int getinode();
unsigned int allocate_Data_Block();
unsigned int getInodeFromFilename(unsigned int inodeNum, const char* filename);
void listDirectory();
void preInitialize();
void openFileSystem();
void change_Directory(const char *newDirectoryName);
void remove_Directory(const char *directoryName);
bool isDirectory(unsigned int num);
void updatepwd(char *last);
void deallocateInodeDirectory(unsigned int num);
bool inodeExists(unsigned int num);
void cpin(char* src, char* targ);
void cpout(char* src, char* targ);
void writeAtBlock(unsigned int block_num, void * buffer, unsigned int bytes_read);
void writeToBlockAtOffset(unsigned int block_num, int offset, unsigned int buffer[], unsigned int nbytes);
inode_type getInodeFromNum(unsigned int num);
bool isFile(unsigned int num);
void readFromBlockOffset(unsigned int block_num, int offset, void * buffer, unsigned int nbytes);
void removeFile(char *fileName);
void deallocateInodeFile(unsigned int num);
void create_root_directory();
void initfs(char* path, unsigned int blocks, unsigned int inodes);
void help();


int main() {
  char input[INPUT_SIZE]; //Assigning input array of size 256
  char *splitter;  //splitter char pointer is defined
  char *filepath;  //filepath char pointer is defined
  int i;
  for (i=0;i<16;i++) {
    zeroDirType[i] = 0;
  }
    
  printf("Size of super block = %lu , size of i-node = %lu\n",sizeof(superBlock),sizeof(inode));
  help();
  printf("Please initialize the file system or open filesystem\n");
  while(1) {
    if(filesystemExists) {
        printf(">>>%s :  ",presentWorkingDir);
    }
    scanf(" %[^\n]s", input); //Reading user input
    splitter = strtok(input," "); //tokensize the read input with respect to space
    if(strcmp(splitter, "initfs") == 0){ //if the first word is initfs , execute preInitialization Method
        preInitialize();  //preInitialization() method is invoked.
    } else if(strcmp(splitter, "cpin")==0){
        char *destinationName;
        char *sourceFile;
        sourceFile = strtok(NULL, " ");
        destinationName = strtok(NULL, " ");
        cpin(sourceFile,destinationName);
    } else if(strcmp(splitter, "cpout")==0) {
        if(!filesystemExists) {
            printf("The file system is not initialized. Please retry after initializing file system\n");
        } else {
            char *destinationName;
            char *sourceFile;
            sourceFile = strtok(NULL, " ");
            destinationName = strtok(NULL, " ");
            cpout(sourceFile,destinationName);
        }
    } else if(strcmp(splitter, "mkdir")==0) {
        char *directoryName;
        directoryName = strtok(NULL, " ");
        if(getInodeFromFilename(currentInodeNumber,directoryName) != 0){
            printf("Directory exists. Please change the directoryName.\n");
        } else {
            unsigned int inodeNum = getinode();
            if (inodeNum == 0) {
                printf("Inodes are not avaialable\n");
            } else {
                createDirectory(directoryName, inodeNum);
            }
        }
    } else if (strcmp(splitter, "rm") == 0) {  // Remove the particular file from the directory
        char *fileName;
        fileName = strtok(NULL, " ");
        removeFile(fileName);
    }else if (strcmp(splitter, "ls")==0){   // Display the files present in the present directory
          listDirectory();
    }  else if (strcmp(splitter, "cd")==0) {
        char *newDirectoryName;
        newDirectoryName = strtok(NULL, " ");
        change_Directory(newDirectoryName);
    } else if (strcmp(splitter, "rmdir")==0) {  // remove the directory from the filesystem
        char *directoryName;
        directoryName = strtok(NULL, " ");
        remove_Directory(directoryName);
    }else if (strcmp(splitter, "pwd") == 0) {  // Display the present working directory
          printf("%s\n",presentWorkingDir);
    } else if(strcmp(splitter, "open")==0){
        openFileSystem();
        printf("Successfully opened the file system.\n");
    } else if (strcmp(splitter, "help") == 0){
        help();
    } else if (strcmp(splitter, "q") == 0) { //Checking if the token1 is "q" Quit from the filesystem
       lseek(fileDescriptor, BLOCK_SIZE, 0);
       write(fileDescriptor, &superBlock, BLOCK_SIZE); // Updating the superblock contents to file before exiting
       close(fileDescriptor);
       return 0;
    } else {
      printf("Not a valid command. Please \"help\" to get the list of commands.\n");
    }
    
  }
}

    // this method opens the filesystem provided.
void openFileSystem(){
    char *filepath;
    filepath = strtok(NULL, " ");
    if(access(filepath, F_OK) != -1) {
        fileDescriptor = open(filepath,O_RDWR);
        lseek(fileDescriptor,BLOCK_SIZE, SEEK_SET);
        read(fileDescriptor,&superBlock,BLOCK_SIZE);
        filesystemExists = true;
        currentInodeNumber = 1;
        strcpy(presentWorkingDir,"/");
        
    } else {
    printf("File not found");
    }
}

    // Displays the list of all files present the present directory
void listDirectory(){
    inode_type currentInode = getCurInode();
    dir_type directoryEntry;
    lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]),SEEK_SET);
    int i;
    for(i=0; i<currentInode.nlinks + 2; i++) {
        read(fileDescriptor,&directoryEntry,sizeof(directoryEntry));
        if (strcmp(directoryEntry.filename, "") != 0) {
            printf("%s\n",directoryEntry.filename);
        } else {
            break;
        }
    }
}

    // This method removes the file from the present directory of filesystem.
void removeFile(char *fileName) {
  unsigned int inodenum = getInodeFromFilename(currentInodeNumber,fileName);
  if (inodenum == 0 || !inodeExists(inodenum)) {
    printf("File doesn't exist\n");
  } else {
    inode_type fInode = getInodeFromNum(inodenum);
      if (isFile(inodenum)) {
        int flag = 1;
        int index = 0;
        unsigned int block_num;
        unsigned int* fInodeAddr = fInode.addr;

          unsigned int buffer[BLOCK_SIZE/4];
          int i;
          for (i = 0; i < BLOCK_SIZE/4; i++) { buffer[i] = 0; }

          while(flag) {
          if(index!=10) {
            block_num = fInodeAddr[index];
            if(index==fInode.size/BLOCK_SIZE) {
              add_block_to_freeList(block_num, buffer);
              flag=0;
            } else {
              add_block_to_freeList(block_num, buffer);
            }
            index++;
          } else {
            unsigned int b1[BLOCK_SIZE/4];
            int i;
            for (i = 0; i < BLOCK_SIZE/4; i++) { b1[i] = 0; }
            readFromBlockOffset(fInodeAddr[10], 0, b1, BLOCK_SIZE);
            int k;
            for(k=0;k<BLOCK_SIZE/4;k++) {
              unsigned int b2[BLOCK_SIZE/4]={0};
              readFromBlockOffset(b1[k], 0, b2, BLOCK_SIZE);
              int l;
              for(l=0;l<BLOCK_SIZE/4;l++) {
                unsigned int b3[BLOCK_SIZE/4]={0};
                readFromBlockOffset(b2[l], 0, b3, BLOCK_SIZE);
                int m;
                for(m=0;m<BLOCK_SIZE/4;m++) {
                  block_num = b3[m];
                  if(index==fInode.size/BLOCK_SIZE) {
                    add_block_to_freeList(block_num, buffer);
                    flag=0;
                    k=BLOCK_SIZE/4;
                    l=BLOCK_SIZE/4;
                    m=BLOCK_SIZE/4;
                  } else {
                    add_block_to_freeList(block_num, buffer);
                  }
                  index++;
                }
              }
            }
          }
        }
        deallocateInodeFile(inodenum);
        inode_type currentInode;
        lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
        read(fileDescriptor, &currentInode, INODE_SIZE);
        if (currentInode.nlinks == 1) {
          lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * 2, SEEK_SET);
          write(fileDescriptor, zeroDirType, sizeof(dir_type));
        } else {
            int deleteinode;
            int lastInode = currentInode.nlinks + 1;
            dir_type directoryEntry;
            lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]),SEEK_SET);
            int i;
            for(i=0;i<currentInode.nlinks + 2; i++){
              read(fileDescriptor,&directoryEntry,sizeof(directoryEntry));
              if(strcmp(fileName,directoryEntry.filename) == 0) {
                deleteinode = i;
                break;
              }
            }
            if (deleteinode == lastInode) {
              lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * deleteinode, SEEK_SET);
              write(fileDescriptor, zeroDirType, sizeof(dir_type));
            } else {
              dir_type entry;
              lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * lastInode, SEEK_SET);
              read(fileDescriptor,&entry,sizeof(dir_type));
              lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * lastInode, SEEK_SET);
              write(fileDescriptor, zeroDirType, sizeof(dir_type));
              lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * deleteinode, SEEK_SET);
              write(fileDescriptor, &entry, sizeof(dir_type));
            }
        }
        currentInode.nlinks--;
        lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
        write(fileDescriptor, &currentInode, INODE_SIZE);
        printf("Successfully removed the file.\n");
      }
      else {
        printf("Please enter a filename\n");
      }
  }
}

    
    // This method copies the file from file system intot he destination file in the computer
void cpout(char* src, char* targ){
  unsigned int inodenum = getInodeFromFilename(currentInodeNumber,src);
  if (inodenum == 0) {
    printf("Error: src file(%s) doesn't exist\n",src);
  } else {
    inode_type srcInode = getInodeFromNum(inodenum);
    if (!isFile(inodenum)) {
      printf("Error: src file(%s) is a dir\n",src);
    } else {
      int destination;
      if((destination = open(targ,O_RDWR|O_CREAT,0600))== -1) {
        printf("\n Error opening destination file (%s) Error: [%s]\n", targ, strerror(errno));
        return;
      }
      int flag = 1;
      int index = 0;
      unsigned int block_num;
      unsigned int* srcInodeAddr = srcInode.addr;

      unsigned int buffer[BLOCK_SIZE/4];
      int i;
      for (i = 0; i < BLOCK_SIZE/4; i++) { buffer[i] = 0; }
      while(flag) {
        if(index!=10) {
          block_num = srcInodeAddr[index];
          if(index==srcInode.size/BLOCK_SIZE) {
            readFromBlockOffset(block_num, 0, buffer, srcInode.size%BLOCK_SIZE);
            write(destination,buffer,srcInode.size%BLOCK_SIZE);
            flag=0;
          } else {
            readFromBlockOffset(block_num, 0, buffer, BLOCK_SIZE);
            write(destination,buffer,BLOCK_SIZE);
          }
          index++;
        } else {
          unsigned int b1[BLOCK_SIZE/4]={0};
          readFromBlockOffset(srcInodeAddr[10], 0, b1, BLOCK_SIZE);
          int k;
          for(k=0;k<BLOCK_SIZE/4;k++) {
            unsigned int b2[BLOCK_SIZE/4]={0};
            readFromBlockOffset(b1[k], 0, b2, BLOCK_SIZE);
            int l;
            for(l=0;l<BLOCK_SIZE/4;l++) {
              unsigned int b3[BLOCK_SIZE/4]={0};
              readFromBlockOffset(b2[l], 0, b3, BLOCK_SIZE);
              int m;
              for(m=0;m<BLOCK_SIZE/4;m++) {
                block_num = b3[m];
                if(index==srcInode.size/BLOCK_SIZE) {
                  readFromBlockOffset(block_num, 0, buffer, srcInode.size%BLOCK_SIZE);
                  write(destination,buffer,srcInode.size%BLOCK_SIZE);
                  flag=0;
                  k=BLOCK_SIZE/4;
                  l=BLOCK_SIZE/4;
                  m=BLOCK_SIZE/4;
                } else {
                  readFromBlockOffset(block_num, 0, buffer, BLOCK_SIZE);
                  write(destination,buffer,BLOCK_SIZE);
                }
                index++;
              }
            }
          }
        }
      }
    }
  }
  printf("Successfully copied from v6 to external\n");
}

    
    // this method copies the source file into the v6 filesystem at the working directory
void cpin(char* src, char* targ){
  int srcfd;
  if((srcfd = open(src,O_RDONLY))== -1) {
    printf("\nError opening src file: %s \n",src);
    return;
  }
  unsigned int tmp1= getInodeFromFilename(currentInodeNumber,targ);
  if(tmp1 != 0 && inodeExists(tmp1)){
    printf("Given destination file(%s) already exists\n", targ);
    return;
  }

  unsigned int inodeNum = getinode();
  if(inodeNum == 0){
    printf("Inodes are not avaiable.\n");
    return;
  }

  inode_type currentInode  = getCurInode();
  dir_type tmp;
  tmp.inode = inodeNum;
	strncpy(tmp.filename,targ,12);

	lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0] + 16*(2 + currentInode.nlinks)),SEEK_SET);
  write(fileDescriptor, &tmp, 16);
  currentInode.nlinks++;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
  write(fileDescriptor, &currentInode, INODE_SIZE);

  inode_type fin;
  fin.flags = inode_alloc_flag | dir_large_file | dir_access_rights;
  fin.nlinks = 0;
  fin.gid = 0;
  fin.uid = 0;
  fin.actime = 0;
  fin.modtime = 0;

  unsigned int buffer[BLOCK_SIZE/4];
  int ii;
  for (ii = 0; ii < BLOCK_SIZE/4; ii++) {
    buffer[ii] = 0;
  }
  unsigned int i = 0;
  unsigned int bytes_read = BLOCK_SIZE;
  unsigned int i1;
  while(bytes_read == BLOCK_SIZE){
    // storing data directly in first 10 blocks
 		if(i!=10){
      bytes_read = read(srcfd,buffer,BLOCK_SIZE);
      fin.size += bytes_read;
      unsigned int block_num = allocate_Data_Block();
		  fin.addr[i++] = block_num;
      writeAtBlock(block_num, buffer, bytes_read);
		} else {
      // data in 11th free_block stored as triple indirect blocks
      unsigned int block_num = allocate_Data_Block();
      fin.addr[i] = block_num;
      int k;
      for(k=0;k<BLOCK_SIZE/4;k++){
				unsigned int temp1=allocate_Data_Block();
				writeToBlockAtOffset(block_num,k*4,&temp1,4);
				int l;
				for(l=0;l<BLOCK_SIZE/4;l++)
				{
					unsigned int temp2 = allocate_Data_Block();
					writeToBlockAtOffset(temp1,l*4,&temp2,4);
					int m;
					for(m=0;m<BLOCK_SIZE/4;m++)
					{
						if(bytes_read == BLOCK_SIZE) {
						unsigned int temp3=allocate_Data_Block();
						writeToBlockAtOffset(temp2,m*4,&temp3,4);
						bytes_read = read(srcfd,buffer,BLOCK_SIZE);
                        fin.size += bytes_read;
						writeAtBlock(temp3, buffer, bytes_read);
						} else {
							k=BLOCK_SIZE/4;
							l=BLOCK_SIZE/4;
							m=BLOCK_SIZE/4;
						}
					}
				}
			}
    }
  }
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(inodeNum-1),SEEK_SET);
  write(fileDescriptor, &fin, INODE_SIZE);
  printf("Successfully copied from external to v6\n");
}

void readFromBlockOffset(unsigned int block_num, int offset, void * buffer, unsigned int nbytes){
  lseek(fileDescriptor,(BLOCK_SIZE * block_num) + offset, SEEK_SET);
  read(fileDescriptor,buffer,nbytes);
}

inode_type getCurInode() {
  inode_type currentInode;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
  read(fileDescriptor, &currentInode, INODE_SIZE);
  return currentInode;
}

inode_type getInodeFromNum(unsigned int num) {
  inode_type inode;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
  read(fileDescriptor, &inode, INODE_SIZE);
  return inode;
}

void writeToBlockAtOffset(unsigned int block_num, int offset, unsigned int buffer[], unsigned int nbytes){
  lseek(fileDescriptor,(BLOCK_SIZE * block_num) + offset, SEEK_SET);
  write(fileDescriptor,buffer,nbytes);
}

void writeAtBlock(unsigned int block_num, void * buffer, unsigned int bytes_read) {
  lseek(fileDescriptor, BLOCK_SIZE*block_num, SEEK_SET);
  write(fileDescriptor,buffer,bytes_read);
}

    // return true if the input represents file.
bool isFile(unsigned int num) {
  inode_type inode;
	lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
	read(fileDescriptor,&inode,INODE_SIZE);
  if ((inode.flags >> 12) & 1 && !((inode.flags >> 14) & 1)) {
    return true;
  }
  return false;
}

    // return true if the filedesprector is a directory
bool isDirectory(unsigned int num) {
  inode_type directoryInode;
	lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
	read(fileDescriptor,&directoryInode,INODE_SIZE);
  if ((directoryInode.flags >> 14) & 1) {
    return true;
  }
  return false;
}

bool inodeExists(unsigned int num) {
  inode_type directoryInode;
	lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
	read(fileDescriptor,&directoryInode,INODE_SIZE);
  if ((directoryInode.flags >> 15) & 1) {
    return true;
  }
  return false;
}

    // freeInodes are added
void addFreeInode(unsigned int iNumber){
        if(superBlock.ninode == I_SIZE)
                return;
        superBlock.inode[superBlock.ninode] = iNumber;
        superBlock.ninode++;
}

// File system initialization with the provided blocks and inodes
void initfs(char* path, unsigned int blocks, unsigned int inodes)
{
  printf("About to intialize filesystem...\n");
  unsigned int buffer[BLOCK_SIZE/4];
  int i;
  for (i = 0; i < BLOCK_SIZE/4; i++) {buffer[i] = 0;}
  int inodes_per_block= BLOCK_SIZE/INODE_SIZE; //number of inodes in each block 1024/64==16

  if((inodes%inodes_per_block) == 0){
    superBlock.isize = inodes/inodes_per_block;
  } else {
    superBlock.isize = (inodes/inodes_per_block) + 1;
  }
  superBlock.fsize = blocks;

  //create file for File System
  if((fileDescriptor = open(path,O_RDWR|O_CREAT,0600))== -1) {
    printf("File opening error [%s]\n",strerror(errno));
    return;
  }

  for (i=0; i < blocks; i++) {writeAtBlock(i+2,buffer,BLOCK_SIZE);}

  superBlock.nfree = 0;
  for (i= 2+superBlock.isize + 1; i< blocks; i++) {add_block_to_freeList(i,buffer);}

  superBlock.ninode = inodes;
  for (i = 0; i < inodes; i++) {superBlock.inode[i] = i + 1;}	//Initializing the inode array to inode numbers from 1 to 200

  superBlock.flock = 'f';
  superBlock.ilock = 'i';
  superBlock.fmod = 'f';
  superBlock.time[0] = 0;
  superBlock.time[1] = 0;

  writeAtBlock(1,&superBlock,BLOCK_SIZE);
  create_root_directory();
}

void add_block_to_freeList(unsigned int block_number,  unsigned int *empty_buffer){
  if ( superBlock.nfree == FREE_SIZE ) {
    unsigned int free_list_data[BLOCK_SIZE/4];
    int i;
    for (i=0;i<BLOCK_SIZE/4;i++){free_list_data[i] = 0;}
    free_list_data[0] = FREE_SIZE;
    for ( i = 0; i < FREE_SIZE; i++ ) {
         free_list_data[i + 1] = superBlock.free[i];
    }
    lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 ); //Repositions the file descriptor offset. Here offset is 1024 * block number
    write( fileDescriptor, free_list_data, BLOCK_SIZE ); // Writing free list to header block

    superBlock.nfree = 0;

  } else {

	  lseek( fileDescriptor, (block_number) * BLOCK_SIZE, 0 ); //Repositions the file descriptor offset. Here offset is 1024 * block number
    write( fileDescriptor, empty_buffer, BLOCK_SIZE );  // writing 0 to remaining data blocks to get rid of junk data
  }

  superBlock.free[superBlock.nfree] = block_number;  // Assigning blocks to free array
  ++superBlock.nfree;
}

void updatepwd(char *last) {
  if (strcmp(last, ".") == 0 ) {
    return;
  }
  if (strcmp(last, "..") == 0 ) {
    if(strcmp(presentWorkingDir, "/") == 0) {
      return;
    }
    int end;
    int c = 0;
    int i;
    for (i =strlen(presentWorkingDir)-1 ; i>=0; i--) {
      if (presentWorkingDir[i] == '/') {
        end = i;
        c++;
      }
      if (c == 2) {
        break;
      }
    }
    int j;
    for (j=0;j<=end;j++) {
      presentWorkingDir[j] = presentWorkingDir[j];
    }
    presentWorkingDir[j] = '\0';
  } else {
      strcat(presentWorkingDir, last);
      strcat(presentWorkingDir, "/");
  }
}

    // Root directory has been created
void create_root_directory(){
        int root_data_block = 2 + superBlock.isize;
        dir_type dir[2];
        dir[0].inode = 1;
        dir[0].filename[0] = '.';
        dir[0].filename[1] = '\0';
        dir[1].inode = 1;
        dir[1].filename[0] = '.';
        dir[1].filename[1] = '.';
        dir[1].filename[2] = '\0';
        writeAtBlock(root_data_block, dir, 2*sizeof(dir_type));

        inode_type root;
        root.flags = inode_alloc_flag | dir_flag | dir_large_file | dir_access_rights;
        root.nlinks = 0;
        root.uid = 0;
        root.gid = 0;
        root.size = INODE_SIZE;
        root.addr[0] = root_data_block;
        int i;
        for( i = 1; i < ADDR_SIZE; i++ ) {
          root.addr[i] = 0;
        }
        root.actime = 0;
        root.modtime = 0;

        lseek(fileDescriptor, 2 * BLOCK_SIZE, 0); //Repositions the file descriptor offset. Here offset is 2 * block size.
        write(fileDescriptor, &root, INODE_SIZE); //Writing inode to file system. Writes upto 64 bytes from the inode

        currentInodeNumber = 1;
        strcpy(presentWorkingDir,"/");
}

    
    // Directory has been created with the provided directoryName
void createDirectory(char *directoryName, unsigned int inodeNum) {
  inode_type currentInode;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
  read(fileDescriptor, &currentInode, INODE_SIZE);
  dir_type tmp;
  tmp.inode = inodeNum;
	strncpy(tmp.filename,directoryName,12);

	lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0] + 16*(2 + currentInode.nlinks)),SEEK_SET);
  write(fileDescriptor, &tmp, 16);
  currentInode.nlinks++;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
  write(fileDescriptor, &currentInode, INODE_SIZE);
  unsigned int data_block = allocate_Data_Block();
  int i;
  dir_type directoryEntry;
  directoryEntry.inode = inodeNum;   // root directory's inode number is 1.
  directoryEntry.filename[0] = '.';
  directoryEntry.filename[1] = '\0';

  inode_type directoryInode;
  directoryInode.flags = inode_alloc_flag | dir_flag | dir_large_file | dir_access_rights;   		// flag for root directory
  directoryInode.nlinks = 0; //Initializing the fields of inode struct
  directoryInode.uid = 0; //Initializing the fields of inode struct
  directoryInode.gid = 0; //Initializing the fields of inode struct
  directoryInode.size = INODE_SIZE; //Initializing the fields of inode struct
  directoryInode.addr[0] = data_block; //Initializing the fields of inode struct
  for(i = 1; i < ADDR_SIZE; i++ ) {
    directoryInode.addr[i] = 0;
  }
  directoryInode.actime = 0; //Initializing the fields of inode struct
  directoryInode.modtime = 0; //Initializing the fields of inode struct

	lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(inodeNum-1),SEEK_SET);
  write(fileDescriptor, &directoryInode, INODE_SIZE); //Writing inode to file system. Writes upto 64 bytes from the inode

  lseek(fileDescriptor, data_block * BLOCK_SIZE, 0); //Repositions the file descriptor offset. Here offset is the value of root data block.
  write(fileDescriptor, &directoryEntry, 16); //Writing the root to root data block on the filesystem

  directoryEntry.inode = currentInodeNumber;
  directoryEntry.filename[0] = '.';
  directoryEntry.filename[1] = '.';
  directoryEntry.filename[2] = '\0';
  write(fileDescriptor, &directoryEntry, 16);
}

unsigned int getinode() {
  if (superBlock.ninode == 1) {
    return 0;
  }
  superBlock.ninode--;
  return superBlock.inode[superBlock.ninode];
}

void deallocateInodeFile(unsigned int num) {
  inode_type finode;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
	read(fileDescriptor,&finode,INODE_SIZE);
  superBlock.inode[superBlock.ninode] = num;
  superBlock.ninode++;
  finode.flags =  0;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
  write(fileDescriptor, &finode, INODE_SIZE);
}

void deallocateInodeDirectory(unsigned int num) {
  inode_type directoryInode;
  lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
	read(fileDescriptor,&directoryInode,INODE_SIZE);
  if(directoryInode.nlinks == 0) {
    superBlock.inode[superBlock.ninode] = num;
    superBlock.ninode++;
    directoryInode.flags =  0;
    unsigned int buffer[BLOCK_SIZE/4];
    int i;
    for (i = 0; i < BLOCK_SIZE/4; i++) {buffer[i] = 0;}
    add_block_to_freeList(directoryInode.addr[0], buffer);
    lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(num-1),SEEK_SET);
    write(fileDescriptor, &directoryInode, INODE_SIZE);
  } else {
    printf ("Error dir has files\n");
  }
}

unsigned int getInodeFromFilename(unsigned int inodeNum, const char* filename){
	inode_type directoryInode;
	dir_type directoryEntry;
	lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(inodeNum-1),SEEK_SET);
	read(fileDescriptor,&directoryInode,INODE_SIZE);
	lseek(fileDescriptor,(BLOCK_SIZE*directoryInode.addr[0]),SEEK_SET);
  unsigned int i;
  for(i=0;i<directoryInode.nlinks + 2; i++){
		read(fileDescriptor,&directoryEntry,sizeof(directoryEntry));
		if(strcmp(filename,directoryEntry.filename) == 0) {
			return directoryEntry.inode;
    }
	}
	return 0;
}

unsigned int allocate_Data_Block(){
  if (superBlock.nfree == 0) {
    unsigned int free_block = superBlock.free[superBlock.nfree];
    lseek(fileDescriptor,free_block*BLOCK_SIZE,0);
    unsigned int freelist[BLOCK_SIZE/4];
    read(fileDescriptor, &freelist, BLOCK_SIZE);
    superBlock.nfree = freelist[0];
    int n;
    for(n=0; n<FREE_SIZE; n++){
      superBlock.free[n] = freelist[n+1];
    }
    return free_block;
  }
  superBlock.nfree--;
  unsigned int free_block = superBlock.free[superBlock.nfree];
  if (superBlock.nfree == 0) {
    lseek(fileDescriptor,free_block*BLOCK_SIZE,0);
    unsigned int freelist[BLOCK_SIZE/4];
    read(fileDescriptor, &freelist, BLOCK_SIZE);
    superBlock.nfree = freelist[0];
    int n;
    for(n=0; n<FREE_SIZE; n++){
      superBlock.free[n] = freelist[n+1];
    }
  }
  return free_block;
}

void remove_Directory(const char *directoryName){
    unsigned int dirinodenum = getInodeFromFilename(currentInodeNumber,directoryName);
    if (dirinodenum == 0) {
        printf("No such directory.\n");
    } else {
        if (inodeExists(dirinodenum)) {
            if (isDirectory(dirinodenum)) {
                deallocateInodeDirectory(dirinodenum);
                inode_type currentInode;
                lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
                read(fileDescriptor, &currentInode, INODE_SIZE);
                if (currentInode.nlinks == 1) {
                    lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * 2, SEEK_SET);
                    write(fileDescriptor, zeroDirType, sizeof(dir_type));
                } else {
                    int deleteinode;
                    int lastInode = currentInode.nlinks + 1;
                    dir_type directoryEntry;
                    lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]),SEEK_SET);
                    int i;
                    for(i=0;i<=lastInode; i++){
                        read(fileDescriptor,&directoryEntry,sizeof(directoryEntry));
                        if(strcmp(directoryName,directoryEntry.filename) == 0) {
                            deleteinode = i;
                            break;
                        }
                    }
                    if (deleteinode == lastInode) {
                        lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * deleteinode, SEEK_SET);
                        write(fileDescriptor, zeroDirType, sizeof(dir_type));
                    } else {
                        dir_type entry;
                        lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * lastInode, SEEK_SET);
                        read(fileDescriptor,&entry,sizeof(dir_type));
                        lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * lastInode, SEEK_SET);
                        write(fileDescriptor, zeroDirType, sizeof(dir_type));
                        lseek(fileDescriptor,(BLOCK_SIZE*currentInode.addr[0]) + sizeof(dir_type) * deleteinode, SEEK_SET);
                        write(fileDescriptor, &entry, sizeof(dir_type));
                    }
                }
                currentInode.nlinks--;
                lseek(fileDescriptor,(BLOCK_SIZE*2)+INODE_SIZE*(currentInodeNumber-1),SEEK_SET);
                write(fileDescriptor, &currentInode, INODE_SIZE);
                printf("Directory Removed!\n");
            } else {
                printf("Please input a directory directoryName\n");
            }
        } else {
            printf("No such directory.\n");
        }
    }
}

void change_Directory(const char *newDirectoryName){
    if(!newDirectoryName){
        printf("Not a directory\n");
    }else{
        unsigned int dirinodenumber = getInodeFromFilename(currentInodeNumber,newDirectoryName);
        if (dirinodenumber == 0) {
            printf("No such directory exists.\n");
        } else {
            if (isDirectory(dirinodenumber)) {
                currentInodeNumber = dirinodenumber;
                updatepwd(newDirectoryName);
            }
            else {
                printf("Given directoryName is a file\n");
            }
        }
    }
}

void preInitialize(){
    char *n1, *n2;  //Char pointers n1 and n2 are defined.
    unsigned int numBlocks = 0, numInodes = 0; //Initializing the number of blocks and number of inodes to 0
    char *filepath;
    filepath = strtok(NULL, " ");  //passing NULL tells the compiler to use the last sent string and return the next token which is the file path.
    n1 = strtok(NULL, " "); //passing NULL tells the compiler to use the last sent string and return the next token.
    n2 = strtok(NULL, " "); //passing NULL tells the compiler to use the last sent string and return the next token.
    
        //If the file is not found, throws error and exit the program
    if(access(filepath, F_OK) != -1) {
        printf("filesystem already exists. \n");
        printf("same file system will be used\n");
        fileDescriptor = open(filepath,O_RDWR);
        if(fileDescriptor == -1){ //Opening the file so that in can be read from and write to. If the return value is -1, there is an error in opening the file
            printf("\n filesystem already exists but open() failed with error [%s]\n", strerror(errno));//prints the error message associated with errno
        } else {
            lseek(fileDescriptor,BLOCK_SIZE, SEEK_SET);
            read(fileDescriptor,&superBlock,BLOCK_SIZE);
            filesystemExists = true;
            currentInodeNumber = 1;
            strcpy(presentWorkingDir,"/");
        }
    } else {
        if (!n1 || !n2) //if any missing arguments, it is notifed to the user.
            printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");
        else {
            numBlocks = atoi(n1);   //Convert the read character to integer.
            numInodes = atoi(n2);   //Convert the read character to interger.
            initfs(filepath, numBlocks, numInodes ); //initfs function call. If the return value is 1, the file system is initialized
            printf("The file system is initialized\n");
            filesystemExists = true;
        }
    }
}

void help(){
    printf("******List of commands that work******\n");
    printf("\tcpin  <extsourcefilepath> <v6destfilepath>\n");
    printf("\tcpout  <v6sourcefilepath> <extdestfilepath>\n");
    printf("\tmkdir  <directoryName>\n");
    printf("\trm  <filename>\n");
    printf("\tls\n");
    printf("\tcd <directorypath>\n");
    printf("\trmdir <directoryName>\n");
    printf("\topen <filesystemfileName>\n");
    printf("\thelp\n");
    printf("\tq\n");
}
