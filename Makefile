export DEBUGBUILD	=	false
ifeq ($(DEBUGBUILD), true)
	DFLAGS		=	-ggdb
else
	DFLAGS		=	-O3
endif

export	CPP		=	g++
export	CPP-5		=	g++-5
export	CPP-6		=	g++-6
export	CLANG		=	clang++
export	CC		=	gcc
export	CXX		=	$(CPP-5)

export	CXXFLAGS	= 	-std=c++14 -W -Wall -Wextra -I./include/ -I./ $(DFLAGS)
export	CFLAGSEXT	= 	-I./include -I./ $(DFLAGS)

NAME			=	linq
SRC			=	./overhead.cpp
OBJ			=	$(SRC:.cpp=.o)

all	: $(NAME)

clean	:
	rm -rf $(OBJ)
fclean	: clean
	rm -rf $(NAME)

re	: fclean all

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME) $(CFLAGSEXT)
run:
	./$(NAME)
