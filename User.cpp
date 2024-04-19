#include "User.hpp"

User::User(std::string nick, std::string user, std::string host) : _nick(nick), _user(user), _host(host), _userPrefix(""), _welcome(false), _authed(false), _fd(0), _nJoinedChannels(0) {

}

User::User(const User& user) : _nick(user._nick), _user(user._user), _host(user._host), _userPrefix(user._userPrefix), _joinedChannels(user._joinedChannels), _welcome(user._welcome), _authed(user._authed), _fd(user._fd), _nJoinedChannels(user._nJoinedChannels) {

}

User& User::operator = (const User& user) {
	_nick = user._nick;
	_user = user._user;
	_host = user._host;
	_userPrefix = user._userPrefix;
	_joinedChannels = user._joinedChannels;
	_welcome = user._welcome;
	_authed = user._authed;
	_fd = user._fd;
	_nJoinedChannels = user._nJoinedChannels;
	return *this;
}

User::~User() {

}

std::string User::getNick() const { return _nick; }
std::string User::getUser() const { return _user; }
std::string User::getHost() const { return _host; }
std::string User::getUserPrefix() const { return _userPrefix; }
bool User::getWelcome() const { return _welcome; }
bool User::getAuthed() const { return _authed; }
int User::getFd() const { return _fd; }
int User::getNJoinedChannels() const { return _nJoinedChannels; }
std::vector<Channel*>::iterator User::getJoinedChannelsBegin() { return _joinedChannels.begin(); }
std::vector<Channel*>::iterator User::getJoinedChannelsEnd() { return _joinedChannels.end(); }

void User::setNick(std::string nick) { _nick = nick; }
void User::setUser(std::string user) { _user = user; }
void User::setHost(std::string host) { _host = host; }
void User::setUserPrefix(std::string userPrefix) { _userPrefix = userPrefix; }
void User::setWelcome() { _welcome = true; }
void User::setAuthed() { _authed = true; }
void User::setFd(int fd) { _fd = fd; }

void User::joinChannel(Channel* channel) {
	_joinedChannels.push_back(channel);
	_nJoinedChannels++;
}

void User::partChannel(Channel* channel) {
	_joinedChannels.erase(std::remove(_joinedChannels.begin(), _joinedChannels.end(), channel), _joinedChannels.end());
	_nJoinedChannels--;
}

bool User::operator == (const User& user) const { return _nick == user._nick; }
bool User::operator != (const User& user) const { return _nick != user._nick; }






