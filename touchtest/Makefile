
touchtest : main.cpp helper.cpp capture.cpp tracker.cpp
	g++ -std=c++11 -O3 -o touchtest main.cpp helper.cpp capture.cpp tracker.cpp `pkg-config --libs --cflags opencv` -pthread

clean :
	rm -rf touchtest *.o
