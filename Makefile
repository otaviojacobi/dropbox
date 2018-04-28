CC=gcc
FLAGS=-Wall -lm -lpthread

all: util server client

util:
	$(CC) -o lib/util.o -c lib/dropboxUtil.c $(FLAGS) 

server: util
	$(CC) server/dropboxServer.c lib/util.o -o server/server $(FLAGS) 

client: util
	gcc client/dropboxClient.c lib/util.o -o client/client $(FLAGS) 

clean:
	rm lib/*.o
	rm server/server
	rm client/client