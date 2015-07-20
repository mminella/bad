all: read write random_read random_write async_random_read async_read
LIB_FLAGS = -lrt
%.o: %.cc
	g++ -O3 -c $< -o $@
read: read.o
	g++ $< -o $@ $(LIB_FLAGS)
write: write.o
	g++ $< -o $@ $(LIB_FLAGS)
random_write: random_write.o
	g++ $< -o $@ $(LIB_FLAGS)
random_read: random_read.o
	g++ $< -o $@ $(LIB_FLAGS)
async_random_read: async_random_read.o
	g++ $< -o $@ $(LIB_FLAGS)
async_read: async_read.o
	g++ $< -o $@ $(LIB_FLAGS)
clean:
	\rm -f *.o
	\rm -f read write random_read random_write async_random_read
