#include "disk.h"
#include "diskmanager.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
using namespace std;

//use this to read integers out of buffer
int DiskManager::charToInt(int pos, char* buff) {
    char temp[5];
    for (int i = 0; i < 4; i++) {
        temp[i] = buff[pos + i];
    }

    temp[4] = '\0';

    int numb = atoi(temp);
    return numb;

}

//use this to put integers in a buffer
void DiskManager::intToChar(int pos, int num, char* buff) {
    char tempb[4];
    //sprintf(tempb, "%04d", num);

    for (int i = 0; i < 4; i++) {
        buff[pos + i] = tempb[i];
    }

}


DiskManager::DiskManager(Disk* d, int partcount, DiskPartition* dp)
{
    myDisk = d;
    partCount = partcount;
    int r = myDisk->initDisk();
    int r2;
    char buffer[4096];

    r2 = myDisk->readDiskBlock(0, buffer);

    if (r == 1) {
        cout << "no disk file found, creating new one... " << endl;
        cout << "creating superblock..." << endl;

        diskP = dp;
        setSuperBlock();
        diskP = new DiskPartition[partCount];
    }
    else {
        cout << "disk file found, checking superblock... " << endl;
        diskP = dp;
        if (buffer[0] == '#') {
            cout << "no superblock found, creating superblock..." << endl;
            setSuperBlock();

        }
        else {
            cout << "superblock found, reading info from it..." << endl;
            int i = 0;
            int position = 0;
            do {
                position += 3;
                sBlock[i].pName = buffer[position];
                position++;
                sBlock[i].pStart = charToInt(position, buffer);
                position += 4;
                sBlock[i].pEnd = charToInt(position, buffer);
                position += 4;
                i++;

            } while (buffer[position] != '#');
        }
    }

}

/*
 *   returns:
 *   0, if the block is successfully read;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds; (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::readDiskBlock(char partitionname, int blknum, char* blkdata)
{
    // Return -3 if partition doesn't exist
    bool pExists = false;
    // pos keeps track of which partition we're looking at in sBlock
    int pos = 0;
    for (pos; pos < partCount; pos++) {
        if (sBlock[pos].pName == partitionname) {
            pExists = true;
            break;
        }
    }
    if (pExists == false) return -3;

    /*
    // Return -2 if blknum is out of bounds
    if ((blknum < sBlock[pos].pStart) || (blknum >= getPartitionSize(partitionname))) return -2;

    // Return -1 if disk can't be opened
    ifstream f(myDisk->diskFilename, ios::in);
    if (!f) return -1;

    // Disk can be accessed. Read from it. Close it. Return 0 when done.
    */

    int diskpos = blknum + sBlock[pos].pStart;

    int retval = myDisk->readDiskBlock(diskpos, blkdata);

    return retval;
}


/*
 *   returns:
 *   0, if the block is successfully written;
 *  -1, if disk can't be opened; (same as disk)
 *  -2, if blknum is out of bounds;  (same as disk)
 *  -3 if partition doesn't exist
 */
int DiskManager::writeDiskBlock(char partitionname, int blknum, char* blkdata)
{
      // Return -3 if partition doesn't exist
    bool pExists = false;
    int pos = 0;
    for (pos; pos < partCount; pos++) {
        if (sBlock[pos].pName == partitionname) {
            pExists = true;
            break;
        }
    }
    if (pExists == false) return -3;

    /*
    // Return -2 if blknum is out of bounds
    if ((blknum < sBlock[pos].pStart) || (blknum >= getPartitionSize(partitionname))) return -2;

    // Return -1 if disk can't be opened
    fstream f(myDisk->diskFilename, ios::in|ios::out);
    if (!f) return -1;

    // Disk can be accessed. Write to it. Close it. Return 0 when done.
    */

    int diskpos = blknum + sBlock[pos].pStart;

    int retval = myDisk->writeDiskBlock(diskpos, blkdata);

    return retval;
}

/*
 * return size of partition
 * -1 if partition doesn't exist.
 */
int DiskManager::getPartitionSize(char partitionname)
{
    // Search the disk partitions. Return size of requested partition if found.
    for (int i = 0; i < partCount; i++) {
        if (sBlock[i].pName == partitionname) {
            return (sBlock[i].pEnd - sBlock[i].pStart);
        }
    }

    // Requested partition not found. Return -1.
    return -1;
}

//helper functions

void DiskManager::setSuperBlock() {
    int starter = 1;
    char buffer[4096];
    int r;

    for (int j = 0; j < 4096; j++) buffer[j] = '#';

    int position = 0;

    for (int i = 0; i < partCount; i++) {
        sBlock[i].pName = diskP[i].partitionName;
        sBlock[i].pStart = starter;
        starter += diskP[i].partitionSize;
        sBlock[i].pEnd = starter; //this should be 1 + 100, so 101
        starter++; //this should move starter up one for the next partition to start at

        for (int j = 0; j < 3; j++) {
            buffer[position] = '0';
            position++;
        }

        buffer[position] = sBlock[i].pName;
        position++;

        intToChar(position, sBlock[i].pStart, buffer);
        position += 4;
        intToChar(position, sBlock[i].pEnd, buffer);
        position += 4;

    }

    r = myDisk->writeDiskBlock(0, buffer);
}
