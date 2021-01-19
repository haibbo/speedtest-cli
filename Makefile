#
#
# Makefile for speedtest
#
#

CC = /opt/toolchains/stbgcc-4.8-1.5/bin/mips-linux-gnu-gcc

src_dir = .
rg_top_dir = ../../..
rootfs_top_dir = ../..
user_top_dir = ..

CFLAGS += -Wall -pedantic -std=gnu99
CFLAGS += -g
CFLAGS += -O2 -DNDEBUG
LDLIBS = -lpthread -lm -lssl -lcrypto -lcurl -lexpat

ifneq ($(CROSS_COMPILE),)
	CC = gcc
	CC := $(CROSS_COMPILE)$(CC)
	AR := $(CROSS_COMPILE)$(AR)
endif

INC += -I$(user_top_dir)/curl/include
INC += -I$(rg_top_dir)/rg_apps/userspace/gpl/apps/expat/include

LIB += -L$(user_top_dir)/curl/install/lib
LIB += -L$(rg_top_dir)/rg_apps/userspace/gpl/apps/expat/lib
# LIB += -L$(rg_top_dir)/rg_apps/userspace/public/libs/openssl
LIB += -L$(rootfs_top_dir)/lib/OpenSSL_0.908/lib

# LIB_DIR += -Wl,-rpath=$(user_top_dir)/curl/install/lib
# LIB_DIR += -Wl,-rpath=$(rg_top_dir)/rg_apps/userspace/gpl/apps/expat/lib

BIN=speedtest

.PHONY: all clean romfs

all: $(BIN)

clean:
	-$(RM) *.o
	-$(RM) $(BIN)

speedtest: main.c
	$(CC) $(CFLAGS) $(INC) $(LIB) $(LDLIBS) -o speedtest main.c
# 	$(CC) $(CFLAGS) $(INC) $(LIB) $(LIB_DIR) $(LDLIBS) -o speedtest main.c

romfs:
	$(ROMFSINST) /bin/$(EXEC)