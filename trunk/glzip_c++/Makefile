CC = g++
CFLAGS = -Wall 

SOURCES = test.cc
#HEADERS = determin_skip_list.h
TARGET = test 

.PHONY: all clean tags submit

objects = $(SOURCES:.cc=.o)

$(TARGET): $(objects) 
	$(CC) $(CFLAGS) -o $(TARGET) $(objects)

all: $(TARGET)

clean: 
	@rm -f *.o $(TARGET) 

tags:
	@ctags $(SOURCES)

submit: clean

