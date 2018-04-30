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

silent: util_silent server_silent client_silent

util_silent:
	@$(CC) -o lib/util.o -c lib/dropboxUtil.c $(FLAGS) 

server_silent: util_silent
	@$(CC) server/dropboxServer.c lib/util.o -o server/server $(FLAGS) 

client_silent: util_silent
	@gcc client/dropboxClient.c lib/util.o -o client/client $(FLAGS) 

clean_silent:
	@rm lib/*.o
	@rm server/server
	@rm client/client