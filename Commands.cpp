#include "Server.hpp"

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

void Server::TOPIC(std::stringstream& info, User* user) {
	std::string channelTo, topic;
	info >> channelTo;
	std::getline(info, topic);

	topic.erase(0, topic.find_first_not_of(" 	:"));
	// std::cout << channelTo << " : " << topic << "\n";
	std::vector<Channel*>::iterator found;
	for (found = _channels.begin(); found != _channels.end(); ++found)
		if (channelTo == (*found)->getName())
			break;

	//ERR_NOSUCHCHANNEL
	if (found == _channels.end())
		reply(user, "", "403", "", channelTo + " :No such channel");
	else if (found != _channels.end()) {
		//ERR_NOTONCHANNEL
		if (!(*found)->isUserJoined(user))
			reply(user, "", "442", "", user->getNick() + " :You're not on that channel");
		//ERR_CHANOPRIVSNEEDED
		else if (!(*found)->isOperator(user) && (*found)->isTopicOnlyByOperators())
			reply(user, "", "482", "", (*found)->getName() + " :You're not channel operator");
		//Remove topic
		else if (topic == "") {
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

void Server::NICK(std::stringstream& info, User* user) {
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
	if ((foundUser != _users.end())) {
		reply(user, "", "433 *", nick, ":Nickname is already in use.");
		correctNick = 0;
	}

	//Setting the nickname to user if it's correct
	if (correctNick) {
		if (user->getWelcome()) {
			for(std::vector<Channel*>::iterator cha = _channels.begin(); cha != _channels.end(); cha++) {
				if ((*cha)->isUserJoined(user))
					for(std::vector<User*>::iterator us = (*cha)->getUsersBegin(); us != (*cha)->getUsersEnd(); us++)
						if ((*us)->getNick() != user->getNick())
							reply((*us), user->getUserPrefix(), "NICK", ":" + nick, "");
			}
			reply(user, user->getUserPrefix(), "NICK", ":" + nick, "");
		}
		user->setNick(nick);
	}
}

void Server::PRIVMSG(std::stringstream& info, User* user) {
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
		if ((foundUser == _users.end()))
			reply(user, "", "401", "", msgtarget + ":No such nick/channel");
		else
			reply((*foundUser), user->getUserPrefix(), "PRIVMSG", msgtarget, ":" + textToSend);
	}
}

void Server::JOIN(std::stringstream& info, User* user) {
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
			// std::cout << "User {fd: " << user->getFd() << " nick: " << user->getNick() << "} is trying to join channel " + name + " with key " + key << std::endl;
			std::vector<Channel*>::iterator cha;
			for (cha = _channels.begin(); cha != _channels.end(); ++cha)
				if (name == (*cha)->getName())
					break;

			//Create a new channel
			if (cha == _channels.end()) {
				addChannel(new Channel(name, "", "", user, 5));
				std::vector<Channel*>::iterator create = _channels.end() - 1;

				//JOIN MESSAGE
				reply(user, user->getUserPrefix(), "JOIN", ":" + (*create)->getName(), "");

				//RPL_TOPIC
				reply(user, "", "332", "", (*create)->getName() + " :" + (*create)->getTopic());

				//RPL_NAMREPLY
				reply(user, "", "353", "", "= " + (*create)->getName() + " :@" + user->getNick());

				//RPL_ENDOFNAMES
				reply(user, "", "366", "", (*create)->getName() + " :End of Names list");
			}
			//ERR_CHANNELISFULL
			else if ((*cha)->getNUsers() >= (*cha)->getMaxUsers())
				reply(user, "", "471", "", (*cha)->getName() + " :Cannot join channel (+l) - is full");
			//ERR_BADCHANNELKEY
			else if ((*cha)->getKey() != key)
				reply(user, "", "475", "", (*cha)->getName() + " :Cannot join channel (+k) - bad key");
			//ERR_INVITEONLYCHAN
			else if ((*cha)->isInviteOnly())
				reply(user, "", "473", "", (*cha)->getName() + " :Cannot join channel (+i) - invite only");
			else {
				(*cha)->addUser(user);

				//JOIN MESSAGE to all users of that channel
				for(std::vector<User*>::iterator us = (*cha)->getUsersBegin(); us != (*cha)->getUsersEnd(); ++us)
					reply((*us), user->getUserPrefix(), "JOIN", ":" + (*cha)->getName(), "");

				//RPL_TOPIC
				reply(user, "", "332", "", (*cha)->getName() + " :" + (*cha)->getTopic());

				//RPL_NAMREPLY
				std::string Message("= " + (*cha)->getName() + " :");
				for (std::vector<User*>::iterator it = (*cha)->getUsersBegin(); it != (*cha)->getUsersEnd(); ++it) {
					if ((*cha)->isOperator((*it)))
						Message.append("@");
					if (it != (*cha)->getUsersEnd() - 1)
						Message.append((*it)->getNick() + " ");
					else
						Message.append((*it)->getNick());
				}
				reply(user, "", "353", "", Message);

				//RPL_ENDOFNAMES
				reply(user, "", "366", "", (*cha)->getName() + " :End of Names list");
			}
		}
	}
}

