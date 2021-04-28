#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include "disk.h"

using namespace std;

Disk::Disk(int numblocks, char *fname)
{
  blkCount = numblocks;
  blkSize = 4096;
  diskSize = numblocks * blkSize;
  currFileName = strdup(fname);
  copyFileName = strdup(fname);
}

Disk::~Disk()
{
}

int Disk::initDisk()
{
  fstream currf(currFileName, ios::in);
  fstream copyf(copyFileName, ios::in);
  if (!currf || !copyf) {
    currf.open(currFileName, ios::out);
    copyf.open(copyFileName, ios::out);
    if (!currf || !copyf) {
      cerr << "Error: Cannot create disk file" << endl;
      return(-1);
    }
    for (int i = 0; i < diskSize; i++) currf.put('#');
    for (int i = 0; i < diskSize; i++) copyf.put('#');
    currf.close();
    copyf.close();
    return(1);
  }
  currf.close();
  copyf.close();
  return (0);
}

int Disk::copyDiskFile() {
  ifstream currf(currFileName, ios::in);
  fstream copyf(copyFileName, ios::out);
  if (!currf || !copyf) return(-1);
  copyf << currf.rdbuf();
  currf.close();
  copyf.close();
  return (0);
}

int Disk::readDiskBlock(int blknum, char *blkdata)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the block is successfully read;
*/
{
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  ifstream currf(currFileName, ios::in);
  ifstream copyf(copyFileName, ios::in);
  if (!currf || !copyf) return(-1);
  currf.seekg(blknum * blkSize);
  copyf.seekg(blknum * blkSize);
  currf.read(blkdata, blkSize);
  copyf.read(blkdata, blkSize);
  currf.close();
  copyf.close();
  return(0);
}

int Disk::writeDiskBlock(int blknum, char *blkdata)
/*
  returns -1, if DISK can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the block is successfully read;
*/
{
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  fstream currf(currFileName, ios::in|ios::out);
  fstream copyf(copyFileName, ios::in|ios::out);
  if (!currf || !copyf) return(-1);
  currf.seekg(blknum * blkSize);
  copyf.seekg(blknum * blkSize);
  currf.write(blkdata, blkSize);
  copyf.write(blkdata, blkSize);
  currf.close();
  copyf.close();
  return(0);
}
