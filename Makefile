NAME =	ircserv
CPPFLAGS = -Wall -Wextra -Werror -g -std=c++98

SRC = main.cpp User.cpp Channel.cpp Server.cpp Commands.cpp

all: $(NAME)

$(NAME):
	@c++ $(CPPFLAGS) -o $@ $(SRC)

clean:

fclean:
	@rm -f $(NAME)

re:	fclean
	@make all

.PHONY: re, fclean, clean
