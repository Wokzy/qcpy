cc = gcc
ld = gcc
include_dir = ./
cflags = -O0 -I$(include_dir)#-lpthread -mavx512f
ldflags = -lm -lc# -lcrypto -lssl

all: server client
# server: ./server
# client: ./client

SRC_SERVER = server.c misc.c #$(wildcard data_structures/*.c)
SRC_OBJ_SERVER = $(SRC_SERVER:.c=.o)

SRC_CLIENT = client.c misc.c
SRC_OBJ_CLIENT = $(SRC_CLIENT:.c=.o)

server : $(SRC_OBJ_SERVER)
	$(ld) $^ $(ldflags) -o $@

client : $(SRC_OBJ_CLIENT)
	$(ld) $^ $(ldflags) -o $@

%.o : %.c
	$(cc) -c $< $(cflags) -o $@

clean:
	rm -rf $(SRC_OBJ_SERVER) $(SRC_OBJ_CLIENT)
