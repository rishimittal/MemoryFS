all:
	g++ -Wall -g -O0 -D_FILE_OFFSET_BITS=64 memfs.cpp -o MemoryFS_r -lfuse	
	g++ -Wall -g -O0 -D_FILE_OFFSET_BITS=64 memrwfs.cpp -o MemoryFS_rw -lfuse
clean:
	rm -f memrwfs.o MemoryFS_rw
	rm -f memfs.o MemoryFS_r
