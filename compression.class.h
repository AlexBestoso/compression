#define COMPRESSION_CHUNK 16384
#include <zlib.h>
//#include <fcntl.h>
//#include <sys/stat.h>
#include <cassert>
class Compression{
	private:
		size_t readMemory(unsigned char *data, size_t dataSize, unsigned char *out, size_t outSize, size_t offset ){
			size_t ret=0;
			for(int i=offset, o=0; i<dataSize && o<outSize; i++, o++, ret++){
				out[o] = data[i];
			}
			return ret;
		}
	public:

	bool zlibDeflate(char *src, size_t srcSize, char *dst, size_t *dstSize, int level){
                if(src == NULL || srcSize <= 0){
                        printf("zlibInflate(char *src, size_t srcSize, char *dst, size_t *dstSize) : invalid source.\n");
                        return false;
                }
                if(dst != NULL){
			delete[] dst;
                }
		int error=0;
                int flush=0;
                unsigned have;
                z_stream strm;
                int inFd=0, outFd=0;
                unsigned char in[COMPRESSION_CHUNK];
                unsigned char out[COMPRESSION_CHUNK];
		size_t outSize=0;

		strm.zalloc = Z_NULL;
                strm.zfree = Z_NULL;
                strm.opaque = Z_NULL;
                error = deflateInit(&strm, level);
                if(error != Z_OK){
                        printf("Compression::deflate() - failed to deflate '0x%x'\n", error);
                        close(inFd);
                        return false;
                }

		std::string grabber="";
		for(int i=0; i<srcSize; i+=COMPRESSION_CHUNK){
                        strm.avail_in = this->readMemory((unsigned char *)src, srcSize, in, COMPRESSION_CHUNK, i);
                        if(strm.avail_in == -1){
				printf("failed to read in deflate memory\n");
                                (void)deflateEnd(&strm);
                                return false;
                        }
                        flush = (i+COMPRESSION_CHUNK) < srcSize ? Z_NO_FLUSH : Z_FINISH;
                        strm.next_in = in;
                        do{
                                strm.avail_out = COMPRESSION_CHUNK;
                                strm.next_out = out;
                                error = deflate(&strm, flush);
                                assert(error != Z_STREAM_ERROR);
                                have = COMPRESSION_CHUNK - strm.avail_out;
				for(int j=0; j<have; j++){
                                        grabber += out[j];
                                        outSize++;
                                }
                        }while(strm.avail_out == 0);
                        assert(strm.avail_in == 0);
                }
		if(outSize > dstSize[0]){
			printf("Buffer overflow\n");
			return false;
		}

		dstSize[0] = outSize;
                for(int i=0; i<outSize; i++){
                        dst[i] = grabber[i];
		}
                (void)deflateEnd(&strm);
		return true;
	}

	bool zlibInflate(char *src, size_t srcSize, char *dst, size_t dstSize){
		if(src == NULL || srcSize <= 0){
			printf("zlibInflate(char *src, size_t srcSize, char *dst, size_t *dstSize) : invalid source.\n");
			return false;
		}
		if(dst == NULL || dstSize <= 0){
			printf("inflate : invalid destination buffer.\n");
			return false;
		}

		z_stream strm;
		int error;
		unsigned have;
		unsigned char in[COMPRESSION_CHUNK];
                unsigned char out[COMPRESSION_CHUNK];
		
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		
		error = inflateInit(&strm);
		if(error != Z_OK){
			printf("Compression DMA inflate : failed to init inflation stream.\n");
			return  false;
		}

		std::string grabber = "";
		size_t outsize=0;
		for(int i=0; i<srcSize; i+=COMPRESSION_CHUNK){
			strm.avail_in = this->readMemory((unsigned char *)src, srcSize, in, COMPRESSION_CHUNK, i);
			if(strm.avail_in == 0) break;
                        strm.next_in = in;
                        do{
                                strm.avail_out = COMPRESSION_CHUNK;
                                strm.next_out = out;
                                error = inflate(&strm, Z_NO_FLUSH);
                                assert(error != Z_STREAM_ERROR);
                                switch (error) {
                                        case Z_NEED_DICT:
						printf("need dict error.\n");
                                                error = Z_DATA_ERROR;     /* and fall through */
                                        case Z_DATA_ERROR:
                                        case Z_MEM_ERROR:
                                                (void)inflateEnd(&strm);
						printf("MEM/DATA error\n");
                                                return false;
                                }

                                have = COMPRESSION_CHUNK - strm.avail_out;
                                //if(this->readMemory(in, COMPRESSION_CHUNK, out, have, 0) != have){
                                //        printf("failed to read memory from zlib inflation\n");
                                //        (void)inflateEnd(&strm);
                                //        return false;
                                //}
				for(int j=0; j<have; j++){
					grabber += out[j];
					outsize++;
				}
                        }while(strm.avail_out ==  0);
                }
		(void)inflateEnd(&strm);
		if(outsize != dstSize)
			return false;
		for(int i=0; i<dstSize; i++)
			dst[i] = grabber[i];
		return true;
	}

