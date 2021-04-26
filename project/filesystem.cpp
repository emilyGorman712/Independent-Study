#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
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

//helper methods

//given file inode, puts all used blocks into Gaddrarr global val
//returns -1 if no indir block
//if indir exists, returns blockaddr of it
int FileSystem::getaddrs(char* inodeBuf) {
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


	if (inodeBuf[pos] != '#') { //if the file has an indir block, set it to 
		x = myDM->charToInt(pos, inodeBuf);
		//x is now addr block of the indir
		int r = myDM->readDiskBlock(myfileSystemName, x, buff);
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
//return -1 if shit don't work
//return 0 if shit do work
//this assumes that all files are len 1 - not great
int FileSystem::rollbackRoot(char* filename, int fnameLen) {

	char fname = filename[fnameLen - 1];
	char rootbuf[64];
	char tempbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	for (int j = 0; j < 64; j++) tempbuf[j] = '#';
	int tbufpos = 0;
	int tbufpos2;
	int overflowblock = GrootDirAddr;
	bool done = false;
	int r;
	r = myDM->readDiskBlock(myfileSystemName, GrootDirAddr, rootbuf); //get the buffer of the subdir
	int bufpos = 0;

	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') {
			r = myPM->writeDiskBlock(overflowblock, tempbuf);
			return 0;
		}
		if (done == false) {
			if (rootbuf[bufpos] == fname && rootbuf[bufpos + 5] == 'F') {//you have found the file
				done = true;
			}
		}
		if (bufpos < 60) {
			if (rootbuf[bufpos] != filename[fnameLen - 1]) {
				tbufpos2 = bufpos;
				for (int q = 0; q < 6; q++) {
					tempbuf[tbufpos] = rootbuf[tbufpos2];
					tbufpos++;
					tbufpos2++;
				}
			}
		}
		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			tbufpos2 = bufpos;
			for (int q = 0; q < 4; q++) {
				tempbuf[tbufpos2] = rootbuf[tbufpos2];
				tbufpos2++;
			}
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
			if (done == true) {
				tbufpos = 54;
				for (int q = 0; q < 6; q++) {
					tempbuf[tbufpos] = rootbuf[q];
					tbufpos++;
				}
			}
			r = myPM->writeDiskBlock(overflowblock, tempbuf);
			overflowblock = xy;
			for (int j = 0; j < 64; j++) tempbuf[j] = '#';
			if (done == true) {
				bufpos = 6;
			}
			else
			{
				bufpos = 0;
			}
			tbufpos = 0;
		}
		else bufpos += 6;
	}
}

//-1 if DNE
//-2 if Dir already there
//file block addr if found
int FileSystem::filefound(char* filename, int len) {
	int subdir = getsubdir(filename, len); //working for at least single files 
	//now just scrape through subdir until you find (or don't find) file
	if (subdir == -4) return -1;
	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	int i = 1;
	char curdirname;
	char fname = filename[len - 1]; //file we're looking for. should already be in the correct dir

	int r = myDM->readDiskBlock(myfileSystemName, subdir, rootbuf);
	int bufpos = 0;

	//this loop will get the addr of the next dir and put it in rootbuf
	while (true) {//loop starting at Rood dir, and go through all the way
		if (rootbuf[bufpos] == '#') return -1;
		if (rootbuf[bufpos] == fname && rootbuf[bufpos + 5] == 'F') {
			int xy = myDM->charToInt(bufpos + 1, rootbuf);
			return xy;
		}
		else if (rootbuf[bufpos] == fname && rootbuf[bufpos + 5] == 'D') {
			//then we found a directory of that name...
			return -2;
		}
		bufpos += 6;
		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
			bufpos = 0;
		}
	}
}

//given file inode, puts all used blocks into Gaddrarr global val
//returns -1 if no indir block
//if indir exists, returns blockaddr of it


