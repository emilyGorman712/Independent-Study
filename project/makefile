CC=g++
#CFLAGS = -ggdb 
CFLAGS = 

all: driver

 
bitvector.o: bitvector.cpp  bitvector.h
	$(CC) $(CFLAGS) -c bitvector.cpp

bitvector_driver: bitvector.o
	$(CC) $(CFLAGS) bitvector_driver.cpp bitvector.o -o bitvector_driver

disk.o: disk.cpp  disk.h
	$(CC) $(CFLAGS) -c disk.cpp

diskmanager.o: diskmanager.cpp  diskmanager.h
	$(CC) $(CFLAGS) -c diskmanager.cpp

partitionmanager.o: partitionmanager.cpp  partitionmanager.h
	$(CC) $(CFLAGS) -c partitionmanager.cpp

recordmanagement.o: recordmanagement.cpp  recordmanagement.h
	$(CC) $(CFLAGS) -c recordmanagement.cpp

driver: driver.cpp client.h bitvector.o disk.o diskmanager.o partitionmanager.o recordmanagement.o
	$(CC)  -o driver driver.cpp bitvector.o disk.o diskmanager.o partitionmanager.o recordmanagement.o

clean: 
	rm -f bitvector.o disk.o diskmanager.o partitionmanager.o recordmanagement.o driver bitvector_driver DISK1
