#include "Channel.hpp"

Channel::Channel(std::string name, std::string topic, std::string key, User* chop, int maxUsers) : _name(name), _topic(topic), _key(key), _nUsers(0), _maxUsers(maxUsers) {
	addOperator(chop);
	addUser(chop);
	_modes.i = false;
	_modes.t = true;
}

Channel::Channel(const Channel& channel) : _name(channel._name), _topic(channel._topic), _key(channel._key), _users(channel._users), _operators(channel._operators), _nUsers(channel._nUsers), _maxUsers(channel._maxUsers), _modes(channel._modes) {

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
	std::cout << RED << "Calling Channel destructor of " << _name << END << std::endl;
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
bool Channel::isInviteOnly() const { return _modes.i; }
bool Channel::isTopicOnlyByOperators() const { return _modes.t; }

void Channel::setName(std::string name) { _name = name; }
void Channel::setTopic(std::string topic) { _topic = topic; }
void Channel::setKey(std::string key) {
	_key = key;
	if (key == "")
		std::cout << CYAN << _name << " has no longer key" << END << std::endl;
	else
		std::cout << CYAN << _name << " has a new key \"" << key << "\"" << END << std::endl;
}
void Channel::setNUsers(uint32_t nUsers) { _nUsers = nUsers; }
void Channel::setMaxUsers(uint32_t maxUsers) {
	_maxUsers = maxUsers;
	if (maxUsers == UINT32_MAX)
		std::cout << CYAN << _name << " has no limit of users" << END << std::endl;
	else
		std::cout << CYAN << _name << " has a new limit of users " << maxUsers << END << std::endl;
}

void Channel::setInviteOnly(bool inviteOnly) {
	_modes.i = inviteOnly;
	if (inviteOnly)
		std::cout << CYAN << _name << " is now invite only" << END << std::endl;
	else
		std::cout << CYAN << _name << " is no longer invite only" << END << std::endl;
}
void Channel::setTopicOnlyByOperators(bool topicOnlyByOperators) {
	_modes.t = topicOnlyByOperators;
	if (topicOnlyByOperators)
		std::cout << CYAN << _name << "'s topic can only be set by operators" << END << std::endl;
	else
		std::cout << CYAN << _name << "'s topic can be set by everyone" << END << std::endl;
}

void Channel::addUser(User* user) {
	_users.push_back(user);
	_nUsers++;
}

void Channel::removeUser(User* user) {
	if (isOperator(user))
		removeOperator(user);
	_users.erase(std::find(_users.begin(), _users.end(), user));
	_nUsers--;
}

void Channel::addOperator(User* user) {
	std::cout << CYAN << _name << " " << user->getNick() << " is now a operator" << END << std::endl;
	_operators.push_back(user);
}

void Channel::removeOperator(User* user) {
	std::cout << CYAN << _name << " " << user->getNick() << " is no longer a operator" << END << std::endl;
	_operators.erase(std::find(_operators.begin(), _operators.end(), user));
}

bool Channel::isOperator(User* user) const {
	std::vector<User*>::const_iterator it;
	for (it = _operators.begin(); it != _operators.end(); ++it)
		if (user == (*it))
			return true;
	return false;
}

bool Channel::isUserJoined(User* user) const {
	std::vector<User*>::const_iterator it;
	for (it = _users.begin(); it != _users.end(); ++it)
		if (user == (*it))
			return true;
	return false;
}


std::ostream&	operator << (std::ostream& out, const Channel& channel) {
	out << channel.getName() << " " << channel.getNUsers() << "|" << channel.getMaxUsers() << "	Modes:";
	if (channel.isInviteOnly())
		std::cout << " i";
	if (channel.isTopicOnlyByOperators())
		std::cout << " t";
	if (channel.getKey().length() > 0)
		std::cout << " k:" << channel.getKey();
	if (channel.getMaxUsers() < UINT32_MAX)
		std::cout << " l:" << channel.getMaxUsers();
	std::cout << std::endl;
	for (std::vector<User*>::const_iterator it = channel.getUsersBegin(); it != channel.getUsersEnd(); it++) {
		out << "	";
		if (channel.isOperator((*it)))
			std::cout << "@";
		std::cout << (*it)->getNick() << "| FD: " << (*it)->getFd() << std::endl;
	}
	return out;
}
