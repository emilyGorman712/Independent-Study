#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "bitvector.h"
#include <iostream>
#include <unistd.h>
using namespace std;
void prtbv(unsigned int* bv, int length) {     //printing function, purely for testing

    for (int i = 0; i < length; i++) {
        if (*bv & (1 << i)) {
            cout << "1";
        }
        else {
            cout << "0";
        }
    }
    cout << "\n";

}

PartitionManager::PartitionManager(DiskManager* dm, char partitionname, int partitionsize)
{
    myDM = dm;
    int x;
    myPartitionName = partitionname;
    myPartitionSize = myDM->getPartitionSize(myPartitionName);
    BitVector* dmBV = new BitVector(myPartitionSize);

    char tempbuffer[4096];
    for (int j = 0; j < 4096; j++) {
        tempbuffer[j] = 'c';
    }

    myDM->readDiskBlock(myPartitionName, 0, tempbuffer);
    if (tempbuffer[0] == '#') {//bitvector has not been set yet, so set it
        dmBV->setBit(0);                                           //first and second bits of vector are set, 1 is set for root dir
        dmBV->getBitVector((unsigned int*)tempbuffer);
        x = myDM->writeDiskBlock(myPartitionName, 0, tempbuffer);      //bit vector written to first block of partition
        if (x != 0) {
            cout << "Error" << endl;
        }
    }
}

PartitionManager::~PartitionManager()
{
}

/*
 * return blocknum, -1 otherwise
 */
int PartitionManager::getFreeDiskBlock()
{
    /* write the code for allocating a partition block */
    BitVector* inBV = new BitVector(myPartitionSize);     //bit vector to read in
    int x;
    char buffer[4096];  //initialize the array with c's, so you don't get garbage memory in the array.
    for (int j = 0; j < 4096; j++) {
        buffer[j] = 'c';
    }

    myDM->readDiskBlock(myPartitionName, 0, buffer);       //reads in bit vector from first block in partition
    inBV->setBitVector((unsigned int*)buffer);
    for (int i = 0; i < myPartitionSize; i++) {
        if (inBV->testBit(i) == 0) {

            return i;
        }
    }

    return -1;
}

/*
 * return 0 for sucess, -1 otherwise
 */
int PartitionManager::returnDiskBlock(int blknum)
{
    /* write the code for deallocating a partition block */
    BitVector* inBV = new BitVector(myPartitionSize);         //bit vector to read in
    int x;
    char buffer[4096];  //initialize the array with c's, so you don't get garbage memory in the array.
    for (int j = 0; j < 4096; j++) {
        buffer[j] = 'c';
    }

    myDM->readDiskBlock(myPartitionName, 0, buffer);           //reads in bit vector from first block of partition
    inBV->setBitVector((unsigned int*)buffer);
    inBV->resetBit(blknum);                                    //resets the block number specified
    inBV->getBitVector((unsigned int*)buffer);
    x = myDM->writeDiskBlock(myPartitionName, 0, buffer);      //writes new bit vector back to partition block 0
    if (x != 0) {
        cout << "Check partman " << endl;
        sleep(100);
        return x;
    }
    else {
        //now need to actually delete addr
        for (int j = 0; j < 4096; j++) {
            buffer[j] = '#';
        }
        return myDM->writeDiskBlock(myPartitionName, blknum, buffer);
    }

}


int PartitionManager::readDiskBlock(int blknum, char* blkdata)
{
    return myDM->readDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::writeDiskBlock(int blknum, char* blkdata)
{
    BitVector* inBV = new BitVector(myPartitionSize);
    int x;
    char buffer[4096];
    for (int j = 0; j < 4096; j++) {
        buffer[j] = 'c';
    }
    myDM->readDiskBlock(myPartitionName, 0, buffer);
    inBV->setBitVector((unsigned int*)buffer);
    inBV->setBit(blknum);
    inBV->getBitVector((unsigned int*)buffer);
    x = myDM->writeDiskBlock(myPartitionName, 0, buffer);
    return myDM->writeDiskBlock(myPartitionName, blknum, blkdata);
}

int PartitionManager::getBlockSize()
{
    return myDM->getBlockSize();
}
