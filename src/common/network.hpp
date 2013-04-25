#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void
sendBuf(int sockfd, const void* buf, size_t buf_len);

int
openFile(std::string const& fname, int flags);

int
createFile(std::string const& fname);

void
recvBuf(int sockfd, void* buf, size_t buf_len);

struct scoped_descriptor
{
	scoped_descriptor(int f): fd(f) {}
	~scoped_descriptor() { close(fd); }

	int get() const { return fd; }
	int fd;
};

void
saveFile(int src, int dst, size_t fsize);

std::string
getAnswer(int sockfd);


#endif

