default: build run

build: main.cpp
	g++ -O2 main.cpp -o app

run: app
	./app

clean: 
	rm -rf app

debug: main.cpp
	g++ -g -O2 main.cpp -o app
