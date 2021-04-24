#include "disk.h"
#include "recordmanagement.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
using namespace std;

int Gaddrarr[19];
int GrootDirAddr = 0;

//given record inode, puts all used blocks into Gaddrarr global val
//returns -1 if no indir block
//if indir exists, returns blockaddr of it
int RecordManagement::getaddrs(char* inodeBuf) {
	for (int j = 0; j < 19; j++) Gaddrarr[j] = -1; // fill with -1s
	int pos = 6;
	int i = 0;
	char buff[64];
	int x = -1;
	for (int j = 0; j < 64; j++) buff[j] = '#';

	while (inodeBuf[pos] != '#' && pos < 18) {
		Gaddrarr[i] = myDM->charToInt(pos, inodeBuf);
		pos += 4;
		i++;
	}


	if (inodeBuf[pos] != '#') { //if the record has an indir block, set it to 
		x = myDM->charToInt(pos, inodeBuf);
		//x is now addr block of the indir
		int r = myDM->readDiskBlock(myrecordManagementName, x, buff);
		//buff should be indir node
		pos = 0; // start at 0th pos of indir 
		while (buff[pos] != '#' && pos < 64) {
			Gaddrarr[i] = myDM->charToInt(pos, buff);
			pos += 4;
			i++;
		}
	}
	return x;
};

//-1 if DNE
//-2 if Dir already there
//record block addr if found
int RecordManagement::recordfound(char* recordname, int len) {
	int subdir = getsubdir(recordname, len); //working for at least single records 
	if (subdir == -4) return -1;
	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	int i = 1;
	char curdirname;
	char rname = recordname[len - 1]; //record we're looking for. should already be in the correct dir

	int r = myDM->readDiskBlock(myrecordManagementName, subdir, rootbuf);
	
	int bufpos = 0;

	//this loop will get the addr of the next dir and put it in rootbuf
	while (true) {//loop starting at Rood dir, and go through all the way
		if (rootbuf[bufpos] == '#') return -1;
		if (rootbuf[bufpos] == rname && rootbuf[bufpos + 5] == 'F') {
			int xy = myDM->charToInt(bufpos + 1, rootbuf);
			return xy;
		}
		else if (rootbuf[bufpos] == rname && rootbuf[bufpos + 5] == 'D') {
			//then we found a directory of that name...
			return -2;
		}
		bufpos += 6;
		if (bufpos >= 60) {
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
			bufpos = 0;
		}
	}
}

//return addr of lowest subdir, 2 less than end
//-4 if path DNE
//assumes /char/char/char validdate b4 sending here
int RecordManagement::getsubdir(char* recordname, int len) {
	if (len == 2) return GrootDirAddr; //just root directory addr
	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	int i = 1;
	char curdirname;

	int r = myDM->readDiskBlock(myrecordManagementName, GrootDirAddr, rootbuf);
	while (i < (len)) {
		curdirname = recordname[i];
		int bufpos = 0;

		//this loop will get the addr of the next dir and put it in rootbuf
		while (true) {//loop starting at Rood dir, and go through all the way
			if (rootbuf[bufpos] == '#') return -4;
			if (rootbuf[bufpos] == curdirname && rootbuf[bufpos + 5] == 'D') {
				int xy = myDM->charToInt(bufpos + 1, rootbuf);
				r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
				bufpos = 0;
				if (i == (len - 3)) {
					return xy;
				}
				else {//need to cd down a directory
					break;
				}
			}

			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				int xy = myDM->charToInt(bufpos, rootbuf);
				r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
				bufpos = 0;
			}
			else bufpos += 6;
		}
		i += 2;
	}

}

RecordManagement::RecordManagement(DiskManager *dm, char recordManagementName)
{
	myDM = dm;
	myrecordManagementName = recordManagementName;
	myrecordManagementSize = myDM->getPartitionSize(recordManagementName);
	myPM = new PartitionManager(dm, recordManagementName, myrecordManagementSize);
	locker = 1;

	for (int i = 0; i < 1000; i++) {
		recordTable[i].recordname = const_cast<char*>("########");
	}

	char buffer[64];
	int r;
	for (int j = 0; j < 64; j++) buffer[j] = '#';
	r = myPM->readDiskBlock(1, buffer); //1th position should be root dir
	if (buffer[0] == '#') {
		cout << "no root dir for part: " << myrecordManagementName << " creating root dir..." << endl;
		r = createDirectory(const_cast<char*>("/R"), 2);
	}
	else if (buffer[0] == 'R') {
		cout << "root dir found for part: " << myrecordManagementName << endl;
		GrootDirAddr = 1;
	}

	for (int i = 0; i < 1000; i++) {
		recordTable[i].recordname = const_cast<char*>("########");
	}

	for (int j = 0; j < 19; j++) Gaddrarr[j] = -1;
}

