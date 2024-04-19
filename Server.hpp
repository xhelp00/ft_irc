#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sstream>

#include "Channel.hpp"
#include "User.hpp"

#define BLUE "\033[0;34m"
#define END "\033[0m"

#include <map>

class Server
{
private:
	std::vector<User*> _users;
	std::vector<Channel*> _channels;
	std::string _pass, _serverPrefix;
	int _serverSocket, _epollSocket, _port;
public:
	//Cannonical form : default constructor, copy constructor, assignment operator, destructor
	Server();
	Server(int port, std::string pass);
	Server(const Server& server);
	Server& operator = (const Server& server);
	~Server();

	//Getters
	std::vector<User*>::iterator getUsersBegin();
	std::vector<User*>::iterator getUsersEnd();
	std::vector<Channel*>::iterator getChannelsBegin();
	std::vector<Channel*>::iterator getChannelsEnd();
	std::string getPass() const;
	std::string getServerPrefix() const;
	int getServerSocket() const;
	int getEpollSocket() const;
	int getPort() const;

	//Setters
	void setPass(std::string pass);
	void setServerPrefix(std::string serverPrefix);
	void setServerSocket(int serverSocket);
	void setEpollSocket(int epollSocket);
	void setPort(int port);

	//Methods

	void addUser(User* user);
	void removeUser(User* user);
	void addChannel(Channel* channel);
	void removeChannel(Channel* channel);

	void reply(User* user, std::string prefix, std::string command , std::string target, std::string text);

	int acceptConnection();
	int recvMessage(User* user);

};

#endif // SERVER_HPP