//return addr of lowest subdir, 2 less than end
//-4 if path DNE
//assumes /char/char/char validdate b4 sending here
int FileSystem::getsubdir(char* filename, int len) {
	if (len == 2) return GrootDirAddr; //just root directory addr
	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	int i = 1;
	char curdirname;

	int r = myDM->readDiskBlock(myfileSystemName, GrootDirAddr, rootbuf);
	while (i < (len)) {
		curdirname = filename[i];
		int bufpos = 0;

		//this loop will get the addr of the next dir and put it in rootbuf
		while (true) {//loop starting at Rood dir, and go through all the way
			if (rootbuf[bufpos] == '#') return -4;
			if (rootbuf[bufpos] == curdirname && rootbuf[bufpos + 5] == 'D') {
				int xy = myDM->charToInt(bufpos + 1, rootbuf);
				r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
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
				r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
				bufpos = 0;
			}
			else bufpos += 6;
		}
		i += 2;
	}
	return -4;
}

FileSystem::FileSystem(DiskManager* dm, char fileSystemName)
{
	myDM = dm;
	myfileSystemName = fileSystemName;
	myfileSystemSize = myDM->getPartitionSize(fileSystemName);
	myPM = new PartitionManager(dm, fileSystemName, myfileSystemSize);
	locker = 1;

	char buffer[64];
	int r;
	for (int j = 0; j < 64; j++) buffer[j] = '#';
	r = myPM->readDiskBlock(1, buffer); //1th position should be root dir
	if (buffer[0] == '#') {
		cout << "no root dir for part: " << myfileSystemName << " creating root dir..." << endl;
		r = createDirectory(const_cast<char*>("/R"), 2);
	}
	else if (buffer[0] == 'R') {
		cout << "root dir found for part: " << myfileSystemName << endl;
		GrootDirAddr = 1;
	}


	//fill the filetable 
	for (int i = 0; i < 1000; i++) {
		fileTable[i].filename = const_cast<char*>("########");
	}
	for (int j = 0; j < 19; j++) Gaddrarr[j] = -1;
}