//return 0 if created successfully
//return -1 if record exists
//return -2 if not enough disk space
//return -3 if invalid input only a-z, A-Z allowed 
//return -4 if directory of same name where you wanna make it
// -4 if invalid path
int RecordManagement::createRecord(char *recordname, int rnameLen)
{
	if (rnameLen < 2) return -3;
	for (int i = 0; i < rnameLen; i++) {
		if ((i % 2 == 0) && (recordname[i] != '/')) {
			return -3;
		}
		else if ((i % 2 == 1) && (!isalpha(recordname[i]))) {
			return -3;
		}
	}

	char rname = recordname[rnameLen - 1];
	char buffer[64];
	char rootbuf[64];
	char crapbuf[64];
	char buff[64];
	for (int j = 0; j < 64; j++) buff[j] = '#';
	for (int j = 0; j < 64; j++) buffer[j] = '#';
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	for (int j = 0; j < 64; j++) crapbuf[j] = '#';
	int pos = 1;

	//subdir is set to addr block of the directory we are writing in
	int i = 1;
	char curdirname;
	char newrname = recordname[rnameLen - 1];
	int currentdiraddr = GrootDirAddr;
	int r = myDM->readDiskBlock(myrecordManagementName, GrootDirAddr, rootbuf); //start at RD

	//go through dir structure, and either cd down, or mkdir as you go

	while (i <= rnameLen) {
		curdirname = recordname[i]; //set to every other entry in dirname 
		int bufpos = 0;	
		//this loop will get the addr of the next dir and put it in rootbuf
		while (true) {//loop starting at Rood dir, and go through all the way

			//check that newrname DNE here already for recordname
			if (rootbuf[bufpos] == newrname && rootbuf[bufpos + 5] == 'F' && (i == rnameLen - 1))return -4;
			//if dir already exists, do this
			if (rootbuf[bufpos] == newrname && rootbuf[bufpos + 5] == 'D' && (i == rnameLen - 1))return -4;


			if (rootbuf[bufpos] == '#') {//need to write record name here and reserve its block
				rootbuf[bufpos] = recordname[i]; 
				int fdb = myPM->getFreeDiskBlock();
				myDM->intToChar(bufpos + 1, fdb, rootbuf);

				if (i == rnameLen - 1) {
					rootbuf[bufpos + 5] = 'F'; //if we at end of recordname, write record
				}
				else {
					rootbuf[bufpos + 5] = 'D';	
				}
				//reserve new addr
				r = myPM->writeDiskBlock(fdb, buff);
				r = myPM->writeDiskBlock(currentdiraddr, rootbuf);

				if (i == rnameLen - 1) {//we just put the record in the dir, now need to write the record inode
					buff[0] = newrname;
					buff[1] = 'F';
					myDM->intToChar(2, 0, buff);
					r = myPM->writeDiskBlock(fdb, buff);
					return 0;
				}
				else {

					r = myDM->readDiskBlock(myrecordManagementName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
					break;

				};

			}
			else if (rootbuf[bufpos] == curdirname) {//if we have found a dir, need to CD into it
			   //exists, so just move into it
				int newdiraddr = myDM->charToInt(bufpos + 1, rootbuf);

				r = myDM->readDiskBlock(myrecordManagementName, newdiraddr, rootbuf);
				currentdiraddr = newdiraddr;
				break;
			}

			bufpos += 6;
			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				if (rootbuf[bufpos] != '#') { //good, something is there, keepmovin
					int xy = myDM->charToInt(bufpos, rootbuf);
					r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
					currentdiraddr = xy;
					bufpos = 0;
				}
				else {//need to set it up
					int fdb = myPM->getFreeDiskBlock(); //get free DB and write it to curdir, 
					for (int j = 0; j < 64; j++) buff[j] = '#';
					r = myPM->writeDiskBlock(fdb, buff); //reserve it

					myDM->intToChar(bufpos, fdb, rootbuf); //put new addr in current dir
					r = myPM->writeDiskBlock(currentdiraddr, rootbuf);

					//overwrite for next loop iter
					r = myDM->readDiskBlock(myrecordManagementName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
				}
			}
		}
		i += 2;
	}

	return 0;
}

//-1 if record desc is invalid
//-2 if len < 0
//-3 if the operation is not permitted
//numb of bytes read if successful
int RecordManagement::readRecord(int recordDesc, char *data, int len)
{
	if (recordTable[recordDesc].recordname[0] == '#') return -1;
	if (len < 0) return -2;
	if (recordTable[recordDesc].mode != 'r' && recordTable[recordDesc].mode != 'm') return -3;

	char recordBuf[64];
	char inodeBuf[64];
	char* recordname = recordTable[recordDesc].recordname;
	int r, addrnumb;
	int pos = 1;
	int x = -22;
	int x2 = -22;
	int count = 0;
	int rwptr = 0;
	for (int j = 0; j < 64; j++) recordBuf[j] = '#';
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is recordname addr 
	//inodeBuf is record inode

	x = recordfound(recordname, strlen(recordname));
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);

	rwptr = recordTable[recordDesc].rw;

	int indir = getaddrs(inodeBuf);
	int recordwalker = 0;
	int readcount = 0;
	int recordsize = myDM->charToInt(2, inodeBuf);
	//Gaddrarr has array of all blocks the record is using 
	int i = 0;
	while (Gaddrarr[i] != -1) {
		r = myDM->readDiskBlock(myrecordManagementName, Gaddrarr[i], recordBuf);
		//recordBuf is now the buffer at an addr for the record 
		int j = 0;
		while (recordBuf[j] != '#' && j < 64) {//read to the end of the buffer
			if (recordwalker >= rwptr) {//gotta get to the rwptr first
				if (readcount >= len) {
					recordTable[recordDesc].rw = rwptr + readcount;
					return readcount;
				}
				if (recordwalker >= recordsize) {
					recordTable[recordDesc].rw = rwptr + readcount;
					return readcount;
				}
				data[recordwalker] = recordBuf[j];
				readcount++;
			}
			recordwalker++;
			j++;
		}
	}
	recordTable[recordDesc].rw = rwptr + readcount;
	return readcount;
}

