all:
	g++ main.cc -lz -o main.o
clean:
	rm compress.z decompress.txt main.o 

requirements:
	sudo apt-get install zlib1g-dev
