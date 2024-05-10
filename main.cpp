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
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#ifdef __linux__
#include <sys/epoll.h>
// Linux-specific includes and definitions
#elif __APPLE__
#include <sys/event.h>
#include <sys/time.h>
// macOS-specific includes and definitions
#endif



int serverSocket;

class Channel;
// class for user info storrage
class User
{
public:
	std::string _nick, _user, _host;
	std::vector<Channel*> _joinedChannels;
	int _welcome, _fd, _nJoinedChannels;
	User(std::string nick, std::string user, std::string host) : _nick(nick), _user(user), _host(host), _welcome(0), _fd(0), _nJoinedChannels(0){}
	~User(){}
};

class Channel {
public:
	std::string _name, _topic, _key;
	std::vector<User*> _users;
	uint32_t _nUsers, _maxUsers;
	Channel(std::string name, std::string topic, std::string key, int maxUsers) : _name(name), _topic(topic), _key(key), _nUsers(0), _maxUsers(maxUsers){}
	~Channel(){}
};

/*
	accept_new_connection_request

	Accepts a new connection from a client
	creates a new user class and
	adds the client socket to epoll
*/
void accept_new_connection_request(std::vector<User*>& users, int server_fd, int epoll_fd) {
	sockaddr_in new_addr;
	socklen_t addrlen = sizeof(new_addr);

	//  Accept new connections
	int conn_sock = accept(server_fd, (struct sockaddr*)&new_addr,
						(socklen_t*)&addrlen);

	if (conn_sock == -1) {
			std::cerr << "accept" << std::endl;
			return;
	}

	// Make the new connection non blocking
	fcntl(conn_sock, F_SETFL, O_NONBLOCK);

	// Prints info about the new client to output
	std::string host = inet_ntoa(new_addr.sin_addr);
	std::cout << "New connection from " << host << ":" << ntohs(new_addr.sin_port) << " on FD: " << conn_sock << std::endl;

	// Create new user class and store info inside
	User *user = new User("", "", host);
	users.push_back(user);
	user->_fd = conn_sock;
#ifdef __linux__
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
#elif __APPLE__

	struct kevent change;
    EV_SET(&change, conn_sock, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, user);

    if (kevent(epoll_fd, &change, 1, NULL, 0, NULL) == -1) {
        std::cerr << "Failed to add file descriptor to kqueue" << std::endl;
        close(epoll_fd);
        return;
    }
	
#endif
}

