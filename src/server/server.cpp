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
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "common/network.hpp"
#include "common/protocol.hpp"

class ParentExit: public std::runtime_error
{
public:
	ParentExit():std::runtime_error("") {}
	~ParentExit() throw() {}
};

void
handleConnection(int sock)
{
	/*int opts;

	opts = fcntl(sock, F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(EXIT_FAILURE);
	}
	
	opts = opts & (~O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
	}*/

	
	scoped_descriptor sockfd = sock;

	//receive filename from client
	char fname[256];
	recvBuf(sockfd.get(), fname, 256);	//expect 256

	int tmp_fd;
	try
	{
		tmp_fd = createFile(fname);
	} catch(...) {
		//send OK to server
		sendBuf(sockfd.get(), FAIL.c_str(), MSG_SIZE+1);
		throw;
	}

	scoped_descriptor fd = tmp_fd;

	//send OK to server
	sendBuf(sockfd.get(), OK.c_str(), MSG_SIZE+1);

	//receive file size
	
	char buffer[100];
	memset(buffer, '\0', 100);
	recvBuf(sockfd.get(), buffer, 100);
	size_t fsize = boost::lexical_cast<size_t>(std::string(buffer));

	std::cout<<"receive file size: "<<fsize<<"\n";

	//receive file content and save it
	saveFile(sockfd.get(), fd.get(), fsize);

	close(fd.get());

	std::cout<<"File saved: "<<fname<<"\n";
	sendBuf(sockfd.get(), OK.c_str(), MSG_SIZE+1);

	//exit
	exit(EXIT_SUCCESS);
}

void
spawn(int sockfd, int lsock)
{
	pid_t npid = fork();
	if (-1 == npid)
	{
		throw std::runtime_error("error creating socket: " + std::string(strerror(errno)));
	}

	std::cout<<"fork successful\n";

	if (npid)	//parent
	{
		std::cout<<"Parent will return to loop; child: "<<npid<<"\n";
		throw ParentExit();
	}

	//child
	close(lsock);
	handleConnection(sockfd);
}

void
reapChildren()
{
	int status;
	pid_t pid;

	while((pid = ::waitpid(-1, &status, WNOHANG)) > 0) {
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status)) {
				std::cout<<"Child " << pid << " exited with non-zero exit code: " << WEXITSTATUS(status)<<"\n";
			} else {
				std::cout<<"Child " << pid << " exited normally.\n";
			}
		} else if (WIFSIGNALED(status)) {
			std::cout<<"Child " << pid << " exited with signal: " << WTERMSIG(status)<<"\n";
		} else {
			std::cout<<"Child " << pid << " exited abnormally.\n";
		}
	}
}

void
startServer(unsigned port)
{
	//socket, bind, listen, accept
	//create socket, close on exec
	int sockfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);

	if (-1 == sockfd)
	{
		throw std::runtime_error("error creating socket: " + std::string(strerror(errno)));
	}
	
	//set SO_REUSEADDR so that bind will not fail because of TIME_WAIT state
	int optval = 1;
        if (-1 == ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
		throw std::runtime_error("setsockopt() failed: " + std::string(strerror(errno)));

	//bind
	sockaddr_in m_addr;
	m_addr.sin_family = AF_INET;
	//m_addr.sin_addr.s_addr = INADDR_ANY;
	//convert IPv4 and IPv6 addresses from text to binary form
	inet_pton(AF_INET, "127.0.0.1", &m_addr.sin_addr);
	m_addr.sin_port = htons (port);

	int bind_return = bind(sockfd, (struct sockaddr *) &m_addr, sizeof (m_addr));
	if (bind_return == -1)
	{
		throw std::runtime_error("error bind(): " + std::string(strerror(errno)));
	}


	//listen, max 10 conn
	//marks  the  socket  referred to by sockfd as a passive socket (accepting)
	int listen_ret = listen(sockfd, 10);
	if (-1 == listen_ret)
	{
		throw std::runtime_error("error listen(): " + std::string(strerror(errno)));
	}

	//accept connections on sockfd
	std::cout<<"accepting connections on "<<sockfd<<"\n";
	while (1)
	{
		reapChildren();
		
		int new_sock = ::accept(sockfd, NULL, NULL);
		if (new_sock < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				struct timeval timeout = {0};
				timeout.tv_sec = 1;

				select(0, 0, 0, 0, &timeout);
				//no connections are present, will sleep a bit
				continue;
			}
			if (errno != EINTR)
			{
				throw std::runtime_error("error accept(): " + std::string(strerror(errno)));
			}
		}
		else
		{
			try {
				std::cout<<"accepted connection, new sockfd: "<<new_sock<<"\n";
				spawn(new_sock, sockfd);
			} catch (ParentExit const& pe) {
				//do nothing
			}
		}


	}
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr<<"usage: ./server <port>\n";
		return -1;
	}

	unsigned port;
	try {
		port = boost::lexical_cast<unsigned>(argv[1]);
	} catch(boost::bad_lexical_cast const& be) {
		std::cerr<<"cannot convert to unsigned : "<<argv[1]<<"\n";
		return -2;
	}

	try
	{
		startServer(port);
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