CFLAGS = -Wall -g -Werror -Wno-error=unused-variable
PORT = 12345
IP_SERVER = 192.168.0.2

all: server subscriber
server: server.cpp common.cpp
subscriber: subscriber.cpp common.cpp

.PHONY: clean run_server run_subscriber
run_server:
	./server ${IP_SERVER} ${PORT}
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM
