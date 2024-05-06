#ifndef USER_HPP
#define USER_HPP

#include <iostream>
#include <vector>

#include "Channel.hpp"

class Channel;

class User
{
private:
	std::string _nick, _user, _host;
	std::vector<Channel*> _joinedChannels;
	bool _welcome, _authed;
	int _fd, _nJoinedChannels;
public:
	//Cannonical form : default constructor, copy constructor, assignment operator, destructor
	User(std::string nick, std::string user, std::string host);
	User(const User& user);
	User& operator = (const User& user);
	~User();

	//Getters
	std::string getNick() const;
	std::string getUser() const;
	std::string getHost() const;
	std::string getUserPrefix() const;
	bool getWelcome() const;
	bool getAuthed() const;
	int getFd() const;
	int getNJoinedChannels() const;
	std::vector<Channel*>::iterator getJoinedChannelsBegin();
	std::vector<Channel*>::iterator getJoinedChannelsEnd();

	//Setters
	void setNick(std::string nick);
	void setUser(std::string user);
	void setHost(std::string host);
	void setWelcome();
	void setAuthed();
	void setFd(int fd);

	//Methods
	void joinChannel(Channel* channel);
	void partChannel(Channel* channel);

};

std::ostream&	operator << (std::ostream& out, const User &user);

#endif // USER_HPP
