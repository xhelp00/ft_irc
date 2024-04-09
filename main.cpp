// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

class User
{
public:
	std::string nick, user, host;
	int welcome, fd;
	User(){}
	~User(){}
};


void accept_new_connection_request(int server_fd, int epoll_fd) {
	sockaddr_in new_addr;
	socklen_t addrlen = sizeof(new_addr);
	/* Accept new connections */
	int conn_sock = accept(server_fd, (struct sockaddr*)&new_addr,
						(socklen_t*)&addrlen);

	if (conn_sock == -1) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return;
		else {
			std::cerr << "accept" << std::endl;
			return;
		}
	}

	/* Make the new connection non blocking */
	fcntl(conn_sock, F_SETFL, O_NONBLOCK);

	std::string host = inet_ntoa(new_addr.sin_addr);
	std::cout << "New connection from " << host << ":" << ntohs(new_addr.sin_port) << " on FD: " << conn_sock << std::endl;

	/* Monitor new connection for read events in edge triggered mode */
	User *user = new User();
	user->host = host;
	user->welcome = 0;
	user->fd = conn_sock;

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = user;

	/* Allow epoll to monitor new connection */
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev)) {
		std::cerr << "Failed to add file descriptor to epoll" << std::endl;
		close(epoll_fd);
		return;
	}
}

int recv_message(User* user) {
	// recieving data
	// std::cout << "CALL";
	char buffer[1024];
	std::string serverName(":IRCQ+.org");
	memset(buffer, 0, sizeof(buffer));
	// std::cout << "FD: " << user->fd << std::endl;

	int bytesReceived = recv(user->fd, buffer, sizeof(buffer), 0);
	if (bytesReceived < 0) {
		std::cerr << "Error in recv(). SAD" << std::endl;
		return 1;
	}

	std::cout << "BuFFer: " << buffer;

	if (!user->welcome){
		user->welcome = 1;
		std::stringstream info(buffer);
		std::string word;
		while (info >> word){
			std::cout << "Word: " << word << std::endl;
			if (word == "NICK"){
				info >> word;
				user->nick = word;
			}
			if (word == "USER"){
				info >> word;
				user->user = word;
			}
		}
		std::string welcomeMessage(serverName + " 001 " + user->nick + " :Welcome to IRCQ+ " + user->nick + "! " + user->user + "@" + user->host + "\n");
		send(user->fd, welcomeMessage.c_str(), welcomeMessage.length(), 0);
	}
	else {
		std::stringstream info(buffer);
		std::string word;
		while (info >> word){
			std::cout << "Word: " << word << std::endl;
			if (word == "QUIT"){
				std::cout << "Client disconnected" << std::endl;
				return 0;
			}
			if (word == "die"){
				std::cout << "DIE COMMAND" << std::endl;
				exit(0);
			}
			if (word == "PING"){
				std::string pongMessage("PONG " + serverName + "\n");
				send(user->fd, pongMessage.c_str(), pongMessage.length(), 0);
			}
			if (word == "PART"){
				send(user->fd, buffer, bytesReceived, 0);

			}
			if (word == "JOIN"){
				send(user->fd, buffer, bytesReceived, 0);
			}
		}
		// send(clientSocket, buffer, bytesReceived, 0);
	}
	return 0;
}

