all: client coord server

client: sys/client.cpp
	g++ -g sys/client.cpp utils/*.cpp -Iutils/ -o client -lpthread
coord: sys/coord.cpp sys/session.cpp sys/transaction.cpp sys/twopc.cpp
	g++ -g sys/coord.cpp sys/session.cpp sys/transaction.cpp sys/twopc.cpp utils/*.cpp -Iutils/ -o coord -lpthread
server: sys/server.cpp
	g++ -g sys/server.cpp utils/*.cpp -Iutils/ -o server -lpthread

clean:
	rm -rf sys/*.o
	rm -rf utils/*.o
	rm -f client coord server
