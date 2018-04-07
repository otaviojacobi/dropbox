# Dropbox

## Using this repository
1. Clone this repository with ```git clone https://github.com/otaviojacobi/dropbox.git```and navigate to the root directory with ```cd dropbox```.
 
 2. In the root directory you can type ```make```to buid all dependencies. You can also use ```make server```, ```make client``` or ```make util``` for just recompiling individual parts. ```make clean```will clean all binary files.

## Using the socket library
1. After building everything, with one terminal go to the server folder and type: 
```
./server 8080
```
This will start a server listening on your device localhost (127.0.0.1) on the port 8080. If you don't specify a port, it will listen on the default port 8888.

2. Open another terminal, go to the client folder and type:
```
./client test 127.0.0.1 8080
```
This will make a client connect to the server (which is listening on 127.0.0.1) on the specified port(8080). The first parameter is just for consistency for the [dropbox especification](https://moodle.inf.ufrgs.br/pluginfile.php/122129/mod_resource/content/1/INF01151-Trabalho_pt1-v2.pdf).