//This operation creates a new directory whose name is pointed to by dirname.
//-1 if the directory already exists, 
//-2 if there is not enough disk space
//-3 if invalid directory name, 
//-4 if record of directory name exists already
//0 if the directory is created successfully.
int RecordManagement::createDirectory(char* dirname, int dnameLen)
{//dirs have this stucture:      /e/a/a/b
	//dir inode is: [name,Block pointer, flag]x10 , bpointer(if needed)
	//name: 1 byte, just stores name of dir
	//flag: 1 byte can be F,D, stores stuff at that dir
	//Block pointer: 4 bytes, points to the block of the record/dir

	//root dir first
	char buff[64];
	for (int j = 0; j < 64; j++) buff[j] = '#';

	if (dnameLen < 2) return -3;
	for (int i = 0; i < dnameLen; i++) {
		if ((i % 2 == 0) && (dirname[i] != '/')) {
			return -3;
		}
		else if ((i % 2 == 1) && (!isalpha(dirname[i]))) {
			return -3;
		}
	}

	if (dirname[1] == 'R' && dnameLen == 2) {
		//create root dir
		buff[0] = 'R';
		int pos = myPM->getFreeDiskBlock();
		myDM->intToChar(1, pos, buff);
		buff[5] = 'D';
		cout << "Root dir buff is: " << endl; for (int j = 0; j < 64; j++) cout << buff[j]; cout << endl;
		GrootDirAddr = pos;
		cout << "root dir adddr is: " << GrootDirAddr << endl;
		return myPM->writeDiskBlock(pos, buff);
	}

	if (dnameLen == 2 && dirname[1] == 'R') {
		cout << "-------------------------------NOT SUPPOSED TO HAPPEN" << endl;
		sleep(100);
	}

	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	int i = 1;
	char curdirname;
	char newdirname = dirname[dnameLen - 1];
	int currentdiraddr = GrootDirAddr;
	int r = myDM->readDiskBlock(myrecordManagementName, GrootDirAddr, rootbuf); //start at RD

	//go through dir structure, and either cd down, or mkdir as you go

	while (i <= dnameLen) {
		curdirname = dirname[i]; //set to every other entry in dirname
		int bufpos = 0;

		//this loop will get the addr of the next dir and put it in rootbuf
		while (true) {//loop starting at Rood dir, and go through all the way

			//check that newdirname DNE here already for recordname
			if (rootbuf[bufpos] == newdirname && rootbuf[bufpos + 5] == 'F' && (i == dnameLen - 1))return -4;
			//if dir already exists, do this
			if (rootbuf[bufpos] == newdirname && rootbuf[bufpos + 5] == 'D' && (i == dnameLen - 1))return -1;

			if (rootbuf[bufpos] == '#') {//need to write dir name here and reserve its block
				rootbuf[bufpos] = dirname[i];
				int fdb = myPM->getFreeDiskBlock();
				myDM->intToChar(bufpos + 1, fdb, rootbuf);
				rootbuf[bufpos + 5] = 'D';

				//reserve new addr
				for (int j = 0; j < 64; j++) buff[j] = '#';
				r = myPM->writeDiskBlock(fdb, buff);
				r = myPM->writeDiskBlock(currentdiraddr, rootbuf);

				if (i == dnameLen - 1) return 0; //if we at the end of dirname, done
				else {

					r = myDM->readDiskBlock(myrecordManagementName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
					break;

				};

			}
			else if (rootbuf[bufpos] == curdirname) {//if we have found a dir, need to CD into it
			   //exists, so just move into it
				int newdiraddr = myDM->charToInt(bufpos + 1, rootbuf);

				r = myDM->readDiskBlock(myrecordManagementName, newdiraddr, rootbuf);
				currentdiraddr = newdiraddr;
				break;
			}

			bufpos += 6;
			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				if (rootbuf[bufpos] != '#') { //good, something is there, keepmovin
					int xy = myDM->charToInt(bufpos, rootbuf);
					r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
					currentdiraddr = xy;
					bufpos = 0;
				}
				else {//need to set it up
					int fdb = myPM->getFreeDiskBlock(); //get free DB and write it to curdir, 
					for (int j = 0; j < 64; j++) buff[j] = '#';
					r = myPM->writeDiskBlock(fdb, buff); //reserve it

					myDM->intToChar(bufpos, fdb, rootbuf); //put new addr in current dir
					r = myPM->writeDiskBlock(currentdiraddr, rootbuf);

					//overwrite for next loop iter
					r = myDM->readDiskBlock(myrecordManagementName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
				}
			}
		}
		i += 2;
	}

	return 0;
}

//returns int of the lock id if successful
//-1 if record is already locked
//-2 if record DNE
//-3 if it is currently opened
//-4 if it cannot be locked for any other reason
int RecordManagement::lockRecord(char* recordname, int rnameLen)
{
	//new int for lock 0 for unlocked
	//1 for open 0 for closed
	int found = -1;
	int x = 0;
	int r = -1;
	char crap[64];
	for (int j = 0; j < 64; j++) crap[j] = '#';

	if (recordfound(recordname, rnameLen) == -2) return -4;
	if (recordfound(recordname, rnameLen) == -1) return -2;

	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname) { //for all of that record
			found = 1;
			if (recordTable[i].lock != 0) {
				return -1; //if any instance of this record is locked, return -1			
			}
			if (recordTable[i].open == 1) return -3; //if any instance is open, return -3

		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			x = i;
			break;
		}
	}

	locker += 3;
	int newlockid = locker;

	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname) { //for all of that record
			recordTable[i].lock = newlockid;

		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			break;
		}
	}
	if (found < 0) { //this record isn't in table yet, so put it there
		recordTable[x].recordname = recordname;
		recordTable[x].lock = newlockid;
		recordTable[x].open = 0;
		recordTable[x].rw = 0;
	}
	return newlockid;
}

