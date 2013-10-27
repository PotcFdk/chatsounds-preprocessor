all: chatsounds-preprocessor

clean:
	rm -f chatsounds-preprocessor

chatsounds-preprocessor: main.cpp
	g++ -std=c++11 -s -O2 -fomit-frame-pointer -o $@ $< -lboost_system -lboost_filesystem -lboost_serialization -lbass
