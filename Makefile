all: client coord

client: sys/client.cpp
	g++ -g sys/client.cpp utils/*.cpp -Iutils/ -o client -lpthread
coord: sys/coord.cpp sys/session.cpp
	g++ -g sys/coord.cpp sys/session.cpp utils/*.cpp -Iutils/ -o coord -lpthread

clean:
	rm -rf sys/*.o
	rm -rf utils/*.o
	rm -f client coord