//0 if successful
//-1 if lockid is invalid
//-2 for any other reason,like  not in table
int RecordManagement::unlockRecord(char* recordname, int rnameLen, int lockId)
{
	int found = -2;
	if (recordfound(recordname, rnameLen) == 0) return -2;


	char crap[64];
	for (int j = 0; j < 64; j++) crap[j] = '#';

	//need to unlock every instance of that record in the table
	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname) {//recordname found, unlock it
			if (recordTable[i].lock != lockId) return -1;
			found = 1;
		}

		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			break;
		}
	}

	if (found == -2) return -2; //record not in table

	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname) {
			recordTable[i].lock = 0;
		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			break;
		}
	}
	return 0;
}

//-1 if record DNE
//-2 if mode is invalid
//-3 if record cannot be opened because of locking restrictions
//-4 for any other reason
//integer of index in recordtable array and set the rw pointer
int RecordManagement::openRecord(char* recordname, int rnameLen, char mode, int lockId)
{ //c, 2, r, -1
	int found = -2;
	int f2 = -2;
	char crap[64];
	for (int j = 0; j < 64; j++) crap[j] = '#';

	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname) {
			found = i; //if the record is there already, can use found later
			break;
		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			f2 = i;
			break;
		}

	}

	//still going to open at new spot f2

	if (found == -2) {//not in table yet
		//1 if found 0 otherwise
		if (recordfound(recordname, rnameLen) == -1) {
			cout << "recordfound!" << endl;
			return -1;
		}
		if (lockId != -1) return -3;

		//record not in table, but is on Disk
	}
	else {//if record already in table, gotta have same lockId
		if (recordTable[found].lock == 0 && lockId != -1) return -3;
		if (recordTable[found].lock != 0 && recordTable[found].lock != lockId) return -3;
		else recordTable[f2].lock = recordTable[found].lock;
	}


	if (rnameLen < 2) return -1;
	for (int i = 0; i < rnameLen; i++) {
		if ((i % 2 == 0) && (recordname[i] != '/')) {
			return -1;
		}
		else if ((i % 2 == 1) && (!isalpha(recordname[i]))) {
			return -1;
		}
	}

	if (mode != 'r' && mode != 'w' && mode != 'm') return -2;

	if (f2 == -2) {
		for (int i = 1; i < 1000; i++) {
			char* hash = recordTable[i].recordname;
			if (hash[1] == '#') {
				f2 = i;
				break;
			}
		}
		if (f2 == -2) {
			cout << "ERROR, FIX ME 3453412389" << endl;
			sleep(100);
		}
	}
	//if a record is opened with the correct lock id, then 
	if (lockId != -1) {
		recordTable[f2].lock = lockId;
	}
	else {
		recordTable[f2].lock = 0;
	}

	recordTable[f2].recordname = recordname;
	recordTable[f2].open = 1;
	recordTable[f2].rw = 0;
	if (mode == 'r') {
		recordTable[f2].mode = 'r';
		return f2;
	}
	else if (mode == 'w') {
		recordTable[f2].mode = 'w';
		return f2;
	}
	else if (mode == 'm') {
		recordTable[f2].mode = 'm';
		return f2;
	}

	return -4;
}

