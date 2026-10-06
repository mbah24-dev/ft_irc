# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/04/11 22:23:31 by mbah              #+#    #+#              #
#    Updated: 2026/10/06 14:25:11 by zcherif          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Program name
NAME		= ircserv

# Compiler and flags
CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -g -MMD -MP
CPPFLAGS	= -I./src/core/channel \
			  -I./src/core/request \
			  -I./src/core/responseCodes \
			  -I./src/core/server \
			  -I./src/core/user \
			  -I./src/utils

# Directories
OBJ_DIR		= obj

MAIN_SRC	= main.cpp
SERVER_SRCS	= src/core/server/Server.cpp
USER_SRCS	= src/core/user/User.cpp
CHANNEL_SRCS= src/core/channel/Channel.cpp
REQUEST_SRCS= src/core/request/Request.cpp
UTILS_SRCS	= 
CHANNEL_MODES_SRCS = src/core/channel/ChannelModes.cpp
SERVER_CMD_SRCS = src/core/server/ServerUserCommands.cpp \
			  src/core/server/ServerChannelCommands.cpp \
			  src/core/server/ServerAdminCommands.cpp
SERVER_RESP_SRCS = src/core/server/ServerResponses.cpp
SERVER_NICK_UTILS_SRCS = src/core/server/nickUtils.cpp

SRCS		= $(MAIN_SRC) \
			  $(SERVER_SRCS) \
			  $(SERVER_CMD_SRCS) \
			  $(SERVER_RESP_SRCS) \
			  $(SERVER_NICK_UTILS_SRCS) \
			  $(USER_SRCS) \
			  $(CHANNEL_SRCS) \
			  $(REQUEST_SRCS) \
			  $(CHANNEL_MODES_SRCS) \
			  $(UTILS_SRCS)

OBJS		= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS		= $(OBJS:.o=.d)

.PHONY: all clean fclean re

all: $(OBJ_DIR) $(NAME)

# Create object directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/src
	@mkdir -p $(OBJ_DIR)/src/core
	@mkdir -p $(OBJ_DIR)/src/core/channel
	@mkdir -p $(OBJ_DIR)/src/core/request
	@mkdir -p $(OBJ_DIR)/src/core/responseCodes
	@mkdir -p $(OBJ_DIR)/src/core/server
	@mkdir -p $(OBJ_DIR)/src/core/user
	@mkdir -p $(OBJ_DIR)/src/utils

# Compile source files into object files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Link object files into executable
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

# Include dependency files
-include $(DEPS)

clean:
	@rm -rf $(OBJ_DIR)
	@echo "Object files removed."

fclean: clean
	@rm -f $(NAME)
	@echo "Executable removed."

re: fclean all
	@echo "Project rebuilt successfully."

help:
	@echo "Available targets:"
	@echo "  all      - Build the program (default)"
	@echo "  clean    - Remove object files"
	@echo "  fclean   - Remove object files and executable"
	@echo "  re       - Rebuild everything"
	@echo "  help     - Display this help message"

show:
	@echo "Sources:"
	@echo "  $(SRCS)" | tr ' ' '\n' | sed 's/^/    /'
	@echo ""
	@echo "Objects:"
	@echo "  $(OBJS)" | tr ' ' '\n' | sed 's/^/    /'
	