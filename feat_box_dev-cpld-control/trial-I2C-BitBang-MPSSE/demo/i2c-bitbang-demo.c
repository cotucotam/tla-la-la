#include <stdio.h>
#include <stdlib.h>
#include <mpsse.h>

#define PIN_SDA 0x80
#define PIN_SCL 0x40

#define SDA     7
#define SCL     6

#define ACK     0
#define NAK     1

#define TIME    100

uint8_t started = 0;
uint8_t PORT = 0x3F;    // set SCL & SDA as input

void I2C_delay(void)
{ 
    int i;

    for (i = 0; i < TIME / 2; ++i) {
        ;
    }
}

/*--------------------------------------------------------------------------------
Function	: I2C_read_SDA
Purpose		: set SDA as input
Parameters	: mpsse     - MPSSE structure
Return		: return logic level of SDA, normal is HIGH due to pullup resistor
--------------------------------------------------------------------------------*/
uint8_t I2C_read_SDA(struct mpsse_context * mpsse)
{
    PORT &= ~PIN_SDA;
    SetDirection(mpsse, PORT);
    return PinState(mpsse, SDA, -1);
}

/*--------------------------------------------------------------------------------
Function	: I2C_read_SCL
Purpose		: set SCL as input
Parameters	: mpsse     - MPSSE structure
Return		: return logic level of SCL, normal is HIGH due to pullup resistor
--------------------------------------------------------------------------------*/
uint8_t I2C_read_SCL(struct mpsse_context * mpsse)
{
    PORT &= ~PIN_SCL;
    SetDirection(mpsse, PORT);
    return PinState(mpsse, SCL, -1);
}

/*--------------------------------------------------------------------------------
Function	: I2C_clear_SDA
Purpose		: set SDA as output, clear SDA to LOW
Parameters	: mpsse     - MPSSE structure
Return		: none
--------------------------------------------------------------------------------*/
void I2C_clear_SDA(struct mpsse_context * mpsse)
{
    PORT |= PIN_SDA;
    SetDirection(mpsse, PORT);
}

/*--------------------------------------------------------------------------------
Function	: I2C_clear_SCL
Purpose		: set SCL as output, clear SCL to LOW
Parameters	: mpsse     - MPSSE structure
Return		: none
--------------------------------------------------------------------------------*/
void I2C_clear_SCL(struct mpsse_context * mpsse)
{
    PORT |= PIN_SCL;
    SetDirection(mpsse, PORT);
}

/*--------------------------------------------------------------------------------
Function	: I2C_start 
Purpose		: Send a start signal
Parameters	: mpsse     - MPSSE structure
Return		: none
--------------------------------------------------------------------------------*/
void I2C_start(struct mpsse_context *mpsse)
{
    if (started == 1)
    {
        I2C_read_SDA(mpsse);
        I2C_delay();
        I2C_read_SCL(mpsse);
        while (I2C_read_SCL(mpsse) == 0);
    }

    I2C_delay();

    I2C_clear_SDA(mpsse);
    I2C_delay();
    I2C_clear_SCL(mpsse);
    I2C_delay();
    started = 1;
}

/*--------------------------------------------------------------------------------
Function	: I2C_stop 
Purpose		: Send a stop signal
Parameters	: mpsse     - MPSSE structure
Return		: none
--------------------------------------------------------------------------------*/
void I2C_stop(struct mpsse_context *mpsse)
{
    I2C_clear_SDA(mpsse);
    I2C_delay();

    I2C_read_SCL(mpsse);
    while (I2C_read_SCL(mpsse) == 0);
    I2C_delay();

    I2C_read_SDA(mpsse);
    I2C_delay();
    started = 0;
}

/*--------------------------------------------------------------------------------
Function	: I2C_write_bit
Purpose		: Write 1 bit to slave
Parameters	: mpsse     - MPSSE structure
              bit       - bit to write
Return		: none
--------------------------------------------------------------------------------*/
void I2C_write_bit(struct mpsse_context * mpsse, uint8_t bit)
{
    if (bit)
        I2C_read_SDA(mpsse);
    else
        I2C_clear_SDA(mpsse);
    I2C_delay();

    I2C_read_SCL(mpsse);
    I2C_delay();
    while (I2C_read_SCL(mpsse) == 0);

    I2C_clear_SCL(mpsse);
}

/*--------------------------------------------------------------------------------
Function	: I2C_read_bit
Purpose		: Read 1 bit from slave
Parameters	: mpsse     - MPSSE structure
Return		: uint8_t
--------------------------------------------------------------------------------*/
uint8_t I2C_read_bit(struct mpsse_context * mpsse)
{
    uint8_t bit;

    I2C_read_SDA(mpsse);
    I2C_delay();

    I2C_read_SCL(mpsse);
    while (I2C_read_SCL(mpsse) == 0);
    I2C_delay();

    bit = I2C_read_SDA(mpsse);
    I2C_clear_SCL(mpsse);

    return bit;
}

