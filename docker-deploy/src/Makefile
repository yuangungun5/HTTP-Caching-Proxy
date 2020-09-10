TARGETS = server

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.o *.~

server:	main.cpp
	g++ -g -pthread -std=c++11 -ggdb3 -o $@ $<