//-1 if record desc is invalid
//-2 for any other reason
//0 if successful
int RecordManagement::closeRecord(int recordDesc)
{
	if (recordDesc >= 1000 || recordDesc < 1) return -1;
	char* hash = recordTable[recordDesc].recordname;
	if (hash[1] == '#') {
		return -1;
	}
	if (recordTable[recordDesc].open == 0) return -1;

	recordTable[recordDesc].open = 0;

	return 0;
}

//-1 if record DNE
//-2 if record opened or locked
//-3 if cannot be deleted for any other reason
//0 if deleted successfully
int RecordManagement::deleteRecord(char *recordname, int rnameLen)
{
	if (rnameLen < 2) return -3;
	for (int i = 0; i < rnameLen; i++) {
		if ((i % 2 == 0) && (recordname[i] != '/')) {
			return -3;
		}
		else if ((i % 2 == 1) && (!isalpha(recordname[i]))) {
			return -3;
		}
	}

	if (recordfound(recordname, rnameLen) == -1) return -1;

	char inodeBuf[64];
	int recordaddr;
	int x, pos, r;
	int found = -1;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';
	r = myPM->readDiskBlock(pos, inodeBuf);

	pos = 0;

	//x is record block addr
	//inodeBuf is record buffer 
	x = recordfound(recordname, rnameLen);
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);

	//is open? if yes return -2;
	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname) {
			found = i; //if the record is there already, can use found later
			if (recordTable[found].open == 1) return -2; //if any instance open, fail
			if (recordTable[found].lock != 0) return -2;
		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			break;
		}
	}

	//if it passes all those checks, need to actually move through record and delete it
	//put all the addrs into an array, then just call  
	int Saddrarr[19];
	for (int j = 0; j < 19; j++) Saddrarr[j] = -1; // fill with -1s
	pos = 6;
	int i = 0;
	char buff[64];
	x = -1;
	for (int j = 0; j < 64; j++) buff[j] = '#';

	while (inodeBuf[pos] != '#' && pos < 18) {
		Saddrarr[i] = myDM->charToInt(pos, inodeBuf);
		pos += 4;
		i++;
	}

	if (inodeBuf[pos] != '#') { //if the record has an indir block, set it to 
		x = myDM->charToInt(pos, inodeBuf);
		//x is now addr block of the indir
		int r = myDM->readDiskBlock(myrecordManagementName, x, buff);
		//buff should be indir node
		pos = 0; // start at 0th pos of indir 
		while (buff[pos] != '#' && pos < 64) {
			Saddrarr[i] = myDM->charToInt(pos, buff);
			pos += 4;
			i++;
		}
	}

	int indirblock = x;
	char rootbuf[64];
	char testbuf[64];
	for (int j = 0; j < 64; j++) testbuf[j] = '#';
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';

	//now delete every addr block in Gaddr
	for (int i = 0; i < 19; i++) {
		if (Saddrarr[i] != -1) {
			r = myDM->readDiskBlock(myrecordManagementName, Saddrarr[i], testbuf);
			myPM->returnDiskBlock(Saddrarr[i]);
			r = myDM->readDiskBlock(myrecordManagementName, Saddrarr[i], testbuf);
		}
	}

	if (indirblock != -1) {
		r = myPM->returnDiskBlock(indirblock);
	}

	//need to 'del' from directories
	//get current dir of record
	int direc = getsubdir(recordname, rnameLen);
	char rname = recordname[rnameLen - 1];

	r = myDM->readDiskBlock(myrecordManagementName, direc, rootbuf); //get the buffer of the subdir
	int bufpos = 0;
	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') return -8; //shouldn't hit this
		if (rootbuf[bufpos] == rname && rootbuf[bufpos + 5] == 'F') {//you have found the record
			rootbuf[bufpos + 5] = 'X';
			r = myPM->writeDiskBlock(direc, rootbuf);
			return 0;
		}
		bufpos += 6;

		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
			bufpos = 0;
			direc = xy;
		}
	}

	r = myPM->returnDiskBlock(x);

	return 0;
}