//return 0 if created successfully
//return -1 if file exists
//return -2 if not enough disk space
//return -3 if invalid input only a-z, A-Z allowed 
//return -4 if directory of same name where you want to make it
// -4 if invalid path
int FileSystem::createFile(char* filename, int fnameLen)
{
	if (fnameLen < 2) return -3;
	for (int i = 0; i < fnameLen; i++) {
		if ((i % 2 == 0) && (filename[i] != '/')) {
			return -3;
		}
		else if ((i % 2 == 1) && (!isalpha(filename[i]))) {
			return -3;
		}
	}

	char fname = filename[fnameLen - 1];

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
	char newfname = filename[fnameLen - 1];
	int currentdiraddr = GrootDirAddr;
	int r = myDM->readDiskBlock(myfileSystemName, GrootDirAddr, rootbuf); //start at RD

	//go through dir structure, and either cd down, or mkdir as you go

	while (i <= fnameLen) {
		curdirname = filename[i]; //set to every other entry in dirname 
		int bufpos = 0;

		//this loop will get the addr of the next dir and put it in rootbuf
		while (true) {//loop starting at Rood dir, and go through all the way

			//check that newfname DNE here already for filename
			if (rootbuf[bufpos] == newfname && rootbuf[bufpos + 5] == 'F' && (i == fnameLen - 1))return -4;
			//if dir already exists, do this
			if (rootbuf[bufpos] == newfname && rootbuf[bufpos + 5] == 'D' && (i == fnameLen - 1))return -4;


			if (rootbuf[bufpos] == '#') {//need to write file name here and reserve its block
				//R0004D
				rootbuf[bufpos] = filename[i]; //note that dirname is acutally the filename at this point
				int fdb = myPM->getFreeDiskBlock();
				myDM->intToChar(bufpos + 1, fdb, rootbuf);

				if (i == fnameLen - 1) {
					rootbuf[bufpos + 5] = 'F'; //if we at end of filename, write file
				}
				else {
					rootbuf[bufpos + 5] = 'D';	//I'm not sure on this... assume you can also create dirs when creating a file
				}
				//reserve new addr
				r = myPM->writeDiskBlock(fdb, buff);
				r = myPM->writeDiskBlock(currentdiraddr, rootbuf);

				if (i == fnameLen - 1) {//we just put the file in the dir, now need to write the file inode
					buff[0] = newfname;
					buff[1] = 'F';
					myDM->intToChar(2, 0, buff);
					r = myPM->writeDiskBlock(fdb, buff);
					return 0;
				}
				else {

					r = myDM->readDiskBlock(myfileSystemName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
					break;

				};

			}
			else if (rootbuf[bufpos] == curdirname) {//if we have found a dir, need to CD into it
			   //exists, so just move into it
				int newdiraddr = myDM->charToInt(bufpos + 1, rootbuf);

				r = myDM->readDiskBlock(myfileSystemName, newdiraddr, rootbuf);
				currentdiraddr = newdiraddr;
				break;
			}

			bufpos += 6;
			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				if (rootbuf[bufpos] != '#') { //good, something is there, keepmovin
					int xy = myDM->charToInt(bufpos, rootbuf);
					r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
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
					r = myDM->readDiskBlock(myfileSystemName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
				}
			}
		}
		i += 2;
	}


	return 0;

}

//This operation creates a new directory whose name is pointed to by dirname.
//-1 if the directory already exists, 
//-2 if there is not enough disk space
//-3 if invalid directory name, 
//-4 if file of directory name exists already
//0 if the directory is created successfully.

int FileSystem::createDirectory(char* dirname, int dnameLen)
{	
	//dirs have this stucture:      /e/a/a/b
	//dir inode is: [name,Block pointer, flag]x10 , bpointer(if needed)
	//name: 1 byte, just stores name of dir
	//flag: 1 byte can be F,D, stores stuff at that dir
	//Block pointer: 4 bytes, points to the block of the file/dir

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
	int r = myDM->readDiskBlock(myfileSystemName, GrootDirAddr, rootbuf); //start at RD

	//go through dir structure, and either cd down, or mkdir as you go

	while (i <= dnameLen) {
		curdirname = dirname[i]; //set to every other entry in dirname
		int bufpos = 0;

		//this loop will get the addr of the next dir and put it in rootbuf
		while (true) {//loop starting at Rood dir, and go through all the way

			//check that newdirname DNE here already for filename
			if (rootbuf[bufpos] == newdirname && rootbuf[bufpos + 5] == 'F' && (i == dnameLen - 1))return -4;
			//if dir already exists, do this
			if (rootbuf[bufpos] == newdirname && rootbuf[bufpos + 5] == 'D' && (i == dnameLen - 1))return -1;



			if (rootbuf[bufpos] == '#') {//need to write dir name here and reserve its block
				//R0004D
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

					r = myDM->readDiskBlock(myfileSystemName, fdb, rootbuf);
					currentdiraddr = fdb;
					bufpos = 0;
					break;

				};

			}
			else if (rootbuf[bufpos] == curdirname) {//if we have found a dir, need to CD into it
			   //exists, so just move into it
				int newdiraddr = myDM->charToInt(bufpos + 1, rootbuf);

				r = myDM->readDiskBlock(myfileSystemName, newdiraddr, rootbuf);
				currentdiraddr = newdiraddr;
				break;
			}

			bufpos += 6;
			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				if (rootbuf[bufpos] != '#') { //good, something is there, keepmovin
					int xy = myDM->charToInt(bufpos, rootbuf);
					r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
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
					r = myDM->readDiskBlock(myfileSystemName, fdb, rootbuf);
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
//-1 if file is already locked
//-2 if file DNE
//-3 if it is currently opened
//-4 if it cannot be locked for any other reason
int FileSystem::lockFile(char* filename, int fnameLen)
{
	//new int for lock 0 for unlocked
	//1 for open 0 for closed
	int found = -1;
	int x = 0;
	int r = -1;
	char crap[64];
	for (int j = 0; j < 64; j++) crap[j] = '#';

	if (filefound(filename, fnameLen) == -2) return -4;
	if (filefound(filename, fnameLen) == -1) return -2;

	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename) { //for all of that file
			found = 1;
			if (fileTable[i].lock != 0) {
				return -1; //if any instance of this file is locked, return -1			
			}
			if (fileTable[i].open == 1) return -3; //if any instance is open, return -3

		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			x = i;
			break;
		}
	}

	locker += 3;
	int newlockid = locker;

	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename) { //for all of that file
			fileTable[i].lock = newlockid;

		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			break;
		}
	}
	if (found < 0) { //this file isn't in table yet, so put it there
		fileTable[x].filename = filename;
		fileTable[x].lock = newlockid;
		fileTable[x].open = 0;
		fileTable[x].rw = 0;
	}
	return newlockid;
}

