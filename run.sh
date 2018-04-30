#!/bin/bash

user=test_user
port=8080
server=127.0.0.1 #todo use localhost instead of IP

ps -a | grep server | awk '{print $1}' | xargs kill -9

make silent && make clean_silent && make silent

cd server
echo "Cleaning test directory from user $user..."
rm -rf $user

echo "Starting server in background on port $port..."
stdbuf -oL ./server 8080 > server_logs.txt 2>&1 &
cd ..
echo "Server running in background"

echo "Starting client..."
cd client
sleep 0.5
./client $user $server $port

ps -a | grep server | awk '{print $1}' | xargs kill -9