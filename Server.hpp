#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <typeinfo>

#include "Channel.hpp"
#include "User.hpp"

#define CHANNEL_TYPE typeid(std::vector<Channel*>).name()
#define USER_TYPE typeid(std::vector<User*>).name()

class Server
{
private:
	std::vector<User*> _users;
	std::vector<Channel*> _channels;
	User* _serverUser;
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

	//Commands
	void TOPIC(std::stringstream& info, User* user);
	void NICK(std::stringstream& info, User* user);
	void PRIVMSG(std::stringstream& info, User* user);
	void JOIN(std::stringstream& info, User* user, bool invited);
	void PART(std::stringstream& info, User* user);
	void KICK(std::stringstream& info, User* user);
	void MODE(std::stringstream& info, User* user);
	void INVITE(std::stringstream& info, User* user);
	void WHO(std::stringstream& info, User* user);
};

template <typename T>
void print(T& li){
	std::string type(typeid(T).name());
	if (type == CHANNEL_TYPE)
		std::cout << GREEN << "Printing content of Channels" << std::endl;
	else if (type == USER_TYPE)
		std::cout << YELLOW << "Printing content of Users" << std::endl;
	for (typename T::const_iterator it = li.begin(); it != li.end(); it++)
		std::cout << (*(*it));
	std::cout << END;
}

#endif // SERVER_HPP



