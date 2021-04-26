using namespace std;

class DiskPartition {
public:
    char partitionName;
    int partitionSize;
};

class SuperBlock {
public:
    char pName;
    int pStart;
    int pEnd;
};

class DiskManager {
    Disk* myDisk;
    int partCount;
    DiskPartition* diskP;

    SuperBlock sBlock[3];

public:
    DiskManager(Disk* d, int partCount, DiskPartition* dp);
    ~DiskManager();
    int readDiskBlock(char partitionname, int blknum, char* blkdata);
    int writeDiskBlock(char partitionname, int blknum, char* blkdata);
    int getBlockSize() { return myDisk->getBlockSize(); };
    int getPartitionSize(char partitionname);
    void setSuperBlock();
    int charToInt(int pos, char* buff);
    void intToChar(int pos, int num, char* buff);
};


