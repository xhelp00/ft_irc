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
	User *user = new User("", "", "");
	user->setFd(_serverSocket);

	ev.events = EPOLLIN | EPOLLET;
	ev.data.ptr = user;

	// add server socket to epoll for monitoring
	if(epoll_ctl(_epollSocket, EPOLL_CTL_ADD, _serverSocket, &ev)) {
		throw std::runtime_error("Failed to add server socket to epoll");
		close(_epollSocket);
	}

	addChannel(new Channel("#general", "Channel for general discussion", "", 5));
	addChannel(new Channel("#random", "Channel for random discussion", "", 5));
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
	close(_serverSocket);
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
void Server::removeUser(User* user) { _users.erase(std::remove(_users.begin(), _users.end(), user), _users.end()); }
void Server::addChannel(Channel* channel) { _channels.push_back(channel); }
void Server::removeChannel(Channel* channel) { _channels.erase(std::remove(_channels.begin(), _channels.end(), channel), _channels.end()); }

void Server::reply(User* user, std::string prefix, std::string command , std::string target, std::string text) {
	std::string reply;
	if (prefix.length() > 0)
		reply.append(prefix + " ");
	else
		reply.append(_serverPrefix + " ");
	reply.append(command + " ");
	if (target.length() > 0)
		reply.append(target);
	else
		reply.append(user->getNick());
	if (text.length() > 0)
		reply.append(" " + text);
	if (reply.find("\n") == std::string::npos)
		reply.append("\n");
	int bytesSend = send(user->getFd(), reply.c_str(), reply.length(), 0);
	if (bytesSend < 0)
		std::cout << RED << "Error in sending message to FD: " << user->getFd() << "	" << reply << std::endl;
	std::cout << BLUE << "FD:" << user->getFd() << "	" << reply << END;
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
		//Needs more testing
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
			return 0;
		}
		//Need to clean memory after
		if (word == "die"){
			std::cout << "DIE COMMAND" << std::endl;
			close(_serverSocket);
			exit(0);
		}
		//Just a reply to client Ping command
		if (word == "PING")
			reply(user, "", "PONG", "", _serverPrefix);
		//Works only for leaving a current channel, needs more work
		if (word == "PART"){
			std::string line, namesToLeave, name, partMessage;
			std::getline(info, line);

			//ERR_NEEDMOREPARAMS
			if (line.length() <= 0)
				reply(user, "", "461", user->getNick(), "PART :Not enough parameters");

			std::stringstream arguments(line);

			arguments >> namesToLeave;
			std::getline(arguments, partMessage);
			partMessage.erase(0, partMessage.find_first_not_of(" 	"));

			std::stringstream names(namesToLeave);

			while (std::getline(names, name, ',')) {
				std::cout << "User {fd: " << user->getFd() << " nick: " << user->getNick() << "} is trying to leave channel " + name + " with message " + partMessage << std::endl;
				std::vector<Channel*>::iterator found;
				for (found = _channels.begin(); found != _channels.end(); ++found)
					if (name == (*found)->getName())
						break;

				//ERR_NOSUCHCHANNEL
				if (found == _channels.end())
					reply(user, "", "403", "", name + " :No such channel");
				else {
					std::vector<User*>::iterator us;
					for (us = (*found)->getUsersBegin(); us != (*found)->getUsersEnd(); ++us)
						if ((*us)->getFd() == user->getFd())
							break;
					//ERR_NOTONCHANNEL
					if (us == (*found)->getUsersEnd())
						reply(user, "", "442", "", name + " :You're not on that channel");
					else {
						for(std::vector<User*>::iterator it = (*found)->getUsersBegin(); it != (*found)->getUsersEnd(); ++it) {
							if (partMessage.length() > 0)
								reply((*it), user->getUserPrefix(), "PART", (*found)->getName(), partMessage);
							else
								reply((*it), user->getUserPrefix(), "PART", (*found)->getName(), "");
						}
						user->partChannel(*found);
						(*found)->removeUser(user);
					}
				}
			}
		}

		//Works need to add creating a new channel when joining an non existing channel
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

			//ERR_NEEDMOREPARAMS
			if (line.length() <= 0)
				reply(user, "", "461", "", "JOIN :Not enough parameters");
			else {

				std::stringstream arguments(line);

				arguments >> namesToJoin;
				arguments >> keysToJoin;

				std::stringstream names(namesToJoin), keys(keysToJoin);

				while (std::getline(names, name, ',')) {
					std::getline(keys, key, ',');
					std::cout << "User {fd: " << user->getFd() << " nick: " << user->getNick() << "} is trying to join channel " + name + " with key " + key << std::endl;
					std::vector<Channel*>::iterator cha;
					for (cha = _channels.begin(); cha != _channels.end(); ++cha)
						if (name == (*cha)->getName())
							break;

					//Leave all joined channels
					if (name == "#0" || name == "0") {
						for (std::vector<Channel*>::iterator ser = user->getJoinedChannelsBegin(); ser != user->getJoinedChannelsEnd(); ++ser) {
							for(std::vector<User*>::iterator us = (*ser)->getUsersBegin(); us != (*ser)->getUsersEnd(); ++us)
								reply((*us), user->getUserPrefix(), "PART", (*ser)->getName(), "");
							user->partChannel(*ser);
							(*ser)->removeUser(user);
						}
						reply(user, "", "403", "", name + " :No such channel");
					}
					//ERR_NOSUCHCHANNEL
					else if (cha == _channels.end())
						reply(user, "", "403", "", name + " :No such channel");
					//ERR_CHANNELISFULL
					else if ((*cha)->getNUsers() >= (*cha)->getMaxUsers())
						reply(user, "", "471", "", (*cha)->getName() + " :Cannot join channel (+l)");
					//ERR_BADCHANNELKEY
					else if ((*cha)->getKey() != key) {
						reply(user, "", "475", "", (*cha)->getName() + " :Cannot join channel (+k) - bad key");
					}
					else {
						(*cha)->addUser(user);
						user->joinChannel(*cha);

						//JOIN MESSAGE to all users of that channel
						for(std::vector<User*>::iterator us = (*cha)->getUsersBegin(); us != (*cha)->getUsersEnd(); ++us)
							reply((*us), user->getUserPrefix(), "JOIN", ":" + (*cha)->getName(), "");

						//RPL_TOPIC
						reply(user, "", "332", "", (*cha)->getName() + " :" + (*cha)->getTopic());

						//RPL_NAMREPLY
						std::string Message("= " + (*cha)->getName() + " :");
						for (std::vector<User*>::iterator it = (*cha)->getUsersBegin(); it != (*cha)->getUsersEnd(); ++it)
							if (it != (*cha)->getUsersEnd() - 1)
								Message.append((*it)->getNick() + " ");
							else
								Message.append((*it)->getNick());
						reply(user, "", "353", "", Message);

						//RPL_ENDOFNAMES
						reply(user, "", "366", "", (*cha)->getName() + " :End of Names list");
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
			std::cout << msgtarget << " << " << textToSend << "\n";

			//ERR_NORECIPIENT
			if (msgtarget.length() <= 0)
				reply(user, "", "411", "", ":No recipient given (PRIVMSG)");
			//ERR_NOTEXTTOSEND
			else if (textToSend.length() <= 0)
				reply(user, "", "412", "", ":No text to send");
			else if ((*msgtarget.begin()) == '#') {
				std::vector<Channel*>::iterator foundServer;
				for (foundServer = _channels.begin(); foundServer != _channels.end(); ++foundServer)
					if (msgtarget == (*foundServer)->getName())
						break;
				//ERR_NOSUCHNICK
				if (foundServer == _channels.end())
					reply(user, "", "401", "", msgtarget + ":No such nick/channel");
				else
					for(std::vector<User*>::iterator it = (*foundServer)->getUsersBegin(); it != (*foundServer)->getUsersEnd(); ++it)
						if ((*it)->getNick() != user->getNick())
							reply((*it), user->getUserPrefix(), "PRIVMSG", msgtarget, ":" + textToSend);
			}
			else {
				std::vector<User*>::iterator foundUser;
				for (foundUser = _users.begin(); foundUser != _users.end(); ++foundUser)
					if (msgtarget == (*foundUser)->getNick())
						break;
				//ERR_NOSUCHNICK
				if (foundUser == _users.end())
					reply(user, "", "401", "", msgtarget + ":No such nick/channel");
				else
					reply((*foundUser), user->getUserPrefix(), "PRIVMSG", msgtarget, ":" + textToSend);
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

			//ERR_NONICKNAMEGIVEN
			else if (nick.length() <= 0) {
				reply(user, "", "431", "", ":No nickname given");
				correctNick = 0;
			}

			//ERR_ERRONEUSNICKNAME
			else if (nick.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]{}\\|^`–-_") != nick.npos || isdigit(nick[0])) {
				reply(user, "", "432", "", ":Erroneous nickname");
				correctNick = 0;
			}

			//Cheking if nick is in use
			std::vector<User*>::iterator foundUser;
			for (foundUser = _users.begin(); foundUser != _users.end(); ++foundUser)
				if (nick == (*foundUser)->getNick())
					break;
			//ERR_NICKNAMEINUSE
			if (foundUser != _users.end()) {
				reply(user, "", "433 *", nick, ":Nickname is already in use.");
				correctNick = 0;
			}

			//Setting the nickname to user if it's correct
			if (correctNick) {
				if (user->getWelcome()) {
					if (user->getNJoinedChannels() > 0) {
						for(std::vector<Channel*>::iterator ser = user->getJoinedChannelsBegin(); ser != user->getJoinedChannelsEnd(); ++ser)
							for(std::vector<User*>::iterator us = (*ser)->getUsersBegin(); us != (*ser)->getUsersEnd(); ++us)
								if ((*us)->getFd() != user->getFd())
									reply(user, user->getUserPrefix(), "NICK", ":" + nick, "");
					}
					else
						reply(user, user->getUserPrefix(), "NICK", ":" + nick, "");
				}
				user->setNick(nick);
			}
		}

		if (word == "USER"){
			info >> word;
			user->setUser(word);
		}

		//Works but need to add check for permission
		//and not sure about setting the topic when not on that channel
		if (word == "TOPIC"){
			std::string channelTo, topic;
			info >> channelTo;
			std::getline(info, topic);

			topic.erase(0, topic.find_first_not_of(" 	:"));
			// std::cout << channelTo << " : " << topic << "\n";
			std::vector<Channel*>::iterator found;
			for (found = _channels.begin(); found != _channels.end(); ++found)
				if (channelTo == (*found)->getName())
					break;

			if (found != _channels.end()) {
				//Remove topic
				if (topic == "") {
					(*found)->setTopic("");
					for(std::vector<User*>::iterator it = (*found)->getUsersBegin(); it != (*found)->getUsersEnd(); ++it)
						reply(*it, user->getUserPrefix(), "TOPIC", (*found)->getName(), ":");
				}
				//Change topic
				else if (topic.length() > 0) {
					(*found)->setTopic(topic);
					for(std::vector<User*>::iterator it = (*found)->getUsersBegin(); it != (*found)->getUsersEnd(); ++it)
						reply(*it, user->getUserPrefix(), "TOPIC", (*found)->getName(), ":" + (*found)->getTopic());
				}
				//RPL_NOTOPIC
				else if ((*found)->getTopic().length() > 0)
					reply(user, "", "331", "", (*found)->getName() + " :No topic is set.");
				//RPL_TOPIC
				else
					reply(user, "", "332", "", (*found)->getName() + " :" + (*found)->getTopic());
			}
		}

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
