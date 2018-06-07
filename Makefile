CC=g++
FLAGS=-lm -lpthread -Wno-write-strings

all: util backup server client

util:
	$(CC) -o lib/util.o -c lib/dropboxUtil.cpp $(FLAGS) 

backup:
	$(CC) -o server/backup.o -c server/dropboxBackupServer.cpp $(FLAGS) 

server: util
	$(CC) server/dropboxServer.cpp lib/util.o server/backup.o -o server/server $(FLAGS) 

client: util
	$(CC) client/dropboxClient.cpp lib/util.o -o client/client $(FLAGS) 

clean:
	rm lib/*.o
	rm server/server
	rm client/client

silent: util_silent backup_silent server_silent client_silent

util_silent:
	@$(CC) -o lib/util.o -c lib/dropboxUtil.cpp $(FLAGS) 

backup_silent:
	@$(CC) -o server/backup.o -c lib/dropboxBackupServer.cpp $(FLAGS) 

server_silent: util_silent
	@$(CC) server/dropboxServer.cpp lib/util.o server/backup.o -o server/server $(FLAGS) 

client_silent: util_silent
	@$(CC) client/dropboxClient.cpp lib/util.o -o client/client $(FLAGS) 

clean_silent:
	@rm lib/*.o
	@rm server/server
	@rm client/client