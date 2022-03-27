.PHONY: all clean

all:
	g++ *.cpp -std=c++17 -lstdc++fs -o serwer


clean:
	rm -f *.o *.gch serwer
