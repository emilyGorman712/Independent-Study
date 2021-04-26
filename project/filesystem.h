
class fileLO {
public:
    char* filename;
    int lock; //0 for unlocked, some unique number otherwise
    int open; //1 for open 0 for closed
    int rw; //rw pointer 
    char mode; //r, w, m    for read, write, both
};

class FileSystem {
    DiskManager* myDM;
    PartitionManager* myPM;
    char myfileSystemName;
    int myfileSystemSize;
    int locker;

    fileLO fileTable[1000];
    int Gaddrarr[19];
    int GrootDirAddr;

public:
    FileSystem(DiskManager* dm, char fileSystemName);
    int createFile(char* filename, int fnameLen);
    int createDirectory(char* dirname, int dnameLen);
    int lockFile(char* filename, int fnameLen);
    int unlockFile(char* filename, int fnameLen, int lockId);
    int deleteFile(char* filename, int fnameLen);
    int deleteDirectory(char* dirname, int dnameLen);
    int openFile(char* filename, int fnameLen, char mode, int lockId);
    int closeFile(int fileDesc);
    int readFile(int fileDesc, char* data, int len);
    int writeFile(int fileDesc, char* data, int len);
    int appendFile(int fileDesc, char* data, int len);
    int seekFile(int fileDesc, int offset, int flag);
    int renameFile(char* filename1, int fnameLen1, char* filename2, int fnameLen2);
    int getAttribute(char* filename, int fnameLen, char* girth);
    int setAttribute(char* filename, int fnameLen, char girth);
    int setAttribute2(char* filename, int fnameLen, int shaft);
    int getAttribute2(char* filename, int fnameLen);

    int filefound(char* filename, int len);
    int getaddrs(char* inodeBuf);
    int getsubdir(char* filename, int len);
    int rollbackRoot(char* filename, int fnameLen);
};

