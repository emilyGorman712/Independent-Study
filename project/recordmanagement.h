class recordLO {
public:
    char* recordname;
    int lock; //0 for unlocked, some unique number otherwise
    int open; //1 for open 0 for closed
    int rw; //rw pointer 
    char mode; //r, w, m    for read, write, both
};

class RecordManagement {
    DiskManager* myDM;
    PartitionManager* myPM;
    char myrecordManagementName;
    int myrecordManagementSize;
    int locker;


    /* declare other private members here */

    recordLO recordTable[1000];
    int Gaddrarr[19];
    int GrootDirAddr;

public:
    RecordManagement(DiskManager* dm, char recordManagementName);
    int createRecord(char* recordname, int rnameLen);
    int createDirectory(char* dirname, int dnameLen);
    int lockRecord(char* recordname, int rnameLen);
    int unlockRecord(char* recordname, int rnameLen, int lockId);
    int deleteRecord(char* recordname, int rnameLen);
    int deleteDirectory(char* dirname, int dnameLen);
    int openRecord(char* recordname, int rnameLen, char mode, int lockId);
    int closeRecord(int recordDesc);
    int readRecord(int recordDesc, char* data, int len);
    int updateRecord(int recordDesc, char* data, int len);
    int appendRecord(int recordDesc, char* data, int len);
    int seekRecord(int recordDesc, int offset, int flag);
    int renameRecord(char* recordname1, int rnameLen1, char* recordname2, int rnameLen2);
    int getAttribute(char* recordname, int rnameLen, char* girth);
    int setAttribute(char* recordname, int rnameLen, char girth);
    int setAttribute2(char* recordname, int rnameLen, int shaft);
    int getAttribute2(char* recordname, int rnameLen);

    /* declare other public members here */
    int recordfound(char* recordname, int len);
    int getaddrs(char* inodeBuf);
    void addAddr(int recordblock);
    int getsubdir(char* recordname, int len);
    int rollbackRoot(char* recordname, int rnameLen);

};

