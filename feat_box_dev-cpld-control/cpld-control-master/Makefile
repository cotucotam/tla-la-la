TARGET = cpld-control
LIBS = -lftdi

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(TARGET): $(TARGET).o
	$(CC) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf $(TARGET) *.o
