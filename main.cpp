#include "Server.hpp"

int main(int argc, char** argv) {
	if (argc != 3)
		return (std::cerr << "Usage: ./ft_irc <port> <password>" << std::endl, 1);
	else if (atoi(argv[1]) < 1024 || atoi(argv[1]) > 65535)
		return (std::cerr << "Port must be between 1024 and 65535" << std::endl, 1);
	else if (strlen(argv[2]) < 1 || strlen(argv[2]) > 30)
		return (std::cerr << "Password must be between 1 and 30 characters" << std::endl, 1);

	try {
		Server server = Server(atoi(argv[1]), argv[2]);

		struct epoll_event events[5];
		int running = 1;
		// recieving data
		while (running) {
			int nfds = epoll_wait(server.getEpollSocket(), events, 5, -1);
			if (nfds == -1)
				throw std::runtime_error("Failed to wait for events");

			for (int i = 0; i < nfds; i++) {
				User *user = reinterpret_cast<User*>(events[i].data.ptr);
				if (user->getFd() == server.getServerSocket())
					server.acceptConnection();
				else if (server.recvMessage(user) == -1)
					running = 0;
			}
		}
		return 0;

	}
	catch (std::exception &e) { return (std::cerr << e.what() << std::endl, 1); }

	return 0;
}
