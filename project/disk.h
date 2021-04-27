using namespace std; 

class Disk {
  public:

  int diskSize;
  int blkSize;
  int blkCount;
  char *currFileName;
  char *copyFileName;

    Disk(int sz, char *fname);
    ~Disk();
    int initDisk();
    int copyDiskFile();
    int readDiskBlock(int blknum, char *blkdata);
    int writeDiskBlock(int blknum, char *blkdata);
    int getBlockSize() {return (blkSize);};
    int getBlockCount() {return (blkCount);};
};


