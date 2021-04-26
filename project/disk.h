using namespace std;

class Disk {
public:

    int diskSize;
    int blkSize;
    int blkCount;
    char *currfilename;
    char *copiedfilename; 

    Disk(int sz, char* rname);
    ~Disk();
    int initDisk();
    int copyDiskfile();
    int readDiskBlock(int blknum, char* blkdata);
    int writeDiskBlock(int blknum, char* blkdata);
    int detectfileError(int errcode, int blknum, char* blkdata);
    int getBlockSize() { return (blkSize); };
    int getBlockCount() { return (blkCount); };
};
