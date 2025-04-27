CC := arm-linux-gnueabihf-gcc
CFLAGS := -I../drivers
# 源文件和目标文件
SRCS := $(wildcard *_test.c)
TARGETS := $(patsubst %.c, build/%, $(SRCS)) 

all: | build $(TARGETS)

build:
	mkdir -p build

build/%: %.c
	$(CC) $(CFLAGS) -o $@ $<
	
clean:
	rm -rf build