/*
	recv_message

	Saves message from client into buffer
	and parses it into words to create a response
*/
int recv_message(std::vector<User*>& users, User* user, std::vector<Channel*>& channels) {
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
		//Works fine
		if (word == "QUIT"){
			std::cout << "Client disconnected" << std::endl;
			return 0;
		}
		//Need to clean memory after
		if (word == "die"){
			std::cout << "DIE COMMAND" << std::endl;
			close(serverSocket);
			exit(0);
		}
		//Just a reply to client Ping command
		if (word == "PING"){
			std::string pongMessage("PONG " + serverPrefix + "\n");
			send(user->_fd, pongMessage.c_str(), pongMessage.length(), 0);
		}
		//Works only for leaving a current channel, needs more work
		if (word == "PART"){
			std::string line, namesToLeave, name, partMessage;
			std::getline(info, line);

			if (line.length() <= 0) {
				//ERR_NEEDMOREPARAMS
				std::string Message(serverPrefix + " 461 " + user->_nick + " PART :Not enough parameters" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}

			std::stringstream arguments(line);

			arguments >> namesToLeave;
			std::getline(arguments, partMessage);
			partMessage.erase(0, partMessage.find_first_not_of(" 	"));

			std::stringstream names(namesToLeave);

			while (std::getline(names, name, ',')) {
				std::cout << "User {fd: " << user->_fd << " nick: " << user->_nick << "} is trying to leave channel " + name + " with message " + partMessage << std::endl;
				std::vector<Channel*>::iterator found;
				for (found = channels.begin(); found != channels.end(); ++found)
					if (name == (*found)->_name)
						break;

				if (found == channels.end()) {
					//ERR_NOSUCHCHANNEL
					std::string Message(serverPrefix + " 403 " + user->_nick + " " + name + " :No such channel" + "\n");
					send(user->_fd, Message.c_str(), Message.length(), 0);
				}
				else {
					std::vector<User*>::iterator us;
					for (us = (*found)->_users.begin(); us != (*found)->_users.end(); ++us)
						if ((*us)->_fd == user->_fd)
							break;
					if (us == (*found)->_users.end()) {
						//ERR_NOTONCHANNEL
						std::string Message(serverPrefix + " 442 " + user->_nick + " " + name + " :You're not on that channel" + "\n");
						send(user->_fd, Message.c_str(), Message.length(), 0);
					}
					else {
						std::string Message(userPrefix + " PART " + (*found)->_name);
						if (partMessage.length() > 0)
							Message.append(" " + partMessage + "\n");
						else
							Message.append("\n");
						std::cout << "User {fd: " << user->_fd << " nick: " << user->_nick << "} is leaving with command " + Message << std::endl;
						for(std::vector<User*>::iterator it = (*found)->_users.begin(); it != (*found)->_users.end(); ++it)
							send((*it)->_fd, Message.c_str(), Message.length(), 0);
						(*found)->_nUsers--;
						user->_joinedChannels.erase(std::remove(user->_joinedChannels.begin(), user->_joinedChannels.end(), (*found)), user->_joinedChannels.end());
						(*found)->_users.erase(std::remove((*found)->_users.begin(), (*found)->_users.end(), user), (*found)->_users.end());
					}
				}
			}
		}

		//Works need to add option 0
		/*
			JOIN #toast,#ircv3 mysecret

			line = JOIN #toast,#ircv3 mysecret

			namesToJoin = #toast,#ircv3
			keysToJoin = mysecret
		*/
		if (word == "JOIN"){
			std::string line, namesToJoin, name, keysToJoin, key;
			std::getline(info, line);

			line.erase(0, line.find_first_not_of(" 	"));

			if (line.length() <= 0) {
				//ERR_NEEDMOREPARAMS
				std::string Message(serverPrefix + " 461 " + user->_nick + " JOIN :Not enough parameters" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}
			else if (line == "#0") {
				//Leave all joined channels
				std::cout << line << std::endl;
				for (std::vector<Channel*>::iterator ser = user->_joinedChannels.begin(); ser != user->_joinedChannels.end(); ++ser) {
					std::string Message(userPrefix + " PART  " + (*ser)->_name + "\n");
					for(std::vector<User*>::iterator us = (*ser)->_users.begin(); us != (*ser)->_users.end(); ++us)
						send((*us)->_fd, Message.c_str(), Message.length(), 0);
				}
			}
			else {

				std::stringstream arguments(line);

				arguments >> namesToJoin;
				arguments >> keysToJoin;

				std::stringstream names(namesToJoin), keys(keysToJoin);

				while (std::getline(names, name, ',')) {
					std::getline(keys, key, ',');
					std::cout << "User {fd: " << user->_fd << " nick: " << user->_nick << "} is trying to join channel " + name + " with key " + key << std::endl;
					std::vector<Channel*>::iterator found;
					for (found = channels.begin(); found != channels.end(); ++found)
						if (name == (*found)->_name)
							break;

					if (found == channels.end()) {
						//ERR_NOSUCHCHANNEL
						std::string Message(serverPrefix + " 403 " + user->_nick + " " + name + " :No such channel" + "\n");
						send(user->_fd, Message.c_str(), Message.length(), 0);
					}
					else if ((*found)->_nUsers >= (*found)->_maxUsers) {
						//ERR_CHANNELISFULL
						std::string Message(serverPrefix + " 471 " + user->_nick + " " + (*found)->_name + " :Cannot join channel (+l)" + "\n");
						send(user->_fd, Message.c_str(), Message.length(), 0);
					}
					else if ((*found)->_key != key) {
						//ERR_BADCHANNELKEY
						std::string Message(serverPrefix + " 475 " + user->_nick + " " + (*found)->_name + " :Cannot join channel (+k) - bad key" + "\n");
						send(user->_fd, Message.c_str(), Message.length(), 0);
					}
					else {
						(*found)->_nUsers++;
						(*found)->_users.push_back(user);
						user->_joinedChannels.push_back(*found);

						{
						//JOIN MESSAGE to all users of that channel
						std::string Message(userPrefix + " JOIN :" + (*found)->_name + "\n");
						for(std::vector<User*>::iterator us = (*found)->_users.begin(); us != (*found)->_users.end(); ++us)
							send((*us)->_fd, Message.c_str(), Message.length(), 0);
						}

						//RPL_TOPIC
						std::string Message(serverPrefix + " 332 " + user->_nick + " " + (*found)->_name + " :" + (*found)->_topic + "\n");
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
			}
		}
		//Works fine i guess
		if (word == "PRIVMSG") {
			std::string msgtarget, textToSend;
			info >> msgtarget;

			std::getline(info, textToSend);

			textToSend.erase(0, textToSend.find_first_not_of(" 	:"));
			std::cout << msgtarget << " : " << textToSend << "\n";

			if (msgtarget.length() <= 0) {
				//ERR_NORECIPIENT
				std::string Message(serverPrefix + " 411 " + user->_nick + " :No recipient given (PRIVMSG)" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}
			else if (textToSend.length() <= 0) {
				//ERR_NOTEXTTOSEND
				std::string Message(serverPrefix + " 412 " + user->_nick + " :No text to send" + "\n");
				send(user->_fd, Message.c_str(), Message.length(), 0);
			}

			if ((*msgtarget.begin()) == '#') {
				std::vector<Channel*>::iterator foundServer;
				for (foundServer = channels.begin(); foundServer != channels.end(); ++foundServer)
					if (msgtarget == (*foundServer)->_name)
						break;
				if (foundServer == channels.end()) {
					//ERR_CANNOTSENDTOCHAN
					std::string Message(serverPrefix + " 401 " + user->_nick + " " + msgtarget + " :No such nick/channel" + "\n");
					send(user->_fd, Message.c_str(), Message.length(), 0);
				}
				else {
					std::string Message(userPrefix + " PRIVMSG " + msgtarget + " :" + textToSend + "\n");
					for(std::vector<User*>::iterator it = (*foundServer)->_users.begin(); it != (*foundServer)->_users.end(); ++it)
						if ((*it)->_nick != user->_nick)
							send((*it)->_fd, Message.c_str(), Message.length(), 0);
				}
			}
			else {
				std::vector<User*>::iterator foundUser;
				for (foundUser = users.begin(); foundUser != users.end(); ++foundUser)
					if (msgtarget == (*foundUser)->_nick)
						break;
				if (foundUser == users.end()) {
					//ERR_NOSUCHNICK
					std::string Message(serverPrefix + " 401 " + user->_nick + " " + msgtarget + " :No such nick/channel" + "\n");
					send(user->_fd, Message.c_str(), Message.length(), 0);
				}
				else {
					std::string Message(userPrefix + " PRIVMSG " + msgtarget + " :" + textToSend + "\n");
					send((*foundUser)->_fd, Message.c_str(), Message.length(), 0);
				}
			}
		}
		//Works fine
		if (word == "NICK"){
			int correctNick = 1;
			std::string	nick;
			info >> nick;
			//Truncating the nick
			if (nick.length() > 30)
				nick = nick.substr(0, 30);
			//No nick given
			else if (nick.length() <= 0) {
				std::string errorMessage(serverPrefix + " 431 " + user->_nick + " :No nickname given" + "\n");
				send(user->_fd, errorMessage.c_str(), errorMessage.length(), 0);
				correctNick = 0;
			}

			//Invalid nickname
			else if (nick.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}\\|^`–-_") != nick.npos || isdigit(nick[0])) {
				std::string errorMessage(serverPrefix + " 432 " + user->_nick + " " + nick + " :Erroneous nickname" + "\n");
				send(user->_fd, errorMessage.c_str(), errorMessage.length(), 0);
				correctNick = 0;
			}

			//Cheking if nick is in use
			std::vector<User*>::iterator foundUser;
			for (foundUser = users.begin(); foundUser != users.end(); ++foundUser)
				if (nick == (*foundUser)->_nick)
					break;
			if (foundUser != users.end()) {
				std::string errorMessage(serverPrefix + " 433 " + "* " + nick + " :Nickname is already in use." + "\n");
				send(user->_fd, errorMessage.c_str(), errorMessage.length(), 0);
				correctNick = 0;
			}

			//Setting the nickname to user if it's correct
			if (correctNick) {
				user->_nick = nick;
				if (user->_welcome) {
					std::string Message(userPrefix + " NICK :" + user->_nick + "\n");
					if (user->_nJoinedChannels > 0) {
						for(std::vector<Channel*>::iterator ser = user->_joinedChannels.begin(); ser != user->_joinedChannels.end(); ++ser)
							for(std::vector<User*>::iterator us = (*ser)->_users.begin(); us != (*ser)->_users.end(); ++us)
								if ((*us)->_fd != user->_fd)
									send((*us)->_fd, Message.c_str(), Message.length(), 0);
					}
					else
						send(user->_fd, Message.c_str(), Message.length(), 0);
				}
			}
		}
		if (word == "USER"){
			info >> word;
			user->_user = word;
		}
		//Works but need to add check for permission
		//and not sure about setting the topic when not on that channel
		if (word == "TOPIC"){
			std::string channelTo, topic;
			info >> channelTo;
			std::getline(info, topic);

			topic.erase(0, topic.find_first_not_of(" 	:"));
			std::cout << channelTo << " : " << topic << "\n";
			std::vector<Channel*>::iterator found;
			for (found = channels.begin(); found != channels.end(); ++found)
				if (channelTo == (*found)->_name)
					break;

			if (found != channels.end()) {
				if (topic == "") {
					//Remove topic
					(*found)->_topic = "";
					std::string Message(userPrefix + " TOPIC " + (*found)->_name + " :" + "\n");
					for(std::vector<User*>::iterator it = (*found)->_users.begin(); it != (*found)->_users.end(); ++it)
						send((*it)->_fd, Message.c_str(), Message.length(), 0);
				}
				else if (topic.length() > 0) {
					//Change topic
					(*found)->_topic = topic;
					std::string Message(userPrefix + " TOPIC " + (*found)->_name + " :" + topic + "\n");
					for(std::vector<User*>::iterator it = (*found)->_users.begin(); it != (*found)->_users.end(); ++it)
						send((*it)->_fd, Message.c_str(), Message.length(), 0);
				}
				else if ((*found)->_topic.length() > 0){
					std::string Message(serverPrefix + " 331 " + user->_nick + " " + (*found)->_name + " :No topic is set." + "\n");
					send(user->_fd, Message.c_str(), Message.length(), 0);
				}
				else {
					std::string Message(serverPrefix + " 332 " + user->_nick + " " + (*found)->_name + " :" + (*found)->_topic + "\n");
					send(user->_fd, Message.c_str(), Message.length(), 0);
				}
			}
		}
		//Displaying a welcome message to a new connected client
		if (!user->_welcome && user->_user.length() > 0 && user->_nick.length() > 0){
			user->_welcome = 1;
			std::string Message(serverPrefix + " 001 " + user->_nick + " :Welcome to IRCQ+ " + user->_nick + "!" + user->_user + "@" + user->_host + "\n");
			send(user->_fd, Message.c_str(), Message.length(), 0);
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	(void) argc;
	std::vector<Channel*> channels;
	Channel *general = new Channel("#general", "Channel for general discussion", "", 5);
	channels.push_back(general);

	std::vector<User*> users;

	// creating server listen socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0) {
		std::cerr << "Setting up socket failed. SAD" << std::endl;
		return 1;
	}

	// specifying the address
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(atoi(argv[1]));
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	// binding socket.
	bind(serverSocket, (sockaddr*)&serverAddress,
		 sizeof(serverAddress));

	// listening to the assigned socket
	listen(serverSocket, SOMAXCONN);
#ifdef __linux__
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
				accept_new_connection_request(users, serverSocket, epoll_fd);
			else
				recv_message(users, user, channels);
		}
	}
#elif __APPLE__
	// Assuming serverSocket is already set up and is non-blocking
    int kq = kqueue();
    if (kq == -1) {
        std::cerr << "Failed to create kqueue" << std::endl;
        return 1;
    }

    User *serverUser = new User("", "", "");
    serverUser->_fd = serverSocket;

    struct kevent change;
    EV_SET(&change, serverSocket, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, serverUser);

    if (kevent(kq, &change, 1, NULL, 0, NULL) == -1) {
        std::cerr << "Failed to add server socket to kqueue" << std::endl;
        close(kq);
        return 1;
    }

    struct kevent events[5]; // Buffer for events
    int nev;

    while (true) {
        nev = kevent(kq, NULL, 0, events, 5, NULL);
        if (nev == -1) {
            std::cerr << "Failed to wait for events" << std::endl;
            close(kq);
            return 1;
        }

        for (int i = 0; i < nev; i++) {
            User *user = reinterpret_cast<User*>(events[i].udata);
            if (user->_fd == serverSocket) {
                accept_new_connection_request(users, serverSocket, kq);
            } else {
                recv_message(users, user, channels);
            }
        }
    }
#endif
	// closing the server socket
	close(serverSocket);

	return 0;
}
