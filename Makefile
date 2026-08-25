##
## ZAPPY - Makefile
## Construit les trois binaires: zappy_server, zappy_ai, zappy_gui
##

CC        = gcc
CXX       = g++
CFLAGS    = -Wall -Wextra -std=gnu11 -Iserver/include
CXXFLAGS  = -Wall -Wextra -std=c++17

## --- Sources ---
SERVER_SRC = server/src/main.c \
             server/src/args.c \
             server/src/server.c \
             server/src/client.c \
             server/src/handshake.c \
             server/src/map.c \
             server/src/map_resources.c \
             server/src/clock.c \
             server/src/action.c \
             server/src/commands.c \
             server/src/cmd_look.c \
             server/src/cmd_broadcast.c \
             server/src/direction.c \
             server/src/cmd_fork.c \
             server/src/egg.c \
             server/src/cmd_inventory.c \
             server/src/incant_rules.c \
             server/src/cmd_incant.c \
             server/src/victory.c \
             server/src/hunger.c \
             server/src/player.c \
             server/src/gui.c \
             server/src/gui_query.c \
             server/src/gui_state.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

AI_SRC  = ai/src/main.c
AI_OBJ  = $(AI_SRC:.c=.o)

GUI_SRC = gui/src/main.cpp
GUI_OBJ = $(GUI_SRC:.cpp=.o)

## --- Règles ---
all: zappy_server zappy_ai zappy_gui

zappy_server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJ)

zappy_ai: $(AI_OBJ)
	$(CC) $(CFLAGS) -o $@ $(AI_OBJ)

zappy_gui: $(GUI_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(GUI_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(SERVER_OBJ) $(AI_OBJ) $(GUI_OBJ)

fclean: clean
	rm -f zappy_server zappy_ai zappy_gui

re: fclean all

.PHONY: all clean fclean re
