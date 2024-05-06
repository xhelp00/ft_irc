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

union Modes
{
	bool i; //Invite-only
	bool t; //TOPIC only by operators
};


class Channel {
private:
	std::string _name, _topic, _key;
	std::vector<User*> _users, _operators;
	uint32_t _nUsers, _maxUsers;
	Modes _modes;
public:
	//Cannonical form : default constructor, copy constructor, assignment operator, destructor
	Channel(std::string name, std::string topic, std::string key, User* chop, int maxUsers);
	Channel(const Channel& channel);
	Channel& operator = (const Channel& channel);
	~Channel();

	//Getters
	std::string getName() const;
	std::string getTopic() const;
	std::string getKey() const;
	uint32_t getNUsers() const;
	uint32_t getMaxUsers() const;
	std::vector<User*>::iterator getUsersBegin();
	std::vector<User*>::iterator getUsersEnd();
	std::vector<User*>::const_iterator getUsersBegin() const;
	std::vector<User*>::const_iterator getUsersEnd() const;
	std::vector<User*>::iterator getOperatorsBegin();
	std::vector<User*>::iterator getOperatorsEnd();
	bool isInviteOnly() const;
	bool isTopicOnlyByOperators() const;

	//Setters
	void setName(std::string name);
	void setTopic(std::string topic);
	void setKey(std::string key);
	void setNUsers(uint32_t nUsers);
	void setMaxUsers(uint32_t maxUsers);
	void setInviteOnly(bool inviteOnly);
	void setTopicOnlyByOperators(bool topicOnlyByOperators);

	//Methods
	void addUser(User* user);
	void removeUser(User* user);
	void addOperator(User* user);
	void removeOperator(User* user);

	bool isOperator(User* user) const;
};

std::ostream&	operator << (std::ostream& out, const Channel& channel);

#endif // CHANNEL_HPP