void Server::PART(std::stringstream& info, User* user) {
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
		// std::cout << "User {fd: " << user->getFd() << " nick: " << user->getNick() << "} is trying to leave channel " + name + " with message " + partMessage << std::endl;
		std::vector<Channel*>::iterator found;
		for (found = _channels.begin(); found != _channels.end(); ++found)
			if (name == (*found)->getName())
				break;

		//ERR_NOSUCHCHANNEL
		if (found == _channels.end())
			reply(user, "", "403", "", name + " :No such channel");
		else {
			//ERR_NOTONCHANNEL
			if (!(*found)->isUserJoined(user))
				reply(user, "", "442", "", name + " :You're not on that channel");
			else {
				for(std::vector<User*>::iterator it = (*found)->getUsersBegin(); it != (*found)->getUsersEnd(); ++it) {
					if (partMessage.length() > 0)
						reply((*it), user->getUserPrefix(), "PART", (*found)->getName(), partMessage);
					else
						reply((*it), user->getUserPrefix(), "PART", (*found)->getName(), "");
				}
				(*found)->removeUser(user);
			}
		}
		if ((*found)->getNUsers() <= 0)
			removeChannel((*found));
	}
}

void Server::KICK(std::stringstream& info, User* user) {
	std::string line, channelsToKicfFrom, channel, namesToKick, name, kickMessage;
	std::getline(info, line);

	line.erase(0, line.find_first_not_of(" 	"));

	//ERR_NEEDMOREPARAMS
	if (line.length() <= 0)
		reply(user, "", "461", "", "KICK :Not enough parameters");
	else {

		std::stringstream arguments(line);

		arguments >> channelsToKicfFrom;
		arguments >> namesToKick;
		std::getline(arguments, kickMessage);

		kickMessage.erase(0, kickMessage.find_first_not_of(" 	:"));

		std::stringstream channels(channelsToKicfFrom), names(namesToKick);

		while (std::getline(channels, channel, ',')) {
			// std::cout << "User {fd: " << user->getFd() << " nick: " << user->getNick() << "} is trying to join channel " + name + " with key " + key << std::endl;
			std::vector<Channel*>::iterator cha;
			for (cha = _channels.begin(); cha != _channels.end(); ++cha)
				if (channel == (*cha)->getName())
					break;

			//ERR_NOSUCHCHANNEL
			if (cha == _channels.end())
				reply(user, "", "403", "", channel + " :No such channel");
			else {
				//ERR_NOTONCHANNEL
				if (!(*cha)->isUserJoined(user))
					reply(user, "", "442", "", name + " :You're not on that channel");
				//ERR_CHANOPRIVSNEEDED
				else if (!(*cha)->isOperator(user))
					reply(user, "", "482", "", (*cha)->getName() + " :You're not channel operator");
				else {
					while (std::getline(names, name, ',')) {
						std::vector<User*>::iterator us;
						for (us = (*cha)->getUsersBegin(); us != (*cha)->getUsersEnd(); ++us)
							if ((*us)->getNick() == name)
								break;
						//ERR_USERNOTINCHANNEL
						if (us == (*cha)->getUsersEnd())
							reply(user, "", "441", "", name + " " + (*cha)->getName() + " :They aren't on that channel");
						else {
							for(std::vector<User*>::iterator it = (*cha)->getUsersBegin(); it != (*cha)->getUsersEnd(); ++it) {
								if (kickMessage.length() > 0)
									reply((*it), user->getUserPrefix(), "KICK", (*cha)->getName(), (*us)->getNick() + " :" + kickMessage);
								else
									reply((*it), user->getUserPrefix(), "KICK", (*cha)->getName(), (*us)->getNick() + " :" + user->getNick());
							}
							(*cha)->removeUser(*us);
						}
					}
				}
			}
		}
	}
}

