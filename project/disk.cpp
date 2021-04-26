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
    currfilename = strdup(rname);
}

Disk::~Disk()
{
}

int Disk::initDisk()
{
    fstream f(currfilename, ios::in);
    if (!f) {
        f.open(currfilename, ios::out);
        if (!f) {
            cerr << "Error: Cannot create disk file" << endl;
            return(-1);
        }
        for (int i = 0; i < diskSize; i++) f.put('#');
        f.close();
        return(1);
    }
    f.close();
    return 0;
}

int Disk::copyDiskfile()
{
    ifstream currf(currfilename, ios::in);
    fstream copyf(copiedfilename, ios::out);
    if (!currf || !copyf) return (-1);
    copyf << currf.rdbuf();
    currf.close();
    copyf.close();
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
    ifstream currf(currfilename, ios::in);
    ifstream copyf(copiedfilename, ios::in);
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
    fstream currf(currfilename, ios::in | ios::out);
    fstream copyf(copiedfilename, ios::in | ios::out);
    if (!currf || !copyf) return(-1);
    currf.seekg(blknum * blkSize);
    copyf.seekg(blknum * blkSize);
    currf.write(blkdata, blkSize);
    copyf.write(blkdata, blkSize);
    currf.close();
    copyf.close();
    return(0);
}

int Disk::detectfileError(int errcode, int blknum, char* blkdata)
{
    ifstream copyf(copiedfilename, ios::in);
    fstream currf(currfilename, ios::in | ios::out);
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


