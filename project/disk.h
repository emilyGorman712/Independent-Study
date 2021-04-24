using namespace std;

class Disk {
public:

    int diskSize;
    int blkSize;
    int blkCount;
    char *currrecordname;
    char *copiedrecordname; 

    Disk(int sz, char* rname);
    ~Disk();
    int initDisk();
    int copyDiskrecord();
    int readDiskBlock(int blknum, char* blkdata);
    int writeDiskBlock(int blknum, char* blkdata);
    int detectrecordError(int errcode, int blknum, char* blkdata);
    int getBlockSize() { return (blkSize); };
    int getBlockCount() { return (blkCount); };
};
