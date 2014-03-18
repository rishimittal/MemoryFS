all : MemoryFS_rw
	
MemoryFS_rw : 		memfs_rw.o utility_rw.o attributes_rw.o
	      		g++ -Wall -g -O0 -D_FILE_OFFSET_BITS=64 MemoryFS_rw memfs_rw.cpp utlity_rw.cpp attributes_rw.cpp

memfs_rw.o :  		memfs_rw.cpp mem_header_rw.h
	      		g++ -Wall -c -O0 -D_FILE_OFFSET_BITS=64 memfs_rw.cpp 

utility_rw.o :		utility_rw.cpp mem_header_rw.h
	      		g++ -Wall -g -O0 -D_FILE_OFFSET_BITS=64 utlity_rw.cpp 

attributes_rw.o : 	attributes_rw.cpp mem_header_rw.h
			g++ -Wall -g -O0 -D_FILE_OFFSET_BITS=64 attributes_rw.cpp 
 
clean:
			rm -f *.o MemoryFS_rw
