CC=g++
FLAGS=-lm -lpthread -Wno-write-strings

all: util server client front-end

util:
	$(CC) -o lib/util.o -c lib/dropboxUtil.cpp $(FLAGS)
	$(CC) -o server/election.o -c server/election.cpp $(FLAGS)

server: util
	$(CC) server/dropboxServer.cpp lib/util.o server/election.o -o server/server $(FLAGS) 

client: util
	$(CC) client/dropboxClient.cpp lib/util.o -o client/client $(FLAGS) 

front-end: util
	$(CC) front-end/dropboxFrontEnd.cpp lib/util.o -o front-end/front-end $(FLAGS) 


clean:
	rm lib/*.o
	rm server/server
	rm client/client
	rm front-end/front-end

silent: util_silent server_silent client_silent front-end_silent

util_silent:
	@$(CC) -o lib/util.o -c lib/dropboxUtil.cpp $(FLAGS) 

server_silent: util_silent
	@$(CC) server/dropboxServer.cpp lib/util.o -o server/server $(FLAGS) 

client_silent: util_silent
	@$(CC) client/dropboxClient.cpp lib/util.o -o client/client $(FLAGS) 

front-end_silent: util_silent
	@$(CC) front-end/dropboxFrontEnd.cpp lib/util.o -o front-end/front-end $(FLAGS) 

clean_silent:
	@rm lib/*.o
	@rm server/server
	@rm client/client
	@rm front-end/front-end
