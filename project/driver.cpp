
/* A driver to test the filesystem */

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "recordmanagement.h"
#include "client.h"
using namespace std;

int main()
{
	Disk* d = new Disk(300, 64, const_cast<char*>("DISK1"));
	DiskPartition* dp = new DiskPartition[3];

	dp[0].partitionName = 'A';
	dp[0].partitionSize = 100;
	dp[1].partitionName = 'B';
	dp[1].partitionSize = 75;
	dp[2].partitionName = 'C';
	dp[2].partitionSize = 105;

	DiskManager* dm = new DiskManager(d, 3, dp);
	RecordManagement* rm1 = new RecordManagement(dm, 'A');
	RecordManagement* rm2 = new RecordManagement(dm, 'B');
	RecordManagement* rm3 = new RecordManagement(dm, 'C');
	Client* c1 = new Client(rm1);
	Client* c2 = new Client(rm1);
	Client* c3 = new Client(rm1);
	Client* c4 = new Client(rm2);
	Client* c5 = new Client(rm2);

	c1->myRM->createRecord(const_cast<char*>("/a"), 2);
	c1->myRM->createRecord(const_cast<char*>("/b"), 2);
	c2->myRM->createRecord(const_cast<char*>("/a"), 2);
	c4->myRM->createRecord(const_cast<char*>("/a"), 2);
	int fd = c2->myRM->openRecord(const_cast<char*>("/b"), 2, 'w', -1);
	c2->myRM->writeRecord(fd, const_cast<char*>("aaaabbbbcccc"), 12);
	c2->myRM->closeRecord(fd);

	return 0;
}