void Server::MODE(std::stringstream& info, User* user) {
	std::string line, target, modes;
	std::getline(info, line);

	std::stringstream arguments(line);

	arguments >> target;
	arguments >> modes;

	//ERR_NEEDMOREPARAMS
	if (target.length() <= 0)
		reply(user, "", "461", "", "MODE :Not enough parameters");
	//Channel mode
	else if ((*target.begin()) == '#') {
		std::vector<Channel*>::iterator foundServer;
		for (foundServer = _channels.begin(); foundServer != _channels.end(); ++foundServer)
			if (target == (*foundServer)->getName())
				break;
		//ERR_NOSUCHNICK
		if (foundServer == _channels.end())
			reply(user, "", "401", "", target + ":No such nick/channel");
		//ERR_CHANOPRIVSNEEDED
		else if (!(*foundServer)->isOperator(user) && modes.length() > 0)
			reply(user, "", "482", "", (*foundServer)->getName() + " :You're not channel operator");
		//RPL_CHANNELMODEIS
		else if (modes.length() <= 0) {
			std::string Message(" ");
			if ((*foundServer)->isInviteOnly())
				Message.append("i");
			if ((*foundServer)->isTopicOnlyByOperators())
				Message.append("t");
			if ((*foundServer)->getKey().length() > 0)
				Message.append("k");
			if ((*foundServer)->getMaxUsers() < UINT32_MAX)
				Message.append("l");
			if ((*foundServer)->getKey().length() > 0)
				Message.append(" " + (*foundServer)->getKey());
			if ((*foundServer)->getMaxUsers() < UINT32_MAX) {
				std::stringstream ss;
				ss << (*foundServer)->getMaxUsers();
				std::string num;
				ss >> num;
				Message.append(" " + num);
			}
			reply(user, "", "324", "", (*foundServer)->getName() + Message);
		}
		else {
			char sign = 0;
			for (std::string::iterator m = modes.begin(); m != modes.end(); ++m) {
				switch (*m) {
					case '-':
					case '+':
						sign = *m;
						break;

					//Give/take channel operator privilege
					case 'o':
						if (sign == 0)
							reply(user, "", "461", "", "MODE :Not enough parameters");
						else {
							std::string nick;
							arguments >> nick;
							nick.erase(0, nick.find_first_not_of(" 	"));
							if (nick.length() > 0) {
								std::vector<User*>::iterator foundUser;
								for (foundUser = (*foundServer)->getUsersBegin(); foundUser != (*foundServer)->getUsersEnd(); ++foundUser)
									if (nick == (*foundUser)->getNick())
										break;
								// std::cout << "DEBUG::: " << nick << std::endl;
								//ERR_USERNOTINCHANNEL
								if (foundUser == (*foundServer)->getUsersEnd())
									reply(user, "", "441", "", nick + " " + (*foundServer)->getName() + " :They aren't on that channel");
								//Deoperate user
								else if (sign == '-')
									(*foundServer)->removeOperator(*foundUser);
								//Give user operator status
								else if (sign == '+')
									(*foundServer)->addOperator(*foundUser);
							}
							//ERR_NEEDMOREPARAMS
							else
								reply(user, "", "461", "", "MODE :Not enough parameters");
						}
						break;

					//Set/remove the user limit to channel
					case 'l':
						if (sign == 0)
							reply(user, "", "461", "", "MODE :Not enough parameters");
						else {
							uint32_t limit;
							arguments >> limit;
							if (limit <= 1 || limit > UINT32_MAX)
								reply(user, "", "461", "", "MODE :Not enough parameters");
							//Remove user limit on channel
							else if (sign == '-')
								(*foundServer)->setMaxUsers(UINT32_MAX);
							//Add user limit on channel
							else if (sign == '+')
								(*foundServer)->setMaxUsers(limit);
						}
						break;

					//Set/remove the channel key (password)
					case 'k':
						if (sign == 0)
							reply(user, "", "461", "", "MODE :Not enough parameters");
						else {
							std::string key;
							arguments >> key;
							key.erase(0, key.find_first_not_of(" 	"));

							//Remove channel key
							if (sign == '-')
								(*foundServer)->setKey("");
							else if (key.length() <= 0)
								reply(user, "", "461", "", "MODE :Not enough parameters");
							//Add channel key
							else if (sign == '+')
								(*foundServer)->setKey(key);
						}
						break;

					//Set/remove Invite-only channel
					case 'i':
						if (sign == 0)
							reply(user, "", "461", "", "MODE :Not enough parameters");
						else {
							if (sign == '-')
								(*foundServer)->setInviteOnly(false);
							else if (sign == '+')
								(*foundServer)->setInviteOnly(true);
						}
						break;

					//Set/remove the restrictions of the TOPIC command to channel operators
					case 't':
						if (sign == 0)
							reply(user, "", "461", "", "MODE :Not enough parameters");
						else {
							if (sign == '-')
								(*foundServer)->setTopicOnlyByOperators(false);
							else if (sign == '+')
								(*foundServer)->setTopicOnlyByOperators(true);
						}
						break;

					//ERR_UNKNOWNMODE
					default:
						std::string ch(1, *m);
						reply(user, "", "472", "", ch + " :is unknown mode char to me for " + target);
						break;
				}
			}
		}
	}
}

void Server::INVITE(std::stringstream& info, User* user) {
	(void) info, (void) user;
}


