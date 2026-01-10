CC = gcc
CFLAGS = -Wall -Wextra -pthread -Icommon
LDFLAGS = -pthread -lm

CLIENT_SRC = client/client.c
SERVER_SRC = server/server.c

CLIENT_BIN = client/client
SERVER_BIN = server/server

.PHONY: all cllient server clean

all: $(CLIENT_BIN) $(SERVER_BIN)
	
client: $(CLIENT_BIN)
$(CLIENT_BIN):$(CLIENT_SRC) common/common.h
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN)

server: $(SERVER_BIN)
$(SERVER_BIN):$(SERVER_SRC) common/common.h
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_BIN)

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN) *.fifo
