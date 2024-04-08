// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

int main()
{
    // creating server listen socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket < 0) {
		std::cerr << "Setting up socket failed. SAD" << std::endl;
		return 1;
	}

    // specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    bind(serverSocket, (sockaddr*)&serverAddress,
         sizeof(serverAddress));

    // listening to the assigned socket
    listen(serverSocket, SOMAXCONN);

    // accepting connection request
	sockaddr_in client;
	socklen_t clinentSize = sizeof(client);
    int clientSocket = accept(serverSocket, (sockaddr*)&client, &clinentSize);

	// get host info
	std::string host = inet_ntoa(client.sin_addr);
	std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;

    // closing the server listen socket.
    close(serverSocket);

    // recieving data
    char buffer[1024];
	int welcome = 0;
	std::string nick, user, serverName(":IRCQ+.org");
	while (true) {
		memset(buffer, 0, sizeof(buffer));

		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived < 0) {
			std::cerr << "Error in recv(). SAD" << std::endl;
			return 1;
		}

		if (bytesReceived == 0) {
			std::cout << "Client disconnected" << std::endl;
			break;
		}

		// std::cout << buffer;

		if (!welcome){
			welcome = 1;
			std::stringstream info(buffer);
			std::string word;
			while (info >> word){
				std::cout << "Word: " << word << std::endl;
				if (word == "NICK"){
					info >> word;
					nick = word;
				}
				if (word == "USER"){
					info >> word;
					user = word;
				}
			}
			std::string welcomeMessage(serverName + " 001 " + nick + " :Welcome to IRCQ+ " + nick + "! " + user + "@" + host + "\n");
			send(clientSocket, welcomeMessage.c_str(), welcomeMessage.length(), 0);
		}
		else {
			std::stringstream info(buffer);
			std::string word;
			while (info >> word){
				std::cout << "Word: " << word << std::endl;
				if (word == "QUIT"){
					std::cout << "Client disconnected" << std::endl;
					return 0;
				}
			}
			send(clientSocket, buffer, bytesReceived, 0);
		}
	}

	// closing the client socket
	close(clientSocket);

    return 0;
}
