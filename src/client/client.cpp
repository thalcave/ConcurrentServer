#include <string>
#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include "common/network.hpp"
#include "common/protocol.hpp"


void
startClient(unsigned port, std::string const& fname, bool use_sendfile)
{
	//will throw if file does not exist/not regular/cannot open for read etc
	scoped_descriptor read_fd(openFile(fname,  O_RDONLY));
	
	struct addrinfo hints, *res;

	// first, load up address structs with getaddrinfo():
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;

	//convert port to string
	std::string port_str = boost::lexical_cast<std::string>(port);
	if (getaddrinfo("127.0.0.1", port_str.c_str(), &hints, &res))
	{
		throw std::runtime_error("getaddrinfo(): " + std::string(strerror(errno)));
	}

	// make a socket:
	scoped_descriptor sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// connect it to the address and port we passed in to getaddrinfo():
	int connect_ret = connect(sockfd.get(), res->ai_addr, res->ai_addrlen);
	if (-1 == connect_ret)
	{
		throw std::runtime_error("error connect(): " + std::string(strerror(errno)));
	}

	//send file over the network

	//first, send name (256 char)
	{
		char filename[256] = {'\0'};

		strcpy(filename, fname.c_str());
		sendBuf(sockfd.get(), filename, 256);
	}

	//receive answer
	if (getAnswer(sockfd.get()) != OK)
	{
		std::cerr<<"Error on server side, will exit\n";
		return;
	}

	//send file

	//first, send size
	struct stat mstat = {0};
	fstat(read_fd.get(), &mstat);

	char data_to_send[100];
	std::string tmp_str = boost::lexical_cast<std::string>(mstat.st_size);
	strncpy(data_to_send, tmp_str.c_str(), 100);
	//memcpy(&data_to_send, &mstat.st_size, sizeof(mstat.st_size));
	sendBuf(sockfd.get(), data_to_send, 100);

	if (!use_sendfile)
	{
		const unsigned BUF_LEN = 65536;
		char buffer[BUF_LEN];

		ssize_t read_bytes;
		do
		{
			memset(buffer, '\0', BUF_LEN);
			//read from file
			read_bytes = read(read_fd.get(), buffer, BUF_LEN);
			if (-1 == read_bytes && (errno != EAGAIN && errno != EINTR))
			{
				throw std::runtime_error("read(): " + std::string(strerror(errno)));
			}

			//send over network
			sendBuf(sockfd.get(), buffer, BUF_LEN);

		} while (read_bytes);
	}
	else	//use sendfile
	{
		ssize_t sent_bytes = ::sendfile(sockfd.get(), read_fd.get(), NULL, mstat.st_size);
		if (-1 == sent_bytes)
		{
			::shutdown(sockfd.get(), SHUT_RDWR);
			::close(sockfd.get());
			throw std::runtime_error("sendfile(): " + std::string(strerror(errno)));
		}
	}

	//receive answer
	std::string answer = getAnswer(sockfd.get());
	if (answer != OK)
	{
		std::cerr<<"Error on server side, will exit: ["<<answer<<"]\n";
		return;
	}


	std::cout<<"File transferred successfully; exiting...\n";
}

int main(int argc, char* argv[])
{
	if (argc < 3 || argc > 4)
	{
		std::cerr<<"usage: ./client <port> <filename> [1 - use sendfile]\n";
		return -1;
	}

	unsigned port;
	try {
		port = boost::lexical_cast<unsigned>(argv[1]);
	} catch(boost::bad_lexical_cast const& be) {
		std::cerr<<"cannot convert to unsigned : "<<argv[1]<<"\n";
		return -2;
	}
	
	bool use_sendfile(false);
	if (4 == argc)
	{
		use_sendfile = true;
	}

	try
	{
		startClient(port, std::string(argv[2]), use_sendfile);
	} catch(std::runtime_error const& rte) {
		std::cerr<<"Error: "<<rte.what()<<"\n";
		return -3;
	} catch (std::exception const& exc) {
		std::cerr<<"Error: "<<exc.what()<<"\n";
		return -4;
	} catch (...) {
		std::cerr<<"Unknown exception\n";
		return -5;
	}

	return 0;
}