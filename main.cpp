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
#include <vector>
#include <algorithm>

int serverSocket;

class Channel;
// class for user info storrage
class User
{
public:
	std::string _nick, _user, _host;
	Channel* _nowAt;
	int _welcome, _fd;
	User(std::string nick, std::string user, std::string host) : _nick(nick), _user(user), _host(host), _nowAt(NULL), _welcome(0), _fd(0){}
	~User(){}
};

class Channel {
public:
	std::string _name, _topic;
	std::vector<User*> _users;
	uint32_t _nUsers, _maxUsers;
	Channel(std::string name, std::string topic, int maxUsers) : _name(name), _topic(topic), _nUsers(0), _maxUsers(maxUsers){}
	~Channel(){}
};

/*
	accept_new_connection_request

	Accepts a new connection from a client
	creates a new user class and
	adds the client socket to epoll
*/
void accept_new_connection_request(int server_fd, int epoll_fd) {
	sockaddr_in new_addr;
	socklen_t addrlen = sizeof(new_addr);

	//  Accept new connections
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

	// Make the new connection non blocking
	fcntl(conn_sock, F_SETFL, O_NONBLOCK);

	// Prints info about the new client to output
	std::string host = inet_ntoa(new_addr.sin_addr);
	std::cout << "New connection from " << host << ":" << ntohs(new_addr.sin_port) << " on FD: " << conn_sock << std::endl;

	// Create new user class and store info inside
	User *user = new User("", "", host);
	user->_fd = conn_sock;

	struct epoll_event ev;
	//  Monitor new connection for read events in edge triggered mode
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = user;

	//  Allow epoll to monitor new connection
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev)) {
		std::cerr << "Failed to add file descriptor to epoll" << std::endl;
		close(epoll_fd);
		return;
	}
}

