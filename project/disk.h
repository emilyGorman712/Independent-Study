#include <vector>
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
    int getBlockCount() {return (blkCount);}
    int addTuple(int x, int y, int blknum);
    int deleteTuple(int x, int y, int blknum);
    int modifyTuple(int x, int y, int newx, int newy, int blknum);
    int findTuple(int x, int y, int blknum);
    tuple<int, int> makeSelection(vector<tuple<int, int>> r, int row);
    vector<int> makeProjection(vector<tuple<int, int>> r, int column);
    void makeJoin(vector<tuple<int, int>> r1, vector<tuple<int, int>> r2);

};