//0 if successful
//-1 if lockid is invalid
//-2 for any other reason,life file not in table
int FileSystem::unlockFile(char* filename, int fnameLen, int lockId)
{
	int found = -2;
	if (filefound(filename, fnameLen) == 0) return -2;


	char crap[64];
	for (int j = 0; j < 64; j++) crap[j] = '#';

	//need to unlock every instance of that file in the table
	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename) {//filename found, unlock it
			if (fileTable[i].lock != lockId) return -1;
			found = 1;
		}

		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			break;
		}
	}

	if (found == -2) return -2; //file not in table

	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename) {
			fileTable[i].lock = 0;
		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			//cout << "ended at: " <<i<<endl;
			break;
		}
	}
	return 0;
}

//-1 if file DNE
//-2 if mode is invalid
//-3 if file cannot be opened because of locking restrictions
//-4 for any other reason
//integer of index in filetable array and set the rw pointer
int FileSystem::openFile(char* filename, int fnameLen, char mode, int lockId)
{ //c, 2, r, -1
	int found = -2;
	int f2 = -2;
	char crap[64];
	for (int j = 0; j < 64; j++) crap[j] = '#';

	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename) {
			found = i; //if the file is there already, can use found later
			break;
		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			f2 = i;
			break;
		}

	}

	//still going to open at new spot f2

	if (found == -2) {//not in table yet
		//1 if found 0 otherwise
		if (filefound(filename, fnameLen) == -1) {
			cout << "filefound!" << endl;
			return -1;
		}
		if (lockId != -1) return -3;

		//file not in table, but is on Disk

	}
	else {//if file already in table, gotta have same lockId
		if (fileTable[found].lock == 0 && lockId != -1) return -3;
		if (fileTable[found].lock != 0 && fileTable[found].lock != lockId) return -3;
		else fileTable[f2].lock = fileTable[found].lock;
	}


	if (fnameLen < 2) return -1;
	for (int i = 0; i < fnameLen; i++) {
		if ((i % 2 == 0) && (filename[i] != '/')) {
			return -1;
		}
		else if ((i % 2 == 1) && (!isalpha(filename[i]))) {
			return -1;
		}
	}

	if (mode != 'r' && mode != 'w' && mode != 'm') return -2;


	if (f2 == -2) {
		for (int i = 1; i < 1000; i++) {
			char* hash = fileTable[i].filename;
			if (hash[1] == '#') {
				f2 = i;
				break;
			}
		}
		if (f2 == -2) {
			cout << "Error" << endl;
			sleep(100);
		}
	}
	//if a file is opened with the correct lock id, then 
	if (lockId != -1) {
		fileTable[f2].lock = lockId;
	}
	else {
		fileTable[f2].lock = 0;
	}

	fileTable[f2].filename = filename;
	fileTable[f2].open = 1;
	fileTable[f2].rw = 0;
	if (mode == 'r') {
		fileTable[f2].mode = 'r';
		return f2;
	}
	else if (mode == 'w') {
		fileTable[f2].mode = 'w';
		return f2;
	}
	else if (mode == 'm') {
		fileTable[f2].mode = 'm';
		return f2;
	}

	return -4;

}
//-1 if file desc is invalid
//-2 for any other reason
//0 if successful
int FileSystem::closeFile(int fileDesc)
{
	if (fileDesc >= 1000 || fileDesc < 1) return -1;
	char* hash = fileTable[fileDesc].filename;
	if (hash[1] == '#') {
		return -1;
	}
	if (fileTable[fileDesc].open == 0) return -1;


	fileTable[fileDesc].open = 0;


	return 0;
}


