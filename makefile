TARGET=client server
all: $(TARGET)

server: Server.cpp
	g++-9 -std=c++2a -Wall -pthread -g Server.cpp -o Server -lldap -llber

client: Client.cpp
	g++-9 -std=c++2a -Wall -pthread -g Client.cpp -o Client -lldap -llber

clean:
	rm Server Client