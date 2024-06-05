main:threadpool.o test.o
	g++ threadpool.o test.o -o main
test.o:test.cpp
	g++ -c test.cpp
threadpool.o:threadpool.cpp
	g++ -c threadpool.cpp