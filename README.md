# ft_irc
This project is about creating your own IRC server.
Internet Relay Chat or IRC is a text-based communication protocol on the Internet. 
It offers real-time messaging that can be either public or private. 
Users can exchange direct messages and join group channels. 
IRCclients connect to IRC servers in order to join channels. 
IRC servers are connected together to form a network.

## General
- Your program should not crash in any circumstances (even when it runs out of memory), and should not quit unexpectedly. If it happens, your project will be considered non-functional and your grade will be 0.
- You have to turn in a Makefile which will compile your source files. It must not relink.
- Your Makefile must at least contain the rules: $(NAME), all, clean, fclean and re.
- Compile your code with c++ and the flags-Wall-Wextra-Werror
- Your code must comply with the C++ 98 standard. Then, it should still compile if you add the flag-std=c++98.
- Try to always develop using the most C++ features you can (for example, choose <cstring> over <string.h>). You are allowed to use C functions, but always prefer their C++ versions if possible.
- Any external library and Boost libraries are forbidden.

<img width="922" alt="image" src="https://github.com/xhelp00/ft_irc/assets/111277585/9c567f58-c373-4eee-a214-6ef6d16f0a33">

You have to develop an IRC server in C++ 98.
You mustn’t develop a client.
You mustn’t handle server-to-server communication.
Your executable will be run as follows: ./ircserv <port> <password> 
- port: The port number on which your IRC server will be listening to for incoming IRC connections.
- password: The connection password. It will be needed by any IRC client that tries to connect to your server.

Even if poll() is mentionned in the subject and the evaluation scale, you can use any equivalent such as select(), kqueue(), or epoll().

## Requirements
- The server must be capable of handling multiple clients at the same time and never hang.
- Forking is not allowed. All I/O operations must be non-blocking.
- Only 1 poll() (or equivalent) can be used for handling all these operations (read, write, but also listen, and so forth).

Because you have to use non-blocking file descriptors, it is possible to use read/recv or write/send functions with no poll() (or equivalent), and your server wouldn’t be blocking. But it would consume more system resources. Thus, if you try to read/recv or write/send in any file descriptor without using poll() (or equivalent), your grade will be 0.

- Several IRC clients exist. You have to choose one of them as a reference. Your reference client will be used during the evaluation process.
- Your reference client must be able to connect to your server without encountering any error.
- Communication between client and server has to be done via TCP/IP (v4 or v6).
- Using your reference client with your server must be similar to using it with any official IRC server. However, you only have to implement the following features:
  - You must be able to authenticate, set a nickname, a username, join a channel, send and receive private messages using your reference client.
  -   All the messages sent from one client to a channel have to be forwarded to every other client that joined the channel.
  -   You must have operators and regular users.
  -   Then, you have to implement the commands that are specific to channel operators:
    -   KICK- Eject a client from the channel
    -   INVITE- Invite a client to a channel
    -   TOPIC- Change or view the channel topic
    -   MODE- Change the channel’s mode:
      i: Set/remove Invite-only channel
      t: Set/remove the restrictions of the TOPIC command to channel operators
      k: Set/remove the channel key (password)
      o: Give/take channel operator privilege
      l: Set/remove the user limit to channel
 
 ## Test
 Verify absolutely every possible error and issue (receiving partial data, low bandwidth, and so forth). To ensure that your server correctly processes everything that you send to it, the following simple test using nc can be done: 
 <img width="901" alt="image" src="https://github.com/xhelp00/ft_irc/assets/111277585/db61aa6b-cd29-4868-8af7-3be001983569">
 
Use ctrl+D to send the command in several parts: ’com’, then ’man’, then ’d\n’. In order to process a command, you have to first aggregate the received packets in order to rebuild it.

 ## Bonus
 - Handle file transfer.
 - Abot.

## Links
Documentation

https://datatracker.ietf.org/doc/html/rfc1459

https://modern.ircdocs.horse/

Example

https://chi.cs.uchicago.edu/chirc/irc_examples.html

Guide

https://ircgod.com/posts/


## Checks
**Basic checks**
- There is a Makefile, the project compiles correctly with the required options, is written in C++, and the executable is called as expected.
- Ask and check how many poll() (or equivalent) are present in the code. There must be only one.
- Verify that the poll() (or equivalent) is called every time before each accept, read/recv, write/send. After these calls, errno should not be used to trigger specific action (e.g. like reading again after errno == EAGAIN).
- Verify that each call to fcntl() is done as follows: fcntl(fd, F_SETFL, O_NONBLOCK); Any other use of fcntl() is forbidden.
- If any of these points is wrong, the evaluation ends now and the final mark is 0.

**Networking**
- The server starts, and listens on all network interfaces on the port given from the command line.
- Using the 'nc' tool, you can connect to the server, send commands, and the server answers you back.
- Ask the team what is their reference IRC client.
- Using this IRC client, you can connect to the server.
- The server can handle multiple connections at the same time. The server should not block. It should be able to answer all demands. Do some test with the IRC client and nc at the same time.
- Join a channel thanks to the appropriate command. Ensure that all messages from one client on that channel are sent to all other clients that joined the channel.

**Networking specials**
- Just like in the subject, using nc, try to send partial commands. Check that the server answers correctly. With a partial command sent, ensure that other connections still run fine.
- Unexpectedly kill a client. Then check that the server is still operational for the other connections and for any new incoming client.
- Unexpectedly kill a nc with just half of a command sent. Check again that the server is not in an odd state or blocked.
- Stop a client (^-Z) connected on a channel. Then flood the channel using another client. The server should not hang. When the client is live again, all stored commands should be processed normally. Also, check for memory leaks during this operation.

**Client Commands basic**
- With both nc and the reference IRC client, check that you can authenticate, set a nickname, a username, join a channel. This should be fine (you should have already done this previously).
- Verify that private messages (PRIVMSG) are fully functional with different parameters.

**Client Commands channel operator**
- With both nc and the reference IRC client, check that a regular user does not have privileges to do channel operator actions. Then test with an operator. All the channel operation commands should be tested (remove one point for each feature that is not working).
