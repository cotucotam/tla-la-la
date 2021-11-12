TARGET = i2c-bitbang-demo
LIBS = -lmpsse

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(TARGET): $(TARGET).o
	$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf $(TARGET) *.o