//-1 if dir DNE
//-2 if ANYTHING in directory
//-3 if cannot be deleted for any other reason
//0 if successful
int RecordManagement::deleteDirectory(char* dirname, int dnameLen)
{
	int subdir = getsubdir(dirname, dnameLen);
	int r, diraddr;
	char rootbuf[64];
	char dirbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	for (int j = 0; j < 64; j++) dirbuf[j] = '#';
	//now just scrape through subdir until you find (or don't find) record, should find it though
	char mydirname = dirname[dnameLen - 1];

	r = myDM->readDiskBlock(myrecordManagementName, subdir, rootbuf);

	int bufpos = 0;
	int dirpos = 0;
	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') return -1; //should be filled in order
		if (rootbuf[bufpos] == mydirname && rootbuf[bufpos + 5] == 'D') {//you have found the dir
			diraddr = myDM->charToInt(bufpos + 1, rootbuf);
			r = myDM->readDiskBlock(myrecordManagementName, diraddr, dirbuf);
			dirpos = bufpos;
			break;
		}

		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
			bufpos = 0;
		}
		else bufpos += 6;
	}
	//diraddr is the address block of our dir
	//dirbuf is the buffer of the dir
	//rootbuf is the directory buffer in which the dir was found, set X there
	//dirpos is the position in rootbuf where the dir should be 

	//if buffer is not empty, return -2
	if (dirbuf[0] != '#') return-2;
	else {
		//need to 'del' from directories
		//get current dir of record 
		int direc = getsubdir(dirname, dnameLen);
		char rname = dirname[dnameLen - 1];
	
		r = myDM->readDiskBlock(myrecordManagementName, direc, rootbuf); //get the buffer of the subdir
		int bufpos = 0;
		while (true) {//loop starting at subdir, and go through all the way
			if (rootbuf[bufpos] == '#') return -8; //shouldn't hit this
			if (rootbuf[bufpos] == rname && rootbuf[bufpos + 5] == 'D') {//you have found the dir
				rootbuf[bufpos + 5] = 'X';
				r = myPM->writeDiskBlock(direc, rootbuf);
				return 0;
			}
			bufpos += 6;

			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				int xy = myDM->charToInt(bufpos, rootbuf);
				r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
				bufpos = 0;
				direc = xy;
			}
		}
	}

	return 0;
}

