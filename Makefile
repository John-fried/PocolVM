TARGET = pocol
SRCS = $(wildcard *.c)

all: $(TARGET)

$(TARGET):
	gcc --std=c99 -O2 -s -o $(TARGET) $(SRCS)
clean:
	rm $(TARGET)