//-1 if file desc is invalid
//-2 if len < 0
//-3 if the operation is not permitted
//numb of bytes read if successful
int FileSystem::readFile(int fileDesc, char* data, int len)
{
	if (fileTable[fileDesc].filename[0] == '#') return -1;
	if (len < 0) return -2;
	if (fileTable[fileDesc].mode != 'r' && fileTable[fileDesc].mode != 'm') return -3;
	//intial tests passed, do real things now
	char fileBuf[64];
	char inodeBuf[64];
	char* filename = fileTable[fileDesc].filename;
	int r, addrnumb;
	int pos = 1;
	int x = -22;
	int x2 = -22;
	int count = 0;
	int rwptr = 0;
	for (int j = 0; j < 64; j++) fileBuf[j] = '#';
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is filename addr 
	//inodeBuf is file inode

	x = filefound(filename, strlen(filename));
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	rwptr = fileTable[fileDesc].rw;

	int indir = getaddrs(inodeBuf);
	int filewalker = 0;
	int readcount = 0;
	int filesize = myDM->charToInt(2, inodeBuf);
	//Gaddrarr has array of all blocks the file is using 
	int i = 0;
	while (Gaddrarr[i] != -1) {
		r = myDM->readDiskBlock(myfileSystemName, Gaddrarr[i], fileBuf);
		//fileBuf is now the buffer at an addr for the file 
		int j = 0;
		while (fileBuf[j] != '#' && j < 64) {//read to the end of the buffer
			if (filewalker >= rwptr) {//gotta get to the rwptr first
				if (readcount >= len) {
					fileTable[fileDesc].rw = rwptr + readcount;
					return readcount;
				}
				if (filewalker >= filesize) {
					fileTable[fileDesc].rw = rwptr + readcount;
					return readcount;
				}
				data[filewalker] = fileBuf[j];
				readcount++;
			}

			filewalker++;
			j++;
		}
	}
	fileTable[fileDesc].rw = rwptr + readcount;
	return readcount;

}

