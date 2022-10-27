all: build/main.out build/client.out build/coro_test.out build/coro_client.out

build/main.out: test/main.cpp
	g++ -g -I . -O0 -fcoroutines -Wall -pthread -std=c++2a $^ -o $@

build/client.out: test/client.cpp
	g++ -g -I . -O0 -fcoroutines -Wall -pthread -std=c++2a $^ -o $@

build/coro_test.out: test/coro_test.cpp
	g++ -g -I . -O0 -fcoroutines -Wall -pthread -std=c++2a $^ -o $@	

build/coro_client.out: test/coro_client.cpp
	g++ -g -I . -O0 -fcoroutines -Wall -pthread -std=c++2a $^ -o $@	


clean:
	rm build/*

start:
	./build/main.out 127.0.0.1 8022

coro_test:
	./build/coro_test.out 127.0.0.1 8022