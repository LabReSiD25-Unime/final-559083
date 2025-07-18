CC = gcc
CFLAGS = -Wall -Wextra -pthread

CLIENT_DIR = client
SERVER_DIR = server

CLIENT_SRCS = $(CLIENT_DIR)/client.c $(CLIENT_DIR)/funzioni_client.c
SERVER_SRCS = $(SERVER_DIR)/server.c $(SERVER_DIR)/funzioni_server.c $(SERVER_DIR)/comandi.c

CLIENT_EXEC = $(CLIENT_DIR)/client.out
SERVER_EXEC = $(SERVER_DIR)/server.out

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC):
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRCS)

$(SERVER_EXEC):
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRCS)

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC)
