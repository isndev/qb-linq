export DEBUGBUILD		=	false
ifeq ($(DEBUGBUILD), true)
		DFLAGS			=	-ggdb
else
		DFLAGS			=	-O3
endif

export	CPP				=	g++
export	CPP-5			=	g++-5
export	CPP-6			=	g++-6
export	CLANG			=	clang++

export CXXFLAGS		= -W -Wall -Wextra -Werror -I./include/ -I./ $(DFLAGS)
export CFLAGSEXT	= -I./include -I./ $(DFLAGS) $(BUILDFLAGS)

NAME	=	linq
SRC		=	./benchmark.cpp
OBJ		=	$(SRC:.cpp=.o)

all		: $(NAME)
clean	:
	rm -rf $(OBJ)
fclean	: clean
	rm -rf $(NAME)
re		: fclean all

$(NAME): $(OBJ)
	$(CPP) $(CXXFLAGS) $(OBJ) -o $(NAME)
run:
	./$(NAME)