/*
	recv_message

	Saves message from client into buffer
	and parses it into words to create a response
*/
int recv_message(User* user, std::vector<Channel*>& channels) {
	char buffer[1024];
	std::string serverPrefix(":IRCQ+.org"), userPrefix(":" + user->_nick + "!" + user->_user + "@" + user->_host);
	memset(buffer, 0, sizeof(buffer));

	// Saves the message into buffer and saves how many bytes it recieved
	int bytesReceived = recv(user->_fd, buffer, sizeof(buffer), 0);
	if (bytesReceived < 0)
		return (std::cerr << "Error in recv(). SAD" << std::endl, 1);

	std::cout << "BuFFer: " << buffer;

	std::stringstream info(buffer);
	std::string word;
	// Parses the message into words and creates a response
	while (info >> word){
		std::cout << "Word: " << word << std::endl;
		if (word == "QUIT"){
			std::cout << "Client disconnected" << std::endl;
			return 0;
		}
		if (word == "die"){
			std::cout << "DIE COMMAND" << std::endl;
			close(serverSocket);
			exit(0);
		}
		if (word == "PING"){
			std::string pongMessage("PONG " + serverPrefix + "\n");
			send(user->_fd, pongMessage.c_str(), pongMessage.length(), 0);
		}
		if (word == "PART"){
			std::string Message(userPrefix + " PART :" + user->_nowAt->_name + "\n");
			for(std::vector<User*>::iterator it = user->_nowAt->_users.begin(); it != user->_nowAt->_users.end(); ++it)
				send((*it)->_fd, Message.c_str(), Message.length(), 0);
			user->_nowAt->_nUsers--;
			user->_nowAt->_users.erase(std::remove(user->_nowAt->_users.begin(), user->_nowAt->_users.end(), user), user->_nowAt->_users.end());
			user->_nowAt = NULL;
		}
		if (word == "JOIN"){
			std::string nameToJoin;
			info >> nameToJoin;

			std::vector<Channel*>::iterator found;
			for (found = channels.begin(); found != channels.end(); ++found)
				if (nameToJoin == (*found)->_name)
					break;

			if (found == channels.end()) {
				//ERR_NOSUCHCHANNEL
				std::string Message(serverPrefix + " 403 " + user->_nick + " " + nameToJoin + " :No such channel" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}
			else if ((*found)->_nUsers >= (*found)->_maxUsers) {
				//ERR_CHANNELISFULL
				std::string Message(serverPrefix + " 471 " + user->_nick + " " + (*found)->_name + " :Cannot join channel (+l)" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}
			else {
				(*found)->_nUsers++;
				(*found)->_users.push_back(user);
				user->_nowAt = (*found);
				std::string Message(userPrefix + " JOIN :" + (*found)->_name + "\n");
				for(std::vector<User*>::iterator it = user->_nowAt->_users.begin(); it != user->_nowAt->_users.end(); ++it)
					if ((*it)->_fd != user->_fd)
						send((*it)->_fd, Message.c_str(), Message.length(), 0);
				//RPL_TOPIC
				Message.append(serverPrefix + " 332 " + user->_nick + " " + (*found)->_name + " :" + (*found)->_topic + "\n");
				//RPL_NAMREPLY
				Message.append(serverPrefix + " 353 " + user->_nick + " = " + (*found)->_name + " :");
				for (std::vector<User*>::iterator it = (*found)->_users.begin(); it != (*found)->_users.end(); ++it)
					Message.append((*it)->_nick + " ");
				Message.append("\n");
				//RPL_ENDOFNAMES
				Message.append(serverPrefix + " 366 " + user->_nick + " " + (*found)->_name + " :End of Names list" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}
		}
		if (word == "PRIVMSG") {
			std::string channel, message;
			info >> channel;
			std::getline(info, message);
			std::string Message(userPrefix + " PRIVMSG " + channel + " :" + message + "\n");

			for(std::vector<User*>::iterator it = user->_nowAt->_users.begin(); it != user->_nowAt->_users.end(); ++it)
				if ((*it)->_fd != user->_fd)
					send((*it)->_fd, Message.c_str(), Message.length(), 0);
		}
		if (word == "NICK"){
			info >> word;
			user->_nick = word;
			//Need to do check if username is valid
			if (user->_welcome) {
				std::string Message(userPrefix + " NICK :" + user->_nick + "\n");
				if (user->_nowAt != NULL) {
					for(std::vector<User*>::iterator it = user->_nowAt->_users.begin(); it != user->_nowAt->_users.end(); ++it)
						send((*it)->_fd, Message.c_str(), Message.length(), 0);
				}
				else
					send(user->_fd, Message.c_str(), Message.length(), 0);
			}
		}
		if (word == "USER"){
			info >> word;
			user->_user = word;
		}
		if (!user->_welcome && user->_user.length() > 0 && user->_nick.length() > 0){
			user->_welcome = 1;
			std::string Message(serverPrefix + " 001 " + user->_nick + " :Welcome to IRCQ+ " + user->_nick + "!" + user->_user + "@" + user->_host + "\n");
			send(user->_fd, Message.c_str(), Message.length(), 0);
		}
	}
	//
	// if (!user->welcome){
	// 	user->welcome = 1;
	// 	std::stringstream info(buffer);
	// 	std::string word;
	// 	while (info >> word){
	// 		std::cout << "Word: " << word << std::endl;
	// 		if (word == "NICK"){
	// 			info >> word;
	// 			user->nick = word;
	// 		}
	// 		if (word == "USER"){
	// 			info >> word;
	// 			user->user = word;
	// 		}
	// 	}
	// 	std::string welcomeMessage(serverName + " 001 " + user->nick + " :Welcome to IRCQ+ " + user->nick + "! " + user->user + "@" + user->host + "\n");
	// 	send(user->fd, welcomeMessage.c_str(), welcomeMessage.length(), 0);
	// }
	// else {
	// 	std::stringstream info(buffer);
	// 	std::string word;
	// 	while (info >> word){
	// 		std::cout << "Word: " << word << std::endl;
	// 		if (word == "QUIT"){
	// 			std::cout << "Client disconnected" << std::endl;
	// 			return 0;
	// 		}
	// 		if (word == "die"){
	// 			std::cout << "DIE COMMAND" << std::endl;
	// 			exit(0);
	// 		}
	// 		if (word == "PING"){
	// 			std::string pongMessage("PONG " + serverName + "\n");
	// 			send(user->fd, pongMessage.c_str(), pongMessage.length(), 0);
	// 		}
	// 		if (word == "PART"){
	// 			send(user->fd, buffer, bytesReceived, 0);

	// 		}
	// 		if (word == "JOIN"){
	// 			send(user->fd, buffer, bytesReceived, 0);
	// 		}
	// 	}
	// }
	return 0;
}

int main() {

	std::vector<Channel*> channels;
	Channel *general = new Channel("#general", "Channel for general discussion", 5);
	channels.push_back(general);

	// creating server listen socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
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

	// initialize a new epoll instance
	struct epoll_event ev, events[5];
	int epoll_fd = epoll_create1(0);

	if (epoll_fd == -1) {
		std::cerr << "Failed to create epoll file descriptor" << std::endl;
		return 1;
	}

	// creates a new user class for storing the server socket number
	User *user = new User("", "", "");
	user->_fd = serverSocket;

	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = user;

	// add server socket to epoll for monitoring
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
			if (user->_fd == serverSocket)
				accept_new_connection_request(serverSocket, epoll_fd);
			else
				recv_message(user, channels);
		}
	}

	// closing the server socket
	close(serverSocket);

	return 0;
}
