#include "Server.hpp"

Server::Server() : _pass(""), _serverPrefix(":IRCQ+"), _serverSocket(0), _epollSocket(0), _port(0) {

}

Server::Server(int port, std::string pass) : _pass(pass), _serverPrefix(":IRCQ+"), _serverSocket(0), _epollSocket(0), _port(port) {

	// creating server listen socket
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
		throw std::runtime_error("Failed to create server socket");

	// specifying the address
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	// binding socket.
	bind(_serverSocket, (sockaddr*)&serverAddress,
		 sizeof(serverAddress));

	// listening to the assigned socket
	listen(_serverSocket, SOMAXCONN);

	// initialize a new epoll instance
	struct epoll_event ev;
	_epollSocket = epoll_create1(0);

	if (_epollSocket == -1)
		throw std::runtime_error("Failed to create epoll instance");

	// creates a new user class for storing the server socket number
	_serverUser = new User("", "", "");
	_serverUser->setFd(_serverSocket);
	_serverUser->setNick("ServerUser");

	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = _serverUser;

	// add server socket to epoll for monitoring
	if(epoll_ctl(_epollSocket, EPOLL_CTL_ADD, _serverSocket, &ev)) {
		throw std::runtime_error("Failed to add server socket to epoll");
		close(_epollSocket);
	}
}

Server::Server(const Server& server) : _users(server._users), _channels(server._channels), _pass(server._pass), _serverPrefix(server._serverPrefix), _serverSocket(server._serverSocket), _epollSocket(server._epollSocket), _port(server._port) {

}

Server& Server::operator = (const Server& server) {
	_users = server._users;
	_channels = server._channels;
	_pass = server._pass;
	_serverPrefix = server._serverPrefix;
	_serverSocket = server._serverSocket;
	_epollSocket = server._epollSocket;
	_port = server._port;
	return *this;
}

Server::~Server() {
	std::cout << RED << "Calling Server destructor" << END << std::endl;
	close(_serverSocket);
	delete _serverUser;
	while (!_channels.empty()) delete _channels.back(), _channels.pop_back();
	while (!_users.empty()) delete _users.back(), _users.pop_back();
}

std::vector<User*>::iterator Server::getUsersBegin() { return _users.begin(); }
std::vector<User*>::iterator Server::getUsersEnd() { return _users.end(); }
std::vector<Channel*>::iterator Server::getChannelsBegin() { return _channels.begin(); }
std::vector<Channel*>::iterator Server::getChannelsEnd() { return _channels.end(); }
std::string Server::getPass() const { return _pass; }
std::string Server::getServerPrefix() const { return _serverPrefix; }
int Server::getServerSocket() const { return _serverSocket; }
int Server::getEpollSocket() const { return _epollSocket; }
int Server::getPort() const { return _port; }

void Server::setPass(std::string pass) { _pass = pass; }
void Server::setServerPrefix(std::string serverPrefix) { _serverPrefix = serverPrefix; }
void Server::setServerSocket(int serverSocket) { _serverSocket = serverSocket; }
void Server::setEpollSocket(int epollSocket) { _epollSocket = epollSocket; }
void Server::setPort(int port) { _port = port; }

void Server::addUser(User* user) { _users.push_back(user); }
void Server::removeUser(User* user) {
	delete user;
	_users.erase(std::find(_users.begin(), _users.end(), user), _users.end());
}
void Server::addChannel(Channel* channel) { _channels.push_back(channel); }
void Server::removeChannel(Channel* channel) {
	delete channel;
	_channels.erase(std::find(_channels.begin(), _channels.end(), channel), _channels.end());
}

