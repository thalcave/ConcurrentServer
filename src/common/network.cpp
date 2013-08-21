#include "network.hpp"

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdexcept>
#include <string.h>
#include <boost/lexical_cast.hpp>


void
sendBuf(int sockfd, const void* buf, size_t buf_len)
{
	int res = 0;
	size_t tot = 0;

	while (tot < buf_len)
	{
		res = ::send(sockfd, reinterpret_cast<const char *>(const_cast<void*>(buf))+tot, buf_len-tot, MSG_NOSIGNAL);
		if(res == -1 && errno == EINTR)	//nothing was sent because a signal interrupted ::send; try again
			continue;

		if (res == -1 && errno != EAGAIN)
		{
			throw std::runtime_error("send(): " + std::string(strerror(errno)));
		}

		if (res > 0)	//some chars were sent before ::send was interrupted; we have to send the remaining part of buf
		{
			tot += res;
		}
	};
}


int
createFile(std::string const& fname)
{
	int fd = open(fname.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
	if (-1 == fd)
	{
		throw std::runtime_error("open mode(" + fname + "): error: " + std::string(strerror(errno)));
	}

	return fd;
}

int
openFile(std::string const& fname, int flags)
{
	int fd = open(fname.c_str(), flags);
	if (-1 == fd)
	{
		throw std::runtime_error("open(" + fname + "): error: " + std::string(strerror(errno)));
	}

	return fd;
}

namespace
{
size_t
readBuf(int sockfd, void* buf, size_t buf_len)
{
	memset(buf, '\0', buf_len);

	int nread = ::recv(sockfd, buf, buf_len, 0);
	
	if (!nread && buf_len)	//received EOF while expecting to read something (something wrong happend at the other end)
	{
		throw std::runtime_error("recvBuf(): peer has performed orderly shutdown");
	}
	
//	std::cout<<"ReadBuf recv: "<<nread<<"\n";

	if (nread < 0)
	{
		throw std::runtime_error("recv(): " + std::string(strerror(errno)));
	}

	return nread;
	
	/*while (read < buf_len)
	{
		std::cout<<"read: "<<read<<"\n";
		int nread = ::recv(sockfd, buf+read, buf_len, 0);
		if (nread <= 0)
		{
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)	//interrupted by signal
			{
				continue;
			}

			if (nread < 0)
			{
				throw std::runtime_error("recv(): " + std::string(strerror(errno)));
			}
			//nread = 0 --> end
			break;
		}

		read += nread;

		std::cout<<"read: "<<read<<"\n";
		std::cout<<"nread: "<<nread<<"\n";


		if (!nread)
		{
			break;
		}
	}*/

	//return read;
}
}

void
recvBuf(int sockfd, void* buf, size_t buf_len)
{
	//we know the exact size
	size_t read = readBuf(sockfd, buf, buf_len);

	while (read != buf_len)
	{
		std::string read_str = boost::lexical_cast<std::string>(read);
		std::string buf_str = boost::lexical_cast<std::string>(buf_len);

		throw std::runtime_error("recvBuf(): expected: " + buf_str + " read: "+read_str);
	}
}




void
saveFile(int src, int dst, size_t fsize)
{
	size_t remaining = fsize;
	const size_t BUF_LEN = 65536;
	char buffer[BUF_LEN];

	size_t read;
	do
	{
		//std::cout<<"save file; remaining: "<<remaining<<"\n";

		size_t min_read = std::min(remaining, BUF_LEN);
		read = readBuf(src, buffer, min_read);
		//std::cout<<"save file; read :"<<read<<"\n";

		remaining -= read;
		//std::cout<<"save file; remaining: "<<remaining<<"\n";

		
		int res = 0;
		size_t tot = 0;

		while (tot < read)
		{
			res = ::write(dst, buffer+tot, read-tot);
			if(res == -1 && errno == EINTR)	//nothing was written because a signal interrupted ; try again
				continue;

			if (res == -1 && errno != EAGAIN)
			{
				throw std::runtime_error("write(): " + std::string(strerror(errno)));
			}

			if (res > 0)	//some chars were written; we have to write the remaining part of buf
			{
				tot += res;
			}
		}
	} while (remaining>0);

	//std::cout<<"exit function\n";
}


std::string
getAnswer(int sockfd)
{
	char buffer[2];
	//std::cout<<"buffer len: ["<<strlen(buffer)<<"]\n";
	recvBuf(sockfd, buffer, 3);

	/*std::cout<<"buffer: ["<<buffer<<"]\n";
	std::cout<<"buffer len: ["<<strlen(buffer)<<"]\n";
	std::cout<<"ch: ["<<(int)buffer[strlen(buffer)-1]<<"]\n";*/

	std::string bufstr(buffer);
	if (bufstr.empty())
	{
		throw std::runtime_error("got empty answer");
	}

	return bufstr;
}