//-1 if file descriptor is invalid
//-2 if length is negative
//-3 if operation is not permitted read/writes
//on success, return number of bytes written
int FileSystem::writeFile(int fileDesc, char* data, int len)
{
	if (fileDesc < 0 || fileDesc >= 1000) return -1;
	if (fileTable[fileDesc].filename[0] == '#') return -1;
	if (len < 0) return -2;
	if (fileTable[fileDesc].mode != 'w' && fileTable[fileDesc].mode != 'm') return -3;
	//intial tests passed, do real things now

	char adrBuf[64];
	char inodeBuf[64];
	char* filename = fileTable[fileDesc].filename;
	int r, addrnumb;
	int pos = 1;
	int x = -22;
	int x2 = -22;
	int count = 0;
	int rwptr = 0;
	for (int j = 0; j < 64; j++) adrBuf[j] = '#';
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is filename addr 
	//inodeBuf is file inode

	x = filefound(filename, strlen(filename));
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);
	//so now the file is found and its addr block is at x and inode in the inodeBuf
	rwptr = fileTable[fileDesc].rw;
	int start = 0;
	int dataStart = 0;
	int end = rwptr + len;
	addrnumb = 2;
	//so move throug the file inode and only put stuff in the data adrBuf if start>= rwptr
	//write everything from rwptr to rwptr + len into data

	while (true) {
		for (int j = 0; j < 64; j++) adrBuf[j] = '#';
		//pos is get the block addr from the file/indir inode
		pos = myDM->charToInt(addrnumb, inodeBuf);
		if (addrnumb == 2) {//if we are looking at the size
			int curfilsize = myDM->charToInt(2, inodeBuf);
			int filesize = max(curfilsize, pos + len);
			if (filesize > 1216) filesize = 1216;
			myDM->intToChar(2, filesize, inodeBuf);
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
			if (addrnumb == 18) {//write the indir block to the file inode
				pos = myDM->charToInt(addrnumb, inodeBuf);
				if (pos == 0) {//then we need to get a freeblock and assign it here
					pos = myPM->getFreeDiskBlock();
					myDM->intToChar(addrnumb, pos, inodeBuf);

					r = myPM->writeDiskBlock(x, inodeBuf);

					for (int j = 0; j < 64; j++) adrBuf[j] = '#';
					x = pos; //x is now the indir addr
					r = myPM->writeDiskBlock(pos, adrBuf); //writes crap at the indir addr

					r = myDM->readDiskBlock(myfileSystemName, pos, inodeBuf);

				}
				else {//if indir already set, just need to set vars
					x = pos;
					r = myDM->readDiskBlock(myfileSystemName, pos, inodeBuf);
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
		else {
			cout << "Error" << endl;
			break;
		}
		addrnumb += 4;
	}
	fileTable[fileDesc].rw = rwptr + dataStart;
	return len;
}

//-1 if filedesc invalid
//-2 if len is neg
//-3 if operation not permitted
int FileSystem::appendFile(int fileDesc, char* data, int len)
{
	if (fileTable[fileDesc].filename[0] == '#') return -1;
	if (len < 0) return -2;
	if (fileTable[fileDesc].mode != 'w' && fileTable[fileDesc].mode != 'm') return -3;

	char inodeBuf[64];
	int r, x, filesize;
	int pos = 0;
	int rwptr = fileTable[fileDesc].rw;
	char* filename = fileTable[fileDesc].filename;
	//need to get file size 
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is filename addr 
	//inodeBuf is file inode

	x = filefound(filename, strlen(filename));
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	//get file size
	filesize = myDM->charToInt(2, inodeBuf);
	if (filesize >= 1216) return -3;
	//move rwptr to filesize, which is the end, then call write func
	rwptr = filesize;
	fileTable[fileDesc].rw = rwptr;
	return writeFile(fileDesc, data, len);

}

//-1 if fileDesc, offset, or flag is invalid
//-2 if attempt to go outside file bounds. (end or beginning of file
//0 if successful
int FileSystem::seekFile(int fileDesc, int offset, int flag)
{
	if (fileTable[fileDesc].filename[0] == '#') return -1;
	if (offset < 0 && flag != 0) return -1;
	char inodeBuf[64];
	int r, x, filesize;
	int pos = 0;
	int rwptr = fileTable[fileDesc].rw;
	char* filename = fileTable[fileDesc].filename;
	//need to get file size 
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	//x is filename addr 
	//inodeBuf is file inode

	x = filefound(filename, strlen(filename));
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	//get file size
	filesize = myDM->charToInt(2, inodeBuf);
	if (flag == 0) {
		//move rw by 'offset' bytes, can be negative
		rwptr += offset;
	}
	else { //if flag is anything else, then set rw pointer to offset
		rwptr = offset;

	}
	if (rwptr < 0 || rwptr > filesize) return -2;

	fileTable[fileDesc].rw = rwptr;
	return 0;
}
//-1 if invalid filename
//-2 if file DNE
//-3 if file already exists
//-4 if file is open/locked
//-5 for any other reason
//0 if successful
int FileSystem::renameFile(char* filename1, int fnameLen1, char* filename2, int fnameLen2)
{ //change filename1 to filename2
	if (fnameLen1 < 2) return -1;
	for (int i = 0; i < fnameLen1; i++) {
		if ((i % 2 == 0) && (filename1[i] != '/')) {
			return -1;
		}
		else if ((i % 2 == 1) && (!isalpha(filename1[i]))) {
			return -1;
		}
	}

	//now check filename2
	if (fnameLen2 < 2) return -1;
	for (int i = 0; i < fnameLen2; i++) {
		if ((i % 2 == 0) && (filename2[i] != '/')) {
			return -1;
		}
		else if ((i % 2 == 1) && (!isalpha(filename2[i]))) {
			return -1;
		}
	}

	if (filefound(filename2, fnameLen2) != -1) return -3;
	if (filefound(filename1, fnameLen1) == -1) return -2;
	//need to get file addr, inodebuff, and filedesc

	char inodeBuf[64];
	int fileaddr;
	int x, pos, r;
	int found = -1;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';
	//x is filename addr 
	//inodeBuf is file inode

	x = filefound(filename1, fnameLen1);
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	//is open? if yes return -4;
	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename1) {
			found = i; //if the file is there already, can use found later
			if (fileTable[found].open == 1) return -4; //if any instance open, fail
			if (fileTable[found].lock != 0) return -4;
			//break;
		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			break;
		}
	}

	//need to rename everything in fileTable too 
	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename1) {
			fileTable[i].filename = filename2;
		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			break;
		}
	}
	//change on directory system as well 
	int subdir = getsubdir(filename1, fnameLen1);
	char rootbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	//now just scrape through subdir until you find (or don't find) file, should find it though
	char fname = filename1[fnameLen1 - 1];
	char fname2 = filename2[fnameLen2 - 1];

	r = myDM->readDiskBlock(myfileSystemName, subdir, rootbuf); //get the buffer of the subdir
	int bufpos = 0;
	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') return -1; //should be filled in order
		if (rootbuf[bufpos] == fname && rootbuf[bufpos + 5] == 'F') {//you have found the file
			rootbuf[bufpos] = fname2;
			myPM->writeDiskBlock(subdir, rootbuf);
			break;
		}

		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
			bufpos = 0;
		}
		else bufpos += 6;
	}
	//passed all checks, now change the filename on disk and write it
	inodeBuf[0] = filename2[fnameLen2 - 1];


	return myPM->writeDiskBlock(x, inodeBuf);

}

