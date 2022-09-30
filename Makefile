all: build/main.out build/client.out

build/main.out: test/main.cpp
	g++ -g -I . -O0 -fcoroutines -Wall -pthread -std=c++2a $^ -o $@

build/client.out: test/client.cpp
	g++ -g -I . -O0 -fcoroutines -Wall -pthread -std=c++2a $^ -o $@

clean:
	rm build/*

start:
	./build/main.out 127.0.0.1 9999