#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SA struct sockaddr

#define BLACK   "\x1b[30m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[36m"
#define RESET   "\x1b[0m"

char IP[10] = "127.0.0.1";
#define PORT 8080

const size_t MAX_BUFF = 40;
const int MAX_LINE = 40;

struct input_event key_buff[MAX_BUFF];
int key_len = -1;
char mouse_buff[MAX_BUFF][MAX_LINE];
int mouse_len = -1;

enum level {
	ERROR,
	WARN,
	INFO,
	DEBUG,
	TRACE
};

int log(char *message, int level)

int read_mouse()
{
	int fd, bytes;
	unsigned char data[3];

	const char *pDevice = "/dev/input/mice";

	// Open Mouse
	fd = open(pDevice, O_RDWR);
	if(fd == -1) {
		printf("ERROR Opening %s\n", pDevice);
		return -1;
	}

	int left, middle, right;
	signed char x, y;

	while(1) {
		// Read Mouse
		bytes = read(fd, data, sizeof(data));

		if(bytes > 0) {
			left = data[0] & 0x1;
			right = data[0] & 0x2;
			middle = data[0] & 0x4;

			x = data[1];
			y = data[2];

			char data[50];
			sprintf(data, "x=%d, y=%d, left=%d, middle=%d, right=%d\n", x, y, left, middle, right);
			//mouse_len++;
			//mouse_buff[mouse_len] = data;
		}
	}
	return 0;
}

static const char *const evval[3] = {
	"RELEASED",
	"PRESSED ",
	"REPEATED"
};

void *read_keyboard(void *dev)
{
	//const char *dev = "/dev/input/by-id/usb-04d9_USB_Keyboard-event-if01";
	//const char *dev = "/dev/input/by-id/usb-04d9_USB_Keyboard-if01-event-kbd";
	//const char *dev = "/dev/input/by-path/pci-0000:00:14.0-usb-0:7:1.0-event-kbd";
	struct input_event ev;
	ssize_t n;
	int fd;
	fd = open(dev, O_RDONLY);

	if (fd == -1) {
		fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
		pthread_exit(NULL);
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

		if (ev.type == EV_KEY)// && ev.value >= 0 && ev.value <= 2) {
			//char data[50];
			printf("%s 0x%04x (%d) - Len%i\n", evval[ev.value], (int)ev.code, (int)ev.code, key_len);
			key_len++;
			key_buff[key_len] = ev;
			//strcpy(key_buff[key_len], data);
			//printf(WHITE "Key event: " RESET "%s Len: %i\n", *(key_buff + key_len), key_len);
		}
	}

	fflush(stdout);
	fprintf(stderr, "%s.\n", strerror(errno));
	pthread_exit(NULL);
}

// Function designed for chat between client and server.
void server_func(int sockfd)
{
	char buff[MAX_BUFF * MAX_LINE];
	while(1) {
		// read the message from client and copy it in buffer
		read(sockfd, buff, sizeof(buff));

		// print buffer
		printf(WHITE "From client: " RESET "%s\t", buff);
		if (strcmp(buff, "get_key") == 0) {
			printf("Recieved get_key command!!");
		}

		printf("To client: \n");
		sleep(2);
		for (int i = 0; i < key_len; i++) {
			printf("	%s 0x%04x (%d)\n", evval[key_buff[i].value], (int)key_buff[i].code, (int)key_buff[i].code);
		}

		// and send that buffer to client
		write(sockfd, key_buff, sizeof(buff));//key_len);

		//Reset buffer
		key_len = 0;

		// if msg contains "Exit" then server exit and chat ended.
		if (strncmp("exit", buff, 4) == 0) {
			printf("Server Exit...\n");
			break;
		}
	}
}

