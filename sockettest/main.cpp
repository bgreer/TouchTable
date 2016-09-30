#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

void client_mode (void)
{
	int fd;
	struct sockaddr_un addr;
	std::string add_name = "\\dev\\touch\0";
	std::string msg = "lol";

	// AF_UNIX means unix-based communication
	// SOCK_DGRAM means UDP protocol
	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	{
		std::cout << "Could not create socket!" << std::endl;
		return;
	}

	// bind to an address
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, add_name.c_str(), add_name.size());
	std::cout << "Binding to " << addr.sun_path << std::endl;
	if (bind(fd, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
	{
		std::cout << "Could not bind to port!" << std::endl;
		close(fd);
		return;
	}



	std::cout << "Sending packet..." << std::endl;
	if (sendto(fd, msg.c_str(), msg.size(), 0, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
	{
		std::cout << "Could not send packet!" << std::endl;
		close(fd);
		return;
	}


	// close socket
	close(fd);

}


void server_mode (void)
{
	int fd;
	struct sockaddr_un addr;
	std::string add_name = "\\dev\\touch\0";
	int buffsize = 128;
	unsigned char *buff = new unsigned char [buffsize];

	// AF_UNIX means unix-based communication
	// SOCK_DGRAM means UDP protocol
	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	{
		std::cout << "Could not create socket!" << std::endl;
		return;
	}

	// bind to an address
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, add_name.c_str(), add_name.size());
	std::cout << "Binding to " << addr.sun_path << std::endl;
	if (bind(fd, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
	{
		std::cout << "Could not bind to port!" << std::endl;
		close(fd);
		return;
	}


	std::cout << "Waiting for packet.." << std::endl;
	while (recvfrom(fd, buff, buffsize, 0, 0, 0) == 0)
	{
		usleep(1000);
	}

	std::cout << "Received packet: " << buff << std::endl;

	delete [] buff;

	// close socket
	close(fd);
}

int main (int argc, char *argv[])
{
	
	// pick which mode to run in
	if (argc < 2)
	{
		std::cout << "Too few arguments!" << std::endl;
		return -1;
	}
	if (atoi(argv[1]) == 0)
	{
		std::cout << "Running in Client Mode" << std::endl;
		client_mode();
	} else {
		std::cout << "Running in Server Mode" << std::endl;
		server_mode();
	}

	return EXIT_SUCCESS;
}
