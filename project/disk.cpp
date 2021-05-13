#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <vector>
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

int Disk::addTuple(int x, int y, int blknum)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the tuple is successfully added;
*/
{
  string tupleDesc = "(" + to_string(x) + "," + to_string(y) + ")";
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  fstream currf(currFileName, ios::in|ios::out);
  fstream copyf(copyFileName, ios::in|ios::out);
  if (!currf || !copyf) return(-1);
  currf.seekg(blknum * blkSize);
  copyf.seekg(blknum * blkSize);
  currf << tupleDesc;
  copyf << tupleDesc;
  currf.close();
  copyf.close();
  return(0);
}
    
int Disk::deleteTuple(int x, int y, int blknum)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the tuple is successfully deleted;
*/
{
  string tupleDesc = "(" + to_string(x) + "," + to_string(y) + ")";
  string tempString;
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  fstream currf(currFileName, ios::in|ios::out);
  fstream copyf(copyFileName, ios::in|ios::out);
  if (!currf || !copyf) return(-1);
  currf.seekg(blknum * blkSize);
  copyf.seekg(blknum * blkSize);
  while (getline(currf, tempString))
  {
    tempString.replace(tempString.find(tempString), tempString.length(), "");
  }
  while (getline(copyf, tempString))
  {
    tempString.replace(tempString.find(tempString), tempString.length(), "");
  }
  currf.close();
  copyf.close();
  return (0);
}

int Disk::modifyTuple(int x, int y, int newx, int newy, int blknum)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns 0, if the tuple is successfully modified;
*/
{
  string tupleDesc = "(" + to_string(x) + "," + to_string(y) + ")";
  string newTupleDesc = "(" + to_string(newx) + "," + to_string(newy) + ")";
  string tempString;
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  fstream currf(currFileName, ios::in|ios::out);
  fstream copyf(copyFileName, ios::in|ios::out);
  if (!currf || !copyf) return(-1);
  currf.seekg(blknum * blkSize);
  copyf.seekg(blknum * blkSize);
  while (getline(currf, tempString))
  {
    tempString.replace(tempString.find(tupleDesc), newTupleDesc.length(), newTupleDesc);
  }
  while (getline(copyf, tempString))
  {
    tempString.replace(tempString.find(tupleDesc), newTupleDesc.length(), newTupleDesc);
  }
  currf.close();
  copyf.close();
  return (0);
}
    
int Disk::findTuple(int x, int y, int blknum)
/*
  returns -1, if disk can't be opened;
  returns -2, if blknum is out of bounds;
  returns -3, if tuple is not found;
  returns 0, if the tuple is successfully found;
*/
{
  string tupleDesc = "(" + to_string(x) + "," + to_string(y) + ")";
  bool found = false;
  string tempString;
  if ((blknum < 0) || (blknum >= blkCount)) return(-2);
  ifstream currf(currFileName, ios::in);
  ifstream copyf(copyFileName, ios::in);
  if (!currf || !copyf) return(-1);
  currf.seekg(blknum * blkSize);
  copyf.seekg(blknum * blkSize);
  while (currf >> tempString)
  {
    if (tempString == tupleDesc)
    {
      bool found = true;
    }
  }
  while (copyf >> tempString)
  {
    if (tempString == tupleDesc)
    {
      bool found = true;
    }
  }
  currf.close();
  copyf.close();
  if (found == true) return (0);
  return(-3);
}
    
tuple<int, int> Disk::makeSelection(vector<tuple<int, int>> r, int row)
//if successful selection is returned
{
  tuple<int, int> tup = r.at(row);
  return tup;
}
    
vector<int> Disk::makeProjection(vector<tuple<int, int>> r, int column)
//if successful join is returned
{
  vector<int> v;
  int element;
  bool duplicate = false;
  vector<int>::iterator i;
  if (column == 0)
  {
    for (tuple<int, int> tup : r) 
    {
      element = get<0>(tup);
      for (int tmpElement : v) 
      {
        if (tmpElement == element)
        {
          duplicate = true;
        }
      }
      if (duplicate == false) {
        v.push_back(element);
      }
      duplicate = false;
    }
  }
  if (column == 1)
  {
    for (tuple<int, int> tup : r) 
    {
      element = get<1>(tup);
      for (int tmpElement : v) 
      {
        if (tmpElement == element)
        {
          duplicate = true;
        }
      }
      if (duplicate == false) {
        v.push_back(element);
      }
      duplicate = false;
    }
  }
  return v;
}
    
void Disk::makeJoin(vector<tuple<int, int>> r1, vector<tuple<int, int>> r2)
{
  for (tuple<int, int> tupA : r1)
  {
    for (tuple<int, int> tupB : r2)
    {
      tuple<tuple<int, int>, tuple<int, int>> join(tupA, tupB);
    }
  }
}