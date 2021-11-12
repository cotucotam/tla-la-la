TARGET  = cpld-control

CC     := gcc
SRC     = src
INC     = inc

CFLAGS  = -I$(INC)
CFLAGS += -DLIBFTDI1=1

LIBS    = -lmpsse
LIBS   += -lftdi1
LIBS   += -lusb-1.0

.PHONY: all static clean

all: i2c.o spi.o smi.o cpld.o main.o
	$(CC) -o $(TARGET) $^ $(CFLAGS) $(LIBS)

static: i2c.o spi.o smi.o cpld.o main.o
	$(CC) -o $(TARGET) $^ $(CFLAGS) $(LIBS) -lpthread -static

%.o: $(SRC)/%.c
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o $(SRC)/*~ cpld-control $(INC)/*~