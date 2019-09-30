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

const int PORT = 8080;
const int MAX_BUFF = 40;
const int MAX_LINE = 40;

char key_buff[MAX_BUFF][MAX_LINE];
int key_len = -1;
char mouse_buff[MAX_BUFF][MAX_LINE];
int mouse_len = -1;

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

		if (ev.type == EV_KEY)// && ev.value >= 0 && ev.value <= 2)
		{
			char data[50];
			sprintf(data, "%s 0x%04x (%d)\n", evval[ev.value], (int)ev.code, (int)ev.code);
			key_len++;
			//key_buff[key_len] = data;
			strcpy(key_buff[key_len], data);
			printf(WHITE "Key event: " RESET "%s Len: %i\n", *(key_buff + key_len), key_len);
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
	while(1) 
	{
		// read the message from client and copy it in buffer 
		read(sockfd, buff, sizeof(buff)); 
		
		// print buffer
		printf(WHITE "From client: " RESET "%s\t", buff);
		if (strcmp(buff, "get_key\n") == 0)
		{
			printf("Recieved get_key command!!");	
		}
		
		printf("To client: \n");
		sleep(2);
		for (int i = 0; i < key_len; i++)
                {
                	printf("	%s", *(key_buff + i));
                }

		// and send that buffer to client 
		write(sockfd, key_buff, sizeof(buff));//key_len); 
		
		//Reset buffer
		key_len = 0;

		// if msg contains "Exit" then server exit and chat ended. 
		if (strncmp("exit", buff, 4) == 0) 
		{ 
			printf("Server Exit...\n"); 
			break; 
		} 
	} 
} 

// Driver function 
int start_server() 
{ 
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
	servaddr.sin_port = htons(PORT); 

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
	//printf("emit write bytes=%d fd=%d code=%d val=%d\n",res, fd, code, val);
}

int create_keyboard(int *key_fd)
{
	struct uinput_user_dev uud;
	int version, rc, fd;

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	printf("fd=%d\n",fd);

	rc = ioctl(fd, UI_GET_VERSION, &version);
	printf("rd=%d\n",rc); 

	if (rc == 0 && version >= 5) 
	{
		printf("Error! version=%d\n",version);
		//return 0;
	}

	/*
	* The ioctls below will enable the device that is about to be
	* created, to pass key events, in this case the space key.
	*/
	int i1 = ioctl(fd, UI_SET_EVBIT, EV_KEY);
	int i2 = ioctl(fd, UI_SET_EVBIT, EV_SYN);
	int i3 = ioctl(fd, UI_SET_KEYBIT, KEY_D);
	int i4 = ioctl(fd, UI_SET_KEYBIT, KEY_U);
	int i5 = ioctl(fd, UI_SET_KEYBIT, KEY_P);
	int i6 = ioctl(fd, UI_SET_KEYBIT, KEY_A);

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
        char buff[MAX_BUFF * MAX_LINE];
        int n = 0;
        while(1)
        {
                printf("Enter the string : "); 
                while ((buff[n++] = getchar()) != '\n') 
                        ; 
                write(sockfd, buff, sizeof(buff)); 
                //bzero(buff, sizeof(buff));

                read(sockfd, key_buff, sizeof(buff));
                printf("Buffer size is %lu bytes\n", sizeof(buff)); 
                printf(WHITE "From Server: " RESET "\n");
                for (int i = 0; i<MAX_BUFF; i++)
                {
                        printf("        %s", *(key_buff + i));
                	emit(fd, EV_KEY, KEY_U, 1);
			emit(fd, EV_SYN, SYN_REPORT, 0);
			//emit(fd, EV_KEY, KEY_U, 0);
			//emit(fd, EV_SYN, SYN_REPORT, 0);
		}		

                if ((strncmp(buff, "exit", 4)) == 0) 
                { 
                        printf("Client Exit...\n"); 
                        break; 
                } 
        } 
}

int start_client() 
{ 
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
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servaddr.sin_port = htons(PORT);    
   
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
        
	ioctl(fd, UI_DEV_DESTROY);                                                           
        close(fd);

	close(sockfd);
        return 0;
}

int main(int argc, char** argv)
{
	const int CLIENT = 0;
	const int SERVER = 1;
	int mode = CLIENT;
	char *cvalue = NULL;
	int index;
	int c;

	opterr = 0;

	while ((c = getopt (argc, argv, "cs")) != -1)
		switch (c)
		{
		case 'c':
			mode = CLIENT;
			break;
		case 's':
			mode = SERVER;
			break;
		//case 'c':
		//	cvalue = optarg;
		//	break;
		case '?':
			if (optopt == 'c')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr,
					"Unknown option character `\\x%x'.\n",
					optopt);
			return 1;
		default:
			abort();
		}

	//printf ("aflag = %d, bflag = %d, cvalue = %s\n",
	//	aflag, bflag, cvalue);

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);
	switch (mode)
	{
	case CLIENT:
		start_client();
		break;
	case SERVER:
		start_server();
		break;
	default:
		abort();
	}
	return 0;
}
