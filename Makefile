all: util server client

util:
	gcc -o lib/util.o -c lib/dropboxUtil.c -Wall -lm

server: util
	gcc server/dropboxServer.c lib/util.o -o server/server -Wall -lm

client: util
	gcc client/dropboxClient.c lib/util.o -o client/client -Wall -lm

clean:
	rm lib/*.o
	rm server/server
	rm client/client