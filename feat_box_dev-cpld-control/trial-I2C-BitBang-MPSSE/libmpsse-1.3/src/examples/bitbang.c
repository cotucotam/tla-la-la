#include <stdio.h>
#include <stdlib.h>
#include <mpsse.h>

#define SDA     7
#define SCL     6

int main(void)
{
	struct mpsse_context *io = NULL;
	int i = 0, retval = EXIT_FAILURE;

    io = Open(0x0403, 0x6010, BITBANG, 0, 0, IFACE_A, NULL, NULL);

	if(io && io->open)
	{
        while(1)
		{
			PinHigh(io, SDA);
			printf("Pin SDA is: %d\n", PinState(io, SDA, -1));
			PinHigh(io, SCL);
			printf("Pin SCL is: %d\n", PinState(io, SCL, -1));
            usleep(1000);
			
			PinLow(io, SDA);
			printf("Pin SDA is: %d\n", PinState(io, SCL, -1));
			PinLow(io, SCL);
			printf("Pin SCL is: %d\n", PinState(io, SCL, -1));
            usleep(1000);
		}

		retval = EXIT_SUCCESS;
	}
	else
	{
		printf("Failed to open MPSSE: %s\n", ErrorString(io));
	}

	Close(io);

	return retval;
}
