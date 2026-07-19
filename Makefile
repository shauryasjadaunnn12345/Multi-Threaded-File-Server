all:
	g++ fileServer.cpp my_functions.cpp -o fileServer  -lpthread -lrt

clean: 
	rm fileServer
	rm -f *.txt