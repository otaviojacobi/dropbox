all: util server client

util:
	gcc -o lib/util.o -c lib/dropboxUtil.c -Wall

server: util
	gcc server/dropboxServer.c lib/util.o -o server/server -Wall

client: util
	gcc client/dropboxClient.c lib/util.o -o client/client -Wall

clean:
	rm lib/*.o
	rm server/server
	rm client/client