//-1 if record descriptor is invalid
//-2 if length is negative
//-3 if operation is not permitted read/writes
//on success, return number of bytes written
int RecordManagement::updateRecord(int recordDesc, char *data, int len)
{
	if (recordDesc < 0 || recordDesc >= 1000) return -1;
	if (recordTable[recordDesc].recordname[0] == '#') return -1;
	if (len < 0) return -2;
	if (recordTable[recordDesc].mode != 'w' && recordTable[recordDesc].mode != 'm') return -3;
	//intial tests passed

	char adrBuf[64];
	char inodeBuf[64];
	char* recordname = recordTable[recordDesc].recordname;
	int r, addrnumb;
	int pos = 1;
	int x = -22;
	int x2 = -22;
	int count = 0;
	int rwptr = 0;
	for (int j = 0; j < 64; j++) adrBuf[j] = '#';
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is recordname addr 
	//inodeBuf is record inode

	x = recordfound(recordname, strlen(recordname));
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);
	//so now the record is found and its addr block is at x and inode in the inodeBuf
	rwptr = recordTable[recordDesc].rw;
	int start = 0;
	int dataStart = 0;
	int end = rwptr + len;
	addrnumb = 2;
	//so move throug the record inode and only put stuff in the data adrBuf if start>= rwptr
	//write everything from rwptr to rwptr + len into data

	while (true) {
		for (int j = 0; j < 64; j++) adrBuf[j] = '#';
		//pos is get the block addr from the record/indir inode
		pos = myDM->charToInt(addrnumb, inodeBuf);
		if (addrnumb == 2) {//if we are looking at the size
			int currecsize = myDM->charToInt(2, inodeBuf);
			int recordsize = max(currecsize, pos + len);
			if (recordsize > 1216) recordsize = 1216;
			myDM->intToChar(2, recordsize, inodeBuf);
			r = myPM->writeDiskBlock(x, inodeBuf);
		}
		else if (addrnumb > 2 && addrnumb < 18) {
			if (pos == 0) {//then we need to get a freeblock and assign it here
				pos = myPM->getFreeDiskBlock();
				myDM->intToChar(addrnumb, pos, inodeBuf);
				r = myPM->writeDiskBlock(x, inodeBuf);
			}
			//pos is the addr of the next block we are writing at
			r = myPM->readDiskBlock(pos, adrBuf);

			for (int i = 0; i < 64; i++) {
				if (start >= rwptr) {
					adrBuf[i] = data[dataStart];
					dataStart++;
				}

				if (start >= end) break;
				start++;
			}

			r = myPM->writeDiskBlock(pos, adrBuf);
			if (start >= end) break;

		}
		else if (addrnumb >= 18) {
			int saddrnumb = addrnumb - 18;
			//then need to set indir block and write new things to it
			if (addrnumb == 18) {//write the indir block to the record inode
				pos = myDM->charToInt(addrnumb, inodeBuf);
				if (pos == 0) {//then we need to get a freeblock and assign it here
					pos = myPM->getFreeDiskBlock();
					myDM->intToChar(addrnumb, pos, inodeBuf);

					r = myPM->writeDiskBlock(x, inodeBuf);

					for (int j = 0; j < 64; j++) adrBuf[j] = '#';
					x = pos; //x is now the indir addr
					r = myPM->writeDiskBlock(pos, adrBuf); //writes crap at the indir addr

					r = myDM->readDiskBlock(myrecordManagementName, pos, inodeBuf);

				}
				else {//if indir already set, just need to set vars
					x = pos;
					r = myDM->readDiskBlock(myrecordManagementName, pos, inodeBuf);
					//x is now indir addr				
					//inodeBuf is now buffer of the inode
				}

			}
			//now we are definitely at the indir inode
			pos = myDM->charToInt(saddrnumb, inodeBuf);


			if (pos == 0) {//need to get a new free block and write to indir block
				pos = myPM->getFreeDiskBlock();
				myDM->intToChar(saddrnumb, pos, inodeBuf);
				r = myPM->writeDiskBlock(x, inodeBuf);
			}
			//we know that we are writing to pos, so write things to it
			//////////////////////////////////////////////

			r = myPM->readDiskBlock(pos, adrBuf);
			for (int i = 0; i < 64; i++) {
				if (start >= rwptr) {
					adrBuf[i] = data[dataStart];
					dataStart++;
				}

				if (start >= end) break;
				start++;
			}

			r = myPM->writeDiskBlock(pos, adrBuf);
			if (start >= end) break;
			////////////////////////////////////////////

		}
		else {
			break;
		}

		addrnumb += 4;

	}

	recordTable[recordDesc].rw = rwptr + dataStart;
	return len;
}

//-1 if recorddesc invalid
//-2 if len is neg
//-3 if operation not permitted
int RecordManagement::appendRecord(int recordDesc, char* data, int len)
{
	if (recordTable[recordDesc].recordname[0] == '#') return -1;
	if (len < 0) return -2;
	if (recordTable[recordDesc].mode != 'w' && recordTable[recordDesc].mode != 'm') return -3;

	char inodeBuf[64];
	int r, x, recordsize;
	int pos = 0;
	int rwptr = recordTable[recordDesc].rw;
	char* recordname = recordTable[recordDesc].recordname;
	//need to get record size 
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is recordname addr 
	//inodeBuf is record inode

	x = recordfound(recordname, strlen(recordname));
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);

	//get record size
	recordsize = myDM->charToInt(2, inodeBuf);
	if (recordsize >= 1216) return -3;
	//move rwptr to recordsize, which is the end, then call write func
	rwptr = recordsize;
	recordTable[recordDesc].rw = rwptr;
	return updateRecord(recordDesc, data, len);
}

//-1 if recordDesc, offset, or flag is invalid
//-2 if attempt to go outside record bounds. (end or beginning of record
//0 if successful
int RecordManagement::seekRecord(int recordDesc, int offset, int flag)
{
	if (recordTable[recordDesc].recordname[0] == '#') return -1;
	if (offset < 0 && flag != 0) return -1;
	char inodeBuf[64];
	int r, x, recordsize;
	int pos = 0;
	int rwptr = recordTable[recordDesc].rw;
	char* recordname = recordTable[recordDesc].recordname;
	//need to get record size 
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is recordname addr 
	//inodeBuf is record inode

	x = recordfound(recordname, strlen(recordname));
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);

	//get record size
	recordsize = myDM->charToInt(2, inodeBuf);
	if (flag == 0) {
		//move rw by 'offset' bytes, can be negative
		rwptr += offset;
	}
	else { //if flag is anything else, then set rw pointer to offset
		rwptr = offset;

	}
	if (rwptr < 0 || rwptr > recordsize) return -2;

	recordTable[recordDesc].rw = rwptr;
	return 0;
}