// Driver function
int start_server(char *ip, int port)
{
	printf("Listening to %s on %i...\n", ip, port);
	int sockfd, connfd;
	unsigned int len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	len = sizeof(cli);

	// Accept the data packet from client and verification
	connfd = accept(sockfd, (SA*)&cli, &len);
	if (connfd < 0) {
		printf("server acccept failed...\n");
		exit(0);
	}
	else
		printf("server acccepted the client...\n");

	pthread_t key_thread;
	printf("Starting keyboard capturing thread...\n");
	pthread_create(&key_thread, NULL, read_keyboard, "/dev/input/by-path/pci-0000:00:14.0-usb-0:7:1.0-event-kbd");
	//read_mouse;

	// Function for chatting between client and server
	server_func(connfd);

	// Close threads
	pthread_join(key_thread, NULL);

	// After chatting close the socket
	close(sockfd);
	return 0;
}

void emit(int fd, int type, int code, int val)
{
	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	/* timestamp values below are ignored */
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	int res = write(fd, &ie, sizeof(ie));
	printf("emit write bytes=%d fd=%d code=%d val=%d\n",res, fd, code, val);
}

int create_keyboard(int *key_fd)
{
	struct uinput_user_dev uud;
	int version, rc, fd;

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	printf("fd=%d\n",fd);

	rc = ioctl(fd, UI_GET_VERSION, &version);
	printf("rd=%d\n",rc);

	if (rc == 0 && version >= 5) {
		printf("Error! version=%d\n",version);
		//return 0;
	}

	/*
	 * The ioctls below will enable the device that is about to be
	 * created, to pass key events.
	 */
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	for(int i = 0; i < 632; i++) {
		ioctl(fd, UI_SET_KEYBIT, i);
	}

	//printf("ioctl = %d, %d, %d ,%d , %d, %d\n", i1,i2,i3,i4,i5,i6);

	memset(&uud, 0, sizeof(uud));
	snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "uinput-keyboard");
	uud.id.bustype = BUS_HOST;
	uud.id.vendor  = 0x1;
	uud.id.product = 0x2;
	uud.id.version = 1;

	write(fd, &uud, sizeof(uud));
	sleep(2);

	int i = ioctl(fd, UI_DEV_CREATE);
	printf("dev create =%d\n", i);
	sleep(2);

	*key_fd = fd;
	//printf("key_fd: %i\n", *key_fd);
	//printf("fd: %i\n", fd);
	return 0;
}

void client_func(int sockfd, int key_fd)
{
	//char buff[MAX_BUFF * MAX_LINE];
	char *buff;
	//int n = 0;

	while(1) {
		sleep(10);
		//printf("Enter the string : ");
		//n=0;
		//while ((buff[n++] = getchar()) != '\n');
		printf("Sending '%s' to server...\n", "get_key");
		buff = "get_key";
		write(sockfd, buff, sizeof(buff));
		//bzero(buff, sizeof(buff));

		read(sockfd, key_buff, sizeof(key_buff));
		printf("Buffer size is %lu bytes\n", sizeof(buff));
		printf(WHITE "From Server: " RESET "\n");
		for (int i = 0; i<MAX_BUFF; i++) {
			if (!key_buff[i].code) {
				//printf("Blank code!\n");
				break;
			}

			printf("        %s 0x%04x (%d)\n", evval[key_buff[i].value], (int)key_buff[i].code, (int)key_buff[i].code);
			emit(key_fd, EV_KEY, key_buff[i].code, key_buff[i].value);
			emit(key_fd, EV_SYN, SYN_REPORT, 0);

			//emit(key_fd, EV_KEY, KEY_U, 1);
			//emit(key_fd, EV_SYN, SYN_REPORT, 0);
			//sleep(1);
			//emit(key_fd, EV_KEY, KEY_U, 0);
			//emit(key_fd, EV_SYN, SYN_REPORT, 0);

			//emit(fd, EV_KEY, KEY_U, 0);
			//emit(fd, EV_SYN, SYN_REPORT, 0);
		}

		//bzero(buff, sizeof(buff));

		if ((strncmp(buff, "exit", 4)) == 0) {
			printf("Client Exit...\n");
			break;
		}
	}
}

