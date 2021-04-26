
/* Driver 6*/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "disk.h"
#include "diskmanager.h"
#include "partitionmanager.h"
#include "filesystem.h"
#include "client.h"

using namespace std;

/*
  This driver will test the getAttributes() and setAttributes()
  functions. You need to complete this driver according to the
  attributes you have implemented in your file system, before
  testing your program.


  Required tests:
  get and set on the fs1 on a file
    and on a file that doesn't exist
    and on a file in a directory in fs1
    and on a file that doesn't exist in a directory in fs1

 fs2, fs3
  on a file both get and set on both fs2 and fs3

  samples are provided below.  Use them and/or make up your own.


*/

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
    FileSystem* fs1 = new FileSystem(dm, 'A');
    FileSystem* fs2 = new FileSystem(dm, 'B');
    FileSystem* fs3 = new FileSystem(dm, 'C');
    Client* c1 = new Client(fs1);
    Client* c2 = new Client(fs2);
    Client* c3 = new Client(fs3);
    Client* c4 = new Client(fs1);
    Client* c5 = new Client(fs2);

    int i, r, l1, l2, f1, f2, f3, f4, f5;
    char buf1[1], buf2[64], buf3[100], buf4[600];
    char rbuf1[37], rbuf2[64], rbuf3[100], rbuf4[600];

    buf2[0] = 's';

    for (i = 0; i < 100; i++) buf3[i] = 'z';
    for (i = 0; i < 600; i++) buf4[i] = 'h';


    //What every need to show your set and get Attributes functions work

    r = c1->myFS->setAttribute(const_cast<char*>("/a"), 2, 'a');
    cout << r << endl;
    r = c4->myFS->setAttribute(const_cast<char*>("/b"), 2, 'r');
    cout << r << endl;
    r = c1->myFS->getAttribute(const_cast<char*>("/a"), 2, buf1);
    cout << "attrbute girth should be: 'a', it is: " << buf1[0] << endl;
    r = c4->myFS->getAttribute(const_cast<char*>("/b"), 2, buf1);
    cout << "attrbute girth should be: 'r', it is: " << buf1[0] << endl;

    r = c1->myFS->setAttribute2(const_cast<char*>("/a"), 2, 12);
    cout << r << (r == 0 ? " correct" : " fail") << endl;
    r = c1->myFS->getAttribute2(const_cast<char*>("/a"), 2);
    cout << "attrbute shaft should be: 12, it is: " << r << endl;

    r = c4->myFS->setAttribute2(const_cast<char*>("/b"), 2, 13);
    cout << r << (r == 0 ? " correct" : " fail") << endl;
    r = c4->myFS->getAttribute2(const_cast<char*>("/b"), 2);
    cout << "attrbute shaft should be: 13, it is: " << r << endl;


    /* r = c1->myFS->getAttributes(const_cast<char *>("/p"), ...);  //should failed!
     cout << ...
     r = c4->myFS->setAttributes(const_cast<char *>("/p"), ...);  //should failed!
     cout << ...

     r = c2->myFS->setAttributes(const_cast<char *>("/f"), ...);
     cout << ...
     r = c5->myFS->setAttributes(const_cast<char *>("/z"), ...);
     cout << ...
     r = c2->myFS->getAttributes(const_cast<char *>("/f"), ...);
     cout << ...
     r = c5->myFS->getAttributes(const_cast<char *>("/z"), ...);
     cout << ...

     r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
     cout << ...
     r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/d"), ...);
     cout << ...
     r = c3->myFS->getAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
     cout << ...
     r = c3->myFS->getAttributes(const_cast<char *>("o/o/o/a/d"), ...);
     cout << ...

     r = c2->myFS->setAttributes(const_cast<char *>("/f"), ...);
     cout << ...
     r = c5->myFS->setAttributes(const_cast<char *>("/z"), ...);
     cout << ...
     r = c2->myFS->getAttributes(const_cast<char *>("/f"), ...);
     cout << ...
     r = c5->myFS->getAttributes(const_cast<char *>("/z"), ...);
     cout << ...

     r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
     cout << ...
     r = c3->myFS->setAttributes(const_cast<char *>("/o/o/o/a/d"), ...);
     cout << ...
     r = c3->myFS->getAttributes(const_cast<char *>("/o/o/o/a/l"), ...);
     cout << ...
     r = c3->myFS->getAttributes(const_cast<char *>("o/o/o/a/d"), ...);
     cout << ...
   */
    return 0;
}
