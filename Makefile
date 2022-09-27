all: build/main.out build/client.out

build/main.out: test/main.cpp
	g++ -g -std=c++2a -I . -O0 $^ -o $@

build/client.out: test/client.cpp
	g++ -g -std=c++2a -I . -O0 $^ -o $@

clean:
	rm build/*

start:
	build/main.out 127.0.0.1 9999
