CC = gcc
CFLAGS = -Wall -Wextra -pthread -Icommon

CLIENT_SRC = client/client.c
SERVER_SRC = server/server.c

CLIENT_BIN = client/client
SERVER_BIN = server/server

all: $(CLIENT_BIN) $(SERVER_BIN)
	
$(CLIENT_BIN):$(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN)

$(SERVER_BIN):$(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN)

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