int start_client(char *ip, int port)
{
	printf("Client searching for server at %s on %i...\n", ip, port);
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli;

	// socket create and varification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip);
	servaddr.sin_port = htons(port);

	// connect the client socket to server socket
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
		printf("connection with the server failed...\n");
		exit(0);
	}
	else
		printf("connected to the server..\n");

	printf("Creating virtual keyboard...\n");
	int key_fd;
	create_keyboard(&key_fd);

	client_func(sockfd, key_fd);

	ioctl(key_fd, UI_DEV_DESTROY);
	close(key_fd);

	close(sockfd);
	return 0;
}

int version()
{
	printf("wl-control v0.0.1 by JosephTheEngineer\n");
	return 0;
}

int help()
{
	printf(WHITE "Usage: " GREEN "wl-control" RESET " <command> [<args>]\n");
        printf("\n");
        printf(WHITE "client  - the device being controlled"
        	"				default" RESET "\n");
        printf(BLUE "	-i" RESET " --ip		the ip that the client connects to"
        	GREEN "	127.0.0.1" RESET "\n");
        printf(BLUE "	-p" RESET " --port	the port that the client connects to"
        	GREEN "	8080" RESET "\n");
        printf(BLUE "	-v" RESET " --visual	stream the servers desktop"
        	GREEN "		false" RESET "\n");
        printf("\n");
        printf(WHITE "server  - the device controlling the clients"
        	"			default" RESET "\n");
        printf(BLUE "	-i" RESET " --ip		the ip that the sever listens for"
        	GREEN "	all" RESET "\n");
        printf(BLUE "	-p" RESET " --port	the port that the sever listens on"
        	GREEN "	8080 " RESET "\n");
        printf(BLUE "	-v" RESET " -visual	enable remote desktop mode"
        	GREEN "		false" RESET "\n");
        printf("\n");
        printf(WHITE "version - show the current version" RESET "\n");
        printf("\n");
        printf(WHITE "help    - show this help list" RESET "\n");
        printf("\n");
        printf("Report bugs to joseph@theengineer.life.\n");
	return 0;
}

int main(int argc, char** argv)
{
	const int CLIENT = 0;
	const int SERVER = 1;
	int mode = CLIENT;
	char *server_ip = IP;
	char *client_ip = NULL;
	int port = PORT;
	int index;
	int c;

	opterr = 0;

	if (strcmp(argv[1], "client") == 0) {
		mode = CLIENT;
	}
	else if (strcmp(argv[1], "server") == 0) {
		mode = SERVER;
	}
	else if (strcmp(argv[1], "version") == 0) {
		version();
		return 0;
	}
	else if (strcmp(argv[1], "help") == 0) {
		help();
		return 0;
	}
	else {
		printf(RED "Invalid command %s." RESET "\n", argv[1]);
		abort();
	}

	// remove sub command to prepare for getopt
	for(int i = 0; i < argc-1; i++) {
		argv[i]=argv[i+1];
	}
	argc--;

	while ((c = getopt (argc, argv, "i:p:v")) != -1)
		switch (c) {
			case 'i': // ip
				switch (mode) {
					case CLIENT:
						server_ip = optarg;
					case SERVER:
						client_ip = optarg;
				}
				break;
			case 'p': // port
				port = (int)optarg;
				break;
			case 'v': // visual
				printf("Visual mode is not implemented yet!\n");
				break;
			case '?':
				if (optopt == 'i' || optopt == 'p') {
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				} else if (isprint (optopt)) {
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				} else {
					fprintf (stderr,
							"Unknown option character `\\x%x'.\n",
							optopt);
				}
				return 1;
			default:
				abort();
		}

	//printf ("aflag = %d, bflag = %d, cvalue = %s\n",
	//	aflag, bflag, cvalue);

	for (index = optind; index < argc; index++) {
		printf ("Non-option argument %s\n", argv[index]);
	}
	switch (mode) {
		case CLIENT:
			printf("Client searching for server at %s on %i...\n", server_ip, port);
			start_client(server_ip, port);
			break;
		case SERVER:
			printf("Listening to %s on %i...\n", client_ip, port);
			start_server(client_ip, port);
			break;
		default:
			abort();
	}
	return 0;
}
