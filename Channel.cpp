#include "Channel.hpp"

Channel::Channel(std::string name, std::string topic, std::string key, User* chop, int maxUsers) : _name(name), _topic(topic), _key(key), _nUsers(0), _maxUsers(maxUsers) {
	addOperator(chop);
	addUser(chop);
}

Channel::Channel(const Channel& channel) : _name(channel._name), _topic(channel._topic), _key(channel._key), _users(channel._users), _operators(channel._operators), _nUsers(channel._nUsers), _maxUsers(channel._maxUsers) {

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

std::string Channel::getName() const { return _name; }
std::string Channel::getTopic() const { return _topic; }
std::string Channel::getKey() const { return _key; }
uint32_t Channel::getNUsers() const { return _nUsers; }
uint32_t Channel::getMaxUsers() const { return _maxUsers; }
std::vector<User*>::iterator Channel::getUsersBegin() { return _users.begin(); }
std::vector<User*>::iterator Channel::getUsersEnd() { return _users.end(); }
std::vector<User*>::const_iterator Channel::getUsersBegin() const { return _users.begin(); }
std::vector<User*>::const_iterator Channel::getUsersEnd() const { return _users.end(); }
std::vector<User*>::iterator Channel::getOperatorsBegin() { return _operators.begin(); }
std::vector<User*>::iterator Channel::getOperatorsEnd() { return _operators.end(); }

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

void Channel::addOperator(User* user) {
	_operators.push_back(user);
}

void Channel::removeOperator(User* user) {
	_operators.erase(std::remove(_operators.begin(), _operators.end(), user), _operators.end());
}

bool Channel::isOperator(User* user) const{
	std::vector<User*>::const_iterator it;
	for (it = _operators.begin(); it != _operators.end(); ++it)
		if (user == (*it))
			return true;
	return false;
}

bool Channel::operator == (const Channel& channel) const { return _name == channel._name; }
bool Channel::operator != (const Channel& channel) const { return _name != channel._name; }

std::ostream&	operator << (std::ostream& out, const Channel& channel) {
	out << channel.getName() << " " << channel.getNUsers() << "|" << channel.getMaxUsers() << std::endl;
	for (std::vector<User*>::const_iterator it = channel.getUsersBegin(); it != channel.getUsersEnd(); it++) {
		out << "	";
		if (channel.isOperator((*it)))
			std::cout << "@";
		std::cout << (*it)->getNick() << "| FD: " << (*it)->getFd() << std::endl;
	}
	return out;
}
