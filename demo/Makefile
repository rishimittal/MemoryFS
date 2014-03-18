all : MemoryFS_rw
	
MemoryFS_rw : 		memfs_rw.o utility_rw.o attributes_rw.o
	      		g++ -O0 -D_FILE_OFFSET_BITS=64 -o MemoryFS_rw memfs_rw.cpp utility_rw.cpp attributes_rw.cpp

memfs_rw.o :  		memfs_rw.cpp mem_header_rw.h
	      		g++ -c -O0 -D_FILE_OFFSET_BITS=64 memfs_rw.cpp 

utility_rw.o :		utility_rw.cpp mem_header_rw.h
	      		g++ -c -O0 -D_FILE_OFFSET_BITS=64 utility_rw.cpp 

attributes_rw.o : 	attributes_rw.cpp mem_header_rw.h
			g++ -c -O0 -D_FILE_OFFSET_BITS=64 attributes_rw.cpp 
 
clean:
			rm -rf *.o MemoryFS_rw