//-1 if invalid recordname
//-2 if record DNE
//-3 if record already exists
//-4 if record is open/locked
//-5 for any other reason
//0 if successful
int RecordManagement::renameRecord(char* recordname1, int rnameLen1, char* recordname2, int rnameLen2)
{ //change recordname1 to recordname2
	if (rnameLen1 < 2) return -1;
	for (int i = 0; i < rnameLen1; i++) {
		if ((i % 2 == 0) && (recordname1[i] != '/')) {
			return -1;
		}
		else if ((i % 2 == 1) && (!isalpha(recordname1[i]))) {
			return -1;
		}
	}

	//now check recordname2
	if (rnameLen2 < 2) return -1;
	for (int i = 0; i < rnameLen2; i++) {
		if ((i % 2 == 0) && (recordname2[i] != '/')) {
			return -1;
		}
		else if ((i % 2 == 1) && (!isalpha(recordname2[i]))) {
			return -1;
		}
	}

	if (recordfound(recordname2, rnameLen2) != -1) return -3;
	if (recordfound(recordname1, rnameLen1) == -1) return -2;
	//need to get record addr, inodebuff, and recorddesc

	char inodeBuf[64];
	int recordaddr;
	int x, pos, r;
	int found = -1;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';
	//x is recordname addr 
	//inodeBuf is record inode

	x = recordfound(recordname1, rnameLen1);
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);

	//is open? if yes return -4;
	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname1) {
			found = i; //if the record is there already, can use found later
			if (recordTable[found].open == 1) return -4; //if any instance open, fail
			if (recordTable[found].lock != 0) return -4;
		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			break;
		}
	}

	//need to rename everything in recordTable too 
	for (int i = 1; i < 1000; i++) {
		if (recordTable[i].recordname == recordname1) {
			recordTable[i].recordname = recordname2;
		}
		char* hash = recordTable[i].recordname;
		if (hash[1] == '#') {
			break;
		}
	}
	//change on directory system as well 
	int subdir = getsubdir(recordname1, rnameLen1);
	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	//now just scrape through subdir until you find (or don't find) record, should find it though
	char rname = recordname1[rnameLen1 - 1];
	char rname2 = recordname2[rnameLen2 - 1];

	r = myDM->readDiskBlock(myrecordManagementName, subdir, rootbuf); //get the buffer of the subdir
	int bufpos = 0;
	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') return -1; //should be filled in order
		if (rootbuf[bufpos] == rname && rootbuf[bufpos + 5] == 'F') {//you have found the record
			rootbuf[bufpos] = rname2;
			myPM->writeDiskBlock(subdir, rootbuf);
			break;
		}

		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myrecordManagementName, xy, rootbuf);
			bufpos = 0;
		}
		else bufpos += 6;
	}
	//passed all checks, now change the recordname on disk and write it
	inodeBuf[0] = recordname2[rnameLen2 - 1];


	return myPM->writeDiskBlock(x, inodeBuf);

}

int RecordManagement::getAttribute(char* recordname, int rnameLen, char* girth)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is record block addr
	//inodeBuf is record buffer 
	x = recordfound(recordname, rnameLen);
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);
	girth[0] = inodeBuf[23];
	return 0;
}

//returns -1 on failure
//returns 0 on good
int RecordManagement::setAttribute(char* recordname, int rnameLen, char girth)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is record block addr
	//inodeBuf is record buffer 
	x = recordfound(recordname, rnameLen);
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);

	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;

	inodeBuf[23] = girth;
	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;
	return myPM->writeDiskBlock(x, inodeBuf);

	return -1;
}

int RecordManagement::getAttribute2(char* recordname, int rnameLen)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is record block addr
	//inodeBuf is record buffer 
	x = recordfound(recordname, rnameLen);
	r = myDM->readDiskBlock(myrecordManagementName, x, inodeBuf);
	return myDM->charToInt(24, inodeBuf);

}

//returns -1 on failure
//returns 0 on good
int RecordManagement::setAttribute2(char* recordname, int rnameLen, int shaft)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is record block addr
	//inodeBuf is record buffer 
	x = recordfound(recordname, rnameLen);
	r = myDM->readDiskBlock(myrecordManegementName, x, inodeBuf);

	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;
	myDM->intToChar(24, shaft, inodeBuf);
	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;
	return myPM->writeDiskBlock(x, inodeBuf);

	return -1;
}