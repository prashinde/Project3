#!/bin/bash
SERVER_PORT=34001
CLIENT_PORT=34002
./coord 127.0.0.1 $CLIENT_PORT $SERVER_PORT > coord.log &
sleep 4
./server 127.0.0.1 34001 > server1.log &
./server 127.0.0.1 34001 > server2.log &
./server 127.0.0.1 34001 > server3.log &

echo "Coordinator and servers are started. Start client on port $CLIENT_PORT"