//-1 if file DNE
//-2 if file opened or locked
//-3 if cannot be deleted for any other reason
//0 if deleted successfully 

int FileSystem::deleteFile(char* filename, int fnameLen)
{
	if (fnameLen < 2) return -3;
	for (int i = 0; i < fnameLen; i++) {
		if ((i % 2 == 0) && (filename[i] != '/')) {
			return -3;
		}
		else if ((i % 2 == 1) && (!isalpha(filename[i]))) {
			return -3;
		}
	}

	if (filefound(filename, fnameLen) == -1) return -1;

	char inodeBuf[64];
	int fileaddr;
	int x, pos, r;
	int found = -1;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';
	r = myPM->readDiskBlock(pos, inodeBuf);
	pos = 0;

	//x is file block addr
	//inodeBuf is file buffer 
	x = filefound(filename, fnameLen);
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	//is open? if yes return -2;
	for (int i = 1; i < 1000; i++) {
		if (fileTable[i].filename == filename) {
			found = i; //if the file is there already, can use found later
			if (fileTable[found].open == 1) return -2; //if any instance open, fail
			if (fileTable[found].lock != 0) return -2;
			//break;
		}
		char* hash = fileTable[i].filename;
		if (hash[1] == '#') {
			break;
		}
	}

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


	if (inodeBuf[pos] != '#') { //if the file has an indir block, set it to 
		x = myDM->charToInt(pos, inodeBuf);
		//x is now addr block of the indir
		int r = myDM->readDiskBlock(myfileSystemName, x, buff);
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
	//indirblock = getaddrs(inodeBuf);

	//now delete every addr block in Gaddr
	for (int i = 0; i < 19; i++) {

		if (Saddrarr[i] != -1) {
			r = myDM->readDiskBlock(myfileSystemName, Saddrarr[i], testbuf);
			myPM->returnDiskBlock(Saddrarr[i]);
			r = myDM->readDiskBlock(myfileSystemName, Saddrarr[i], testbuf);
		}
	}


	if (indirblock != -1) {
		r = myPM->returnDiskBlock(indirblock);
	}

	//need to 'del' from directories
	//get current dir of file 
	int direc = getsubdir(filename, fnameLen);
	char fname = filename[fnameLen - 1];

	r = myDM->readDiskBlock(myfileSystemName, direc, rootbuf); //get the buffer of the subdir
	int bufpos = 0;
	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') return -8; //shouldn't hit this
		if (rootbuf[bufpos] == fname && rootbuf[bufpos + 5] == 'F') {//you have found the file
			rootbuf[bufpos + 5] = 'X';
			r = myPM->writeDiskBlock(direc, rootbuf);
			return 0;
		}
		bufpos += 6;

		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
			bufpos = 0;
			direc = xy;
		}
	}

	//int thingy = rollbackRoot(filename, fnameLen);
		//cout << "Thing should be 0. It is: " << thingy << endl;

	//deallocate block address of file
	r = myPM->returnDiskBlock(x);

	return 0;

}