/*--------------------------------------------------------------------------------
Function	: I2C_write_byte 
Purpose	    : Write 1 byte to slave
Parameters	: mpsse     - MPSSE structure
              byte      - byte to write
Return		: ACK or NAK
--------------------------------------------------------------------------------*/
uint8_t I2C_write_byte(struct mpsse_context * mpsse, uint8_t byte)
{
    uint8_t ack, index;

    for (index = 0; index < 8; ++index)
    {
        I2C_write_bit(mpsse, (byte & 0x80) != 0);
        byte <<= 1;
    }

    ack = I2C_read_bit(mpsse);

    return ack;
}

/*--------------------------------------------------------------------------------
Function	: I2C_read_byte 
Purpose	    : Read 1 byte from slave
Parameters	: mpsse     - MPSSE structure
              ack       - set ACK/NAK bit after read 1 byte (NAK when send last byte)
Return		: uint8_t
--------------------------------------------------------------------------------*/
uint8_t I2C_read_byte(struct mpsse_context * mpsse, uint8_t ack)
{
    uint8_t byte = 0x00, index;

    for (index = 0; index < 8; ++index)
    {
        byte = (byte << 1) | I2C_read_bit(mpsse);
    }

    I2C_write_bit(mpsse, ack);

    return byte;
}

/*--------------------------------------------------------------------------------
Function	: I2C_write_data
Purpose		: Write n bytes data to slave
Parameters	: mpsse             - MPSSE structure
              device_address    - Device address
              memmory_address   - Register address
              data              - Data to write
              length            - Number of byte need to write
Return		: number of NAK
--------------------------------------------------------------------------------*/
uint8_t I2C_write_data(struct mpsse_context * mpsse,
                       uint8_t device_address,
                       uint16_t memmory_address,
                       uint8_t * data,
                       uint8_t length)
{
    uint8_t ret = 0, index;

    I2C_start(mpsse);

    ret += I2C_write_byte(mpsse, device_address & 0xFE);
    ret += I2C_write_byte(mpsse, (memmory_address >> 8) & 0xFF);
    ret += I2C_write_byte(mpsse, memmory_address & 0xFF);

    for (index = 0; index < length; ++index)
        ret += I2C_write_byte(mpsse, *(data + index));

    I2C_stop(mpsse);

    return ret;
}

/*--------------------------------------------------------------------------------
Function	: I2C_read_data
Purpose		: Read n bytes data from slave
Parameters	: mpsse             - MPSSE structure
              device_address    - Device address
              memmory_address   - Register address
              data              - Data to store values
              length            - Number of byte need to read
Return		: number of NAK
--------------------------------------------------------------------------------*/
uint8_t I2C_read_data(struct mpsse_context * mpsse,
                       uint8_t device_address,
                       uint16_t memmory_address,
                       uint8_t * data,
                       uint8_t length)
{
    uint8_t ret = 0, byte, index;

    I2C_start(mpsse);

    ret += I2C_write_byte(mpsse, device_address & 0xFE);
    ret += I2C_write_byte(mpsse, (memmory_address >> 8) & 0xFF);
    ret += I2C_write_byte(mpsse, memmory_address & 0xFF);

    I2C_start(mpsse);
    ret += I2C_write_byte(mpsse, device_address | 0x01);
    for (index = 0; index < length; ++index) 
        *(data + index) = I2C_read_byte(mpsse, (index + 1 == length) ? NAK : ACK);

    I2C_stop(mpsse);

    return ret;
}

int main(int argc, char * argv[]) {
    struct mpsse_context *ftdi = NULL;
    uint8_t dev = 0xA0;
    uint16_t mem = 0x1234;
    uint32_t data_send = 0, data_get = 0;
    uint8_t length = 0;
    uint8_t ret = EXIT_FAILURE;

    data_send = strtoll(argv[1], NULL, 16);
    length = atoi(argv[2]);

    ftdi = Open(0x0403, 0x6010, BITBANG, 0, 0, IFACE_A, NULL, NULL);
    SetDirection(ftdi, PORT);
    usleep(1000);

    if(ftdi == NULL || ftdi->open == 0)
    {
        printf("Failed to open MPSSE: %s\n", ErrorString(ftdi));
        return EXIT_FAILURE;
    }

    // write data
    ret = I2C_write_data(ftdi, dev, mem, (uint8_t *) &data_send, length);

    printf("Data send to 0x%X at 0x%X: 0x%X | %d\n", dev, mem, data_send, ret);
    usleep(10000);

    // read data
    ret = I2C_read_data(ftdi, dev, mem, (uint8_t *) &data_get, length);

    printf("Data get from 0x%X at 0x%X: 0x%X | %d\n", dev, mem, data_get, ret);
    sleep(1);

    Close(ftdi);

    return EXIT_SUCCESS;
}
