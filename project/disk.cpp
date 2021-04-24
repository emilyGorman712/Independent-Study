#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include "disk.h"

using namespace std;

Disk::Disk(int numblocks, char* rname)
{
    blkCount = numblocks;
    blkSize = 4096;
    diskSize = numblocks * blkSize;
    currrecordname = strdup(rname);
}

Disk::~Disk()
{
}

int Disk::initDisk()
{
    fstream f(currrecordname, ios::in);
    if (!f) {
        f.open(currrecordname, ios::out);
        if (!f) {
            cerr << "Error: Cannot create disk record" << endl;
            return(-1);
        }
        for (int i = 0; i < diskSize; i++) f.put('#');
        f.close();
        return(1);
    }
    f.close();
    return 0;
}

int Disk::copyDiskrecord()
{
    ifstream currf(currrecordname, ios::in);
    fstream copyf(copiedrecordname, ios::out);
    if (!currf || !copyf) return (-1);
    copyf << currf.rdbuf();
    currf.close();
    copyf.close() :
    return(0);
}

int Disk::readDiskBlock(int blknum, char* blkdata)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the block is successfully read;
*/
{
    if ((blknum < 0) || (blknum >= blkCount)) return(-2);
    ifstream currf(currrecordname, ios::in);
    ifstream copyf(copiedrecordname, ios::in);
    if (!currf || !copyf) return(-1);
    currf.seekg(blknum * blkSize);
    copyf.seekg(blknum * blkSize);
    currf.read(blkdata, blkSize);
    copyf.read(blkdata, blkSize);
    currf.close();
    copyf.close();
    return(0);
}

int Disk::writeDiskBlock(int blknum, char* blkdata)
/*
  returns -1, if DISK can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the block is successfully read;
*/
{
    if ((blknum < 0) || (blknum >= blkCount)) return(-2);
    fstream currf(currrecordname, ios::in | ios::out);
    fstream copyf(copiedrecordname, ios::in | ios::out);
    if (!currf || !copyf) return(-1);
    currf.seekg(blknum * blkSize);
    copyf.seekg(blknum * blkSize);
    currf.write(blkdata, blkSize);
    copyf.write(blkdata, blkSize);
    currf.close();
    copyf.close();
    return(0);
}

int Disk::detectrecordError(int errcode, int blknum, char* blkdata)
{
    ifstream copyf(copiedrecordname, ios::in);
    fstream currf(currrecordname, ios::in | ios::out);
    if (errcode != 0) {
        copyf.seekg(blknum * blkSize);
        currf.seekg(blknum * blkSize);
        copyf.read(blkdata, blkSize);
        currf.write(blkdata, blkSize);
    }
    currf.close();
    copyf.close();
    return(0);
}


