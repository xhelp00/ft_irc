#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <sstream>

#include "User.hpp"

class User;

class Channel {
private:
	std::string _name, _topic, _key;
	std::vector<User*> _users;
	uint32_t _nUsers, _maxUsers;
public:
	//Cannonical form : default constructor, copy constructor, assignment operator, destructor
	Channel(std::string name, std::string topic, std::string key, int maxUsers);
	Channel(const Channel& channel);
	Channel& operator = (const Channel& channel);
	~Channel();

	//Getters
	std::string getName();
	std::string getTopic();
	std::string getKey();
	uint32_t getNUsers();
	uint32_t getMaxUsers();
	std::vector<User*>::iterator getUsersBegin();
	std::vector<User*>::iterator getUsersEnd();

	//Setters
	void setName(std::string name);
	void setTopic(std::string topic);
	void setKey(std::string key);
	void setNUsers(uint32_t nUsers);
	void setMaxUsers(uint32_t maxUsers);

	//Methods
	void addUser(User* user);
	void removeUser(User* user);

	bool operator == (const Channel& channel) const;
	bool operator != (const Channel& channel) const;
};

#endif // CHANNEL_HPP
