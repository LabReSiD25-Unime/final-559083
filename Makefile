# Compilatore e flag
CC = gcc
CFLAGS = -Wall -Wextra -pthread

# Directory
CLIENT_DIR = client
SERVER_DIR = server

# File sorgenti
CLIENT_SRCS = $(CLIENT_DIR)/client.c $(CLIENT_DIR)/funzioni_client.c
SERVER_SRCS = $(SERVER_DIR)/server.c $(SERVER_DIR)/funzioni_server.c $(SERVER_DIR)/comandi.c

# File oggetto
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

# Eseguibili
CLIENT_EXEC = $(CLIENT_DIR)/client.out
SERVER_EXEC = $(SERVER_DIR)/server.out
# Regola principale
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Client
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Server
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Regola per .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(CLIENT_DIR)/*.o $(SERVER_DIR)/*.o $(CLIENT_EXEC) $(SERVER_EXEC)
