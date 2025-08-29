INCLUDES = includes

all: server

out:
	mkdir out

server: server.c out
	gcc           \
	-I$(INCLUDES) \
	-o out/server \
	server.c

run-server: server
	./out/server

.PHONY: all run-server