/*	Server::recvMessage(User* user)

	Saves message from client into buffer
	and parses it into words to create a response
*/
int Server::recvMessage(User* user) {
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));

	// Saves the message into buffer and saves how many bytes it recieved
	int bytesReceived = recv(user->getFd(), buffer, sizeof(buffer), 0);
	if (bytesReceived < 0)
		return (std::cerr << "Error in recv(). SAD" << std::endl, 1);

	std::cout << "BuFFer: " << buffer;

	std::stringstream info(buffer);
	std::string word;
	// Parses the message into words and creates a response
	while (info >> word){
		std::cout << "Word: " << word << std::endl;
		//Works fine
		if (word == "PASS"){
			std::string pass;
			info >> pass;
			//ERR_PASSWDMISMATCH
			if (pass != _pass)
				reply(user, "", "464", user->getNick(), ":Password incorrect");
			else if (user->getAuthed() == 0)
				user->setAuthed();
		}

		//Works fine
		if (word == "QUIT"){
			std::cout << "Client disconnected" << std::endl;
			removeUser(user);
			return 0;
		}

		//Need to clean memory after
		if (word == "die"){
			std::cout << "DIE COMMAND" << std::endl;
			return -1;
		}

		//Works fine, just a reply to client Ping command
		if (word == "PING")
			reply(user, "", "PONG", "", _serverPrefix);

		/*
			MODE #general +k-l pass
		*/
		if (word == "MODE")
			Server::MODE(info, user);

		if (word == "INVITE")
			Server::INVITE(info, user);

		//Works fine
		if (word == "KICK")
			Server::KICK(info, user);

		//Works fine
		if (word == "PART")
			Server::PART(info, user);

		//Works need to add channel modes
		/*
			JOIN #toast,#ircv3 mysecret

			line = JOIN #toast,#ircv3 mysecret

			namesToJoin = #toast,#ircv3
			keysToJoin = mysecret
		*/
		if (word == "JOIN")
			Server::JOIN(info, user);

		//Works fine
		if (word == "PRIVMSG")
			Server::PRIVMSG(info, user);

		//Works fine
		if (word == "LIST"){
			for (std::vector<Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
				reply(user, "", "322", "", (*it)->getName() + " :" + (*it)->getTopic());
			//RPL_ENDOFNAMES
			reply(user, "", "323", "", ":End of LIST");
		}

		//Works fine
		if (word == "NICK")
			Server::NICK(info, user);

		//Works fine
		if (word == "USER"){
			info >> word;
			user->setUser(word);
		}

		//Works fine
		if (word == "TOPIC")
			Server::TOPIC(info, user);

		//RPL_WELCOME
		if (!user->getWelcome() && user->getUser().length() > 0 && user->getNick().length() > 0 && user->getAuthed()){
			user->setWelcome();
			reply(user, "", "001", "", ":Welcome to IRCQ+ " + user->getUserPrefix().substr(1, user->getUserPrefix().length() - 1));
		}
	}
	print(_users);
	print(_channels);
	return 0;
}

/*	Server::acceptConnection()

	Accepts a new connection from a client
	creates a new user class and
	adds the client socket to epoll
*/
int Server::acceptConnection() {
	sockaddr_in new_addr;
	socklen_t addrlen = sizeof(new_addr);

	// Accept new connections
	int conn_sock = accept(_serverSocket, (struct sockaddr*)&new_addr,
						(socklen_t*)&addrlen);

	if (conn_sock == -1)
			return (std::cerr << "accept" << std::endl, 1);

	// Make the new connection non blocking
	fcntl(conn_sock, F_SETFL, O_NONBLOCK);

	// Prints info about the new client to output
	std::string host = inet_ntoa(new_addr.sin_addr);
	std::cout << "New connection from " << host << ":" << ntohs(new_addr.sin_port) << " on FD: " << conn_sock << std::endl;

	// Create new user class and store info inside
	User *user = new User("", "", host);
	_users.push_back(user);
	user->setFd(conn_sock);

	struct epoll_event ev;
	// Monitor new connection for read events in edge triggered mode
	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = user;

	// Allow epoll to monitor new connection
	if (epoll_ctl(_epollSocket, EPOLL_CTL_ADD, conn_sock, &ev))
		return (std::cerr << "Failed to add client socket to epoll" << std::endl, close(_epollSocket), 1);

	return 0;
}
