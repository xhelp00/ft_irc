#include "Channel.hpp"

Channel::Channel(std::string name, std::string topic, std::string key, int maxUsers) : _name(name), _topic(topic), _key(key), _nUsers(0), _maxUsers(maxUsers) {

}

Channel::Channel(const Channel& channel) : _name(channel._name), _topic(channel._topic), _key(channel._key), _users(channel._users), _nUsers(channel._nUsers), _maxUsers(channel._maxUsers) {

}

Channel& Channel::operator = (const Channel& channel) {
	_name = channel._name;
	_topic = channel._topic;
	_key = channel._key;
	_nUsers = channel._nUsers;
	_maxUsers = channel._maxUsers;
	_users = channel._users;
	return *this;
}

Channel::~Channel() {

}

std::string Channel::getName() { return _name; }
std::string Channel::getTopic() { return _topic; }
std::string Channel::getKey() { return _key; }
uint32_t Channel::getNUsers() { return _nUsers; }
uint32_t Channel::getMaxUsers() { return _maxUsers; }
std::vector<User*>::iterator Channel::getUsersBegin() { return _users.begin(); }
std::vector<User*>::iterator Channel::getUsersEnd() { return _users.end(); }

void Channel::setName(std::string name) { _name = name; }
void Channel::setTopic(std::string topic) { _topic = topic; }
void Channel::setKey(std::string key) { _key = key; }
void Channel::setNUsers(uint32_t nUsers) { _nUsers = nUsers; }
void Channel::setMaxUsers(uint32_t maxUsers) { _maxUsers = maxUsers; }

void Channel::addUser(User* user) {
	_users.push_back(user);
	_nUsers++;
}

void Channel::removeUser(User* user) {
	_users.erase(std::remove(_users.begin(), _users.end(), user), _users.end());
	_nUsers--;
}

bool Channel::operator == (const Channel& channel) const { return _name == channel._name; }
bool Channel::operator != (const Channel& channel) const { return _name != channel._name; }
