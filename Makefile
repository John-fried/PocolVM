TARGET = pocol
CFLAGS = --std=c99
PROFLAGS = -O2 -s
DEBUGFLAGS = -g
SRCS = $(wildcard *.c)

all: CFLAGS += $(PROFLAGS)
all: $(TARGET)

debug: CFLAGS += $(DEBUGFLAGS)
debug: $(TARGET)

$(TARGET):
	gcc $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm $(TARGET)
