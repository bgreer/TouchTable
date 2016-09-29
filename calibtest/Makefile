
calibtest : main.cpp helper.cpp
	g++ -O3 -o calibtest main.cpp helper.cpp `pkg-config --libs --cflags opencv`

clean :
	rm -rf calibtest *.o
