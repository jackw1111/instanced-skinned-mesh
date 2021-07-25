
INCLUDE_PATH = ./include/
LIB_INCLUDE_PATH = ./lib/include
SRC_PATH = ./src/
OBJ_PATH = ./obj/
FLAGS = 
DEBUG = -g
RELEASE = -O2
BUILD = $(RELEASE)
LIB_PATH = ./lib/
LIBS = -ldl ./lib/libassimp.so -lglfw

INC = $(wildcard $(INCLUDE_PATH)*.h)
_OBJS = $(INC:.h=.o)
OBJS = $(sort $(subst $(INCLUDE_PATH), $(OBJ_PATH), $(_OBJS)) )

all: $(OBJS)
	g++ $(OBJS) -L $(LIB_PATH) $(LIBS) -o demo

$(OBJ_PATH)%.o : $(SRC_PATH)%.cpp
	@echo building $* ...
	@g++ $(BUILD) -c -I $(INCLUDE_PATH) -I $(LIB_INCLUDE_PATH) $(FLAGS) $< -o $@ 

./obj/glad.o : ./src/glad.c
	@echo building glad ...
	@gcc -c -I $(INCLUDE_PATH) -I $(LIB_INCLUDE_PATH) $(FLAGS) $< -o $@


clean:
	rm $(OBJ_PATH)*
	rm demo