//-1 if dir DNE
//-2 if ANYTHING in directory
//-3 if cannot be deleted for any other reason
//0 if successful
int FileSystem::deleteDirectory(char* dirname, int dnameLen)
{
	int subdir = getsubdir(dirname, dnameLen);
	int r, diraddr;
	char rootbuf[64];
	char dirbuf[64];
	for (int j = 0; j < 64; j++) rootbuf[j] = '#';
	for (int j = 0; j < 64; j++) dirbuf[j] = '#';
	//now just scrape through subdir until you find (or don't find) file, should find it though
	char mydirname = dirname[dnameLen - 1];

	r = myDM->readDiskBlock(myfileSystemName, subdir, rootbuf);
	//direc traverse and test if last thing is F or D then 
	int bufpos = 0;
	int dirpos = 0;
	while (true) {//loop starting at subdir, and go through all the way
		if (rootbuf[bufpos] == '#') return -1; //should be filled in order
		if (rootbuf[bufpos] == mydirname && rootbuf[bufpos + 5] == 'D') {//you have found the dir
			diraddr = myDM->charToInt(bufpos + 1, rootbuf);
			r = myDM->readDiskBlock(myfileSystemName, diraddr, dirbuf);
			dirpos = bufpos;
			break;
		}

		if (bufpos >= 60) {//need to replace GrootDir with addr there 
			int xy = myDM->charToInt(bufpos, rootbuf);
			r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
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

		//set flag to X and call emily/Ryans cleanup 

		//need to 'del' from directories
		//get current dir of file 
		int direc = getsubdir(dirname, dnameLen);
		char fname = dirname[dnameLen - 1];

		r = myDM->readDiskBlock(myfileSystemName, direc, rootbuf); //get the buffer of the subdir
		int bufpos = 0;
		while (true) {//loop starting at subdir, and go through all the way
			if (rootbuf[bufpos] == '#') return -8; //shouldn't hit this
			if (rootbuf[bufpos] == fname && rootbuf[bufpos + 5] == 'D') {//you have found the dir
				rootbuf[bufpos + 5] = 'X';
				r = myPM->writeDiskBlock(direc, rootbuf);
				return 0;
			}
			bufpos += 6;

			if (bufpos >= 60) {//need to replace GrootDir with addr there 
				int xy = myDM->charToInt(bufpos, rootbuf);
				r = myDM->readDiskBlock(myfileSystemName, xy, rootbuf);
				bufpos = 0;
				direc = xy;
			}
		}
	}

	return 0;
}
int FileSystem::getAttribute(char* filename, int fnameLen, char* girth)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is file block addr
	//inodeBuf is file buffer 
	x = filefound(filename, fnameLen);
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);
	girth[0] = inodeBuf[23];
	return 0;
}

//returns -1 on failure
//returns 0 on good
int FileSystem::setAttribute(char* filename, int fnameLen, char girth)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is file block addr
	//inodeBuf is file buffer 
	x = filefound(filename, fnameLen);
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;

	inodeBuf[23] = girth;
	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;
	return myPM->writeDiskBlock(x, inodeBuf);

	return -1;
}

int FileSystem::getAttribute2(char* filename, int fnameLen)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is file block addr
	//inodeBuf is file buffer 
	x = filefound(filename, fnameLen);
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);
	return myDM->charToInt(24, inodeBuf);

}

//returns -1 on failure
//returns 0 on good
int FileSystem::setAttribute2(char* filename, int fnameLen, int shaft)
{
	char inodeBuf[64];
	int x, pos, r;
	for (int j = 0; j < 64; j++) inodeBuf[j] = '#';

	pos = 0;

	//x is file block addr
	//inodeBuf is file buffer 
	x = filefound(filename, fnameLen);
	r = myDM->readDiskBlock(myfileSystemName, x, inodeBuf);

	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;
	myDM->intToChar(24, shaft, inodeBuf);
	cout << "inodeBuf is: "; for (int j = 0; j < 64; j++) cout << inodeBuf[j]; cout << endl;
	return myPM->writeDiskBlock(x, inodeBuf);

	return -1;
}
