#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>

int read_mouse()
{
    int fd, bytes;
    unsigned char data[3];

    const char *pDevice = "/dev/input/mice";

    // Open Mouse
    fd = open(pDevice, O_RDWR);
    if(fd == -1)
    {
        printf("ERROR Opening %s\n", pDevice);
        return -1;
    }

    int left, middle, right;
    signed char x, y;
    while(1)
    {
        // Read Mouse     
        bytes = read(fd, data, sizeof(data));

        if(bytes > 0)
        {
            left = data[0] & 0x1;
            right = data[0] & 0x2;
            middle = data[0] & 0x4;

            x = data[1];
            y = data[2];
            printf("x=%d, y=%d, left=%d, middle=%d, right=%d\n", x, y, left, middle, right);
        }   
    }
    return 0; 
}

static const char *const evval[3] = {
	"RELEASED",
	"PRESSED ",
	"REPEATED"
};

int read_keyboard()
{
	//const char *dev = "/dev/input/by-id/usb-04d9_USB_Keyboard-event-if01";
	//const char *dev = "/dev/input/by-id/usb-04d9_USB_Keyboard-if01-event-kbd";
	const char *dev = "/dev/input/by-path/pci-0000:00:14.0-usb-0:7:1.0-event-kbd";
	struct input_event ev;
	ssize_t n;
	int fd;
	fd = open(dev, O_RDONLY);
	
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		return EXIT_FAILURE;
	}

	while (1) {
		n = read(fd, &ev, sizeof ev);
		//if (n == (ssize_t)-1) {
		//	if (errno == EINTR)
		//		continue;
		//	else
		//		break;
		//} else if (n != sizeof ev) {
		//	errno = EIO;
		//	break;
		//}

		if (ev.type == EV_KEY)// && ev.value >= 0 && ev.value <= 2)
			printf("%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
	}

	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	return EXIT_FAILURE;
}

int main(int argc, char** argv)
{
	//read_mouse();
	read_keyboard();
	return 0;
}