int main() {
	// creating server listen socket
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0) {
		std::cerr << "Setting up socket failed. SAD" << std::endl;
		return 1;
	}

	// specifying the address
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8080);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	// binding socket.
	bind(serverSocket, (sockaddr*)&serverAddress,
		 sizeof(serverAddress));

	// listening to the assigned socket
	listen(serverSocket, SOMAXCONN);

	// doing epoll stuff
	struct epoll_event ev, events[5];
	int epoll_fd = epoll_create1(0);

	if (epoll_fd == -1) {
		std::cerr << "Failed to create epoll file descriptor" << std::endl;
		return 1;
	}

	User *user = new User();
	user->fd = serverSocket;

	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = user;

	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &ev)) {
		std::cerr << "Failed to add file descriptor to epoll" << std::endl;
		close(epoll_fd);
		return 1;
	}

	// recieving data
	while (true) {
		int nfds = epoll_wait(epoll_fd, events, 5, -1);
		// std::cout << "NFDS: " << nfds << std::endl;
		if (nfds == -1) {
			std::cerr << "Failed to wait for events" << std::endl;
			close(epoll_fd);
			return 1;
		}

		for (int i = 0; i < nfds; i++) {
			// int fd = events[i].data.fd;
			User *user = reinterpret_cast<User*>(events[i].data.ptr);
			// std::cout << "FD: " << events[i].data.fd << "ServerSocket:" << serverSocket << std::endl;
			if (user->fd == serverSocket)
				accept_new_connection_request(serverSocket, epoll_fd);
			else
				recv_message(user);
		}
	}

	// // recieving data
	// char buffer[1024];
	// int welcome = 0;
	// std::string nick, user, serverName(":IRCQ+.org");
	// while (true) {
	// 	int nfds = epoll_wait(epoll_fd, events, 5, -1);

	// 	if (nfds == -1) {
	// 		std::cerr << "Failed to wait for events" << std::endl;
	// 		close(epoll_fd);
	// 		return 1;
	// 	}

	// 	for (int i = 0; i < nfds; i++) {
	// 		int fd = events[i].data.fd;

	// 		if (fd == serverSocket)
	// 			accept_new_connection_request(fd);
	// 		else if ((events[i].events & EPOLLERR) ||
	//                     (events[i].events & EPOLLHUP) ||
	//                     (!(events[i].events & EPOLLIN)))
	// 					close(fd);
	// 		else {
	// 			handle_message(fd);
	// 		}
	// 	}
	// 	// memset(buffer, 0, sizeof(buffer));

	// 	// int bytesReceived = recv(client1Socket, buffer, sizeof(buffer), 0);
	// 	// if (bytesReceived < 0) {
	// 	// 	std::cerr << "Error in recv(). SAD" << std::endl;
	// 	// 	return 1;
	// 	// }

	// 	// if (bytesReceived == 0) {
	// 	// 	std::cout << "Client disconnected" << std::endl;
	// 	// 	break;
	// 	// }

	// 	// std::cout << "BuFFer: " << buffer;

	// 	// if (!welcome){
	// 	// 	welcome = 1;
	// 	// 	std::stringstream info(buffer);
	// 	// 	std::string word;
	// 	// 	while (info >> word){
	// 	// 		std::cout << "Word: " << word << std::endl;
	// 	// 		if (word == "NICK"){
	// 	// 			info >> word;
	// 	// 			nick = word;
	// 	// 		}
	// 	// 		if (word == "USER"){
	// 	// 			info >> word;
	// 	// 			user = word;
	// 	// 		}
	// 	// 	}
	// 	// 	std::string welcomeMessage(serverName + " 001 " + nick + " :Welcome to IRCQ+ " + nick + "! " + user + "@" + host1 + "\n");
	// 	// 	send(client1Socket, welcomeMessage.c_str(), welcomeMessage.length(), 0);
	// 	// }
	// 	// else {
	// 	// 	std::stringstream info(buffer);
	// 	// 	std::string word;
	// 	// 	while (info >> word){
	// 	// 		std::cout << "Word: " << word << std::endl;
	// 	// 		if (word == "QUIT"){
	// 	// 			std::cout << "Client disconnected" << std::endl;
	// 	// 			return 0;
	// 	// 		}
	// 	// 		if (word == "PING"){
	// 	// 			std::string pongMessage("PONG " + serverName + "\n");
	// 	// 			send(client1Socket, pongMessage.c_str(), pongMessage.length(), 0);
	// 	// 		}
	// 	// 		if (word == "PART"){
	// 	// 			send(client1Socket, buffer, bytesReceived, 0);

	// 	// 		}
	// 	// 		if (word == "JOIN"){
	// 	// 			send(client1Socket, buffer, bytesReceived, 0);

	// 	// 		}
	// 	// 	}
	// 	// 	// send(clientSocket, buffer, bytesReceived, 0);
	// 	// }
	// }

	// closing the server socket
	close(serverSocket);

	return 0;
}
