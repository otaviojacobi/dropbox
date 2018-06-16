# Dropbox

## Using this repository
1. Clone this repository with ```git clone https://github.com/otaviojacobi/dropbox.git```and navigate to the root directory with ```cd dropbox```.
 
2. In the root directory you can type ```make```to buid all dependencies. You can also use ```make server```, ```make client``` or ```make util``` for just recompiling individual parts. ```make clean```will clean all binary files.

## Using the socket library
1. After building everything, with one terminal go to the server folder and type:  
```
./server (port) 0
```
Example:
```
./server 8080 0
```
This will start a leader server listening on your device localhost (127.0.0.1) on the port 8080. If you don't specify a port, it will listen on the default port 8888.

1.1. If you want some sever backup, open another terminal, go to the server folder and type: 
```
./server (leader port) -1 (this backup port) (leader device)
```
Example:
```
./server 8080 -1 8081 127.0.0.1
```
This will start a backup server listening on your device localhost (127.0.0.1) on the port 8000. And it will tell to the leader server on port 8080 its existence, so it is going to receive all updates from the leader. So, if the leader dies, the backups make an election and choose a new leader.

2. Open another terminal, go to the front-end folder and type:
```
./front-end  (this front-end port) (leader device) (leader port)
```
Example:
```
./front-end  8000 127.0.0.1 8080
```
This will start the front-end for your client. It will be the bridge between the client and the current leader server. So if the leader server stops, this front-end will contact with the backup that won the election and the client won't know anything happend.

3. Open another terminal, go to the client folder and type:
```
./client (username) (front-end device) (front-end port)
```
Example:
```
./client test 127.0.0.1 8000
```
This will make a client connect to the frontend (which is listening on 127.0.0.1) on the specified port(8000). The first parameter is just your username acording to [dropbox especification](https://moodle.inf.ufrgs.br/pluginfile.php/122129/mod_resource/content/1/INF01151-Trabalho_pt1-v2.pdf).

Now, on the client terminal you should see a ```>> ``` sign, this indicates that you're on the dropbox main terminal. When this project is ready you  should be able to use the following commands: 

* ```Upload <filename>```: Upload files to server. For now you can just type something and the server will recieve it as a message.
* ```Download <filename>```: Download files from server.
* ```List_server```: Lists all files on the server.
* ```List_client```: Lists all files on the client.
* ```Get_sync_dir```: Sync client and server files. Not necessary to call this command, it happens automatically.
* ```Delete <filename>```: Delete file from server.
* ```Exit```: Exit the command line. Finished.