	bool zlibInflate(std::string src, std::string dst){
		int error;
		unsigned have;
		z_stream strm;
		unsigned char in[COMPRESSION_CHUNK];
		unsigned char out[COMPRESSION_CHUNK];

		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		error = inflateInit(&strm);
		if(error != Z_OK){
			printf("Compression::inflat() - failed to init inflation stream\n");
			return false;
		}

		int inFd=0, outFd=0;
		inFd = open(src.c_str(), O_RDONLY);
		if(!inFd){
			perror("open");
			(void)inflateEnd(&strm);
			return false;
		}
		outFd = open(dst.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(!outFd){
			perror("open");
			(void)inflateEnd(&strm);
			close(inFd);
			return false;
		}

		size_t srcSize=0;
		struct stat st;
		if(fstat(inFd, &st) == -1){
			perror("fstat");
			(void)inflateEnd(&strm);
                        close(inFd);
			close(outFd);
			return false;
		}
		srcSize = st.st_size;

		for(int i=0; i<srcSize; i+=COMPRESSION_CHUNK){
			strm.avail_in = pread(inFd, in, COMPRESSION_CHUNK, i);
			if(strm.avail_in == -1){
				perror("pread");
				(void)inflateEnd(&strm);
                        	close(inFd);
                        	close(outFd);
				return false;
			}
			if(strm.avail_in == 0) break;
			strm.next_in = in;
			do{
				strm.avail_out = COMPRESSION_CHUNK;
				strm.next_out = out;
				error = inflate(&strm, Z_NO_FLUSH);
				assert(error != Z_STREAM_ERROR);
				switch (error) {
	            			case Z_NEED_DICT:
         	       				error = Z_DATA_ERROR;     /* and fall through */
            				case Z_DATA_ERROR:
            				case Z_MEM_ERROR:
                				(void)inflateEnd(&strm);
						close(inFd);
                                		close(outFd);
                				return false;
            			}

				have = COMPRESSION_CHUNK - strm.avail_out;
				if(write(outFd, out, have) != have){
					perror("write");
					(void)inflateEnd(&strm);
                                        close(inFd);
                                        close(outFd);
                                        return false;
				}
			}while(strm.avail_out ==  0);
		}		
		
		(void)inflateEnd(&strm);
                close(inFd);
		close(outFd);
		return true;
	}

	bool zlibDeflate(std::string src, std::string dst, int level){
		int error=0;
		int flush=0;
		unsigned have;
		z_stream strm;
		int inFd=0, outFd=0;
		unsigned char in[COMPRESSION_CHUNK];
		unsigned char out[COMPRESSION_CHUNK];
		
		inFd = open(src.c_str(), O_RDONLY);
		if(!inFd){
			printf("failed to open '%s'\n", src.c_str());
			return false;
		}
		
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		error = deflateInit(&strm, level);
		if(error != Z_OK){
			printf("Compression::deflate() - failed to deflate '0x%x'\n", error);
			close(inFd);
			return false;
		}

		size_t srcSize=0;
		struct stat srcSt;
		if(fstat(inFd, &srcSt) == -1){
			printf("Failed to get file stats '%s'\n", src.c_str());
			(void)deflateEnd(&strm);
			close(inFd);
			return false;
		}
		srcSize = srcSt.st_size;
		
		outFd = open(dst.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if(!outFd){
			perror("open");
			(void)deflateEnd(&strm);
                        close(inFd);
			return false;
		}

		for(int i=0; i<srcSize; i+=COMPRESSION_CHUNK){
			strm.avail_in = pread(inFd, in, COMPRESSION_CHUNK, i);
			if(strm.avail_in == -1){
				perror("pread");
				(void)deflateEnd(&strm);
				close(inFd);
				close(outFd);
				return false;
			}
			flush = (i+COMPRESSION_CHUNK) < srcSize ? Z_NO_FLUSH : Z_FINISH;
			strm.next_in = in;
			do{
				strm.avail_out = COMPRESSION_CHUNK;
				strm.next_out = out;
				error = deflate(&strm, flush);
				assert(error != Z_STREAM_ERROR);
				have = COMPRESSION_CHUNK - strm.avail_out;
				if(write(outFd, out, have) < 0){
					perror("write");
					(void)deflateEnd(&strm);
                                	close(inFd);
                                	close(outFd);
                                	return false;
				}
				
			}while(strm.avail_out == 0);
			assert(strm.avail_in == 0);
		}
		(void)deflateEnd(&strm);
		close(inFd);
		close(outFd);
		return true;
	}
};
