#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <linux/uinput.h>

/* emit function is identical to of the first example */

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

int main(void)
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

//  printf("ioctl = %d, %d, %d ,%d , %d, %d\n", i1,i2,i3,i4,i5,i6);

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

   /* Key press, report the event, send key release, and report again */
for(;;)
{
   emit(fd, EV_KEY, KEY_D, 1);
   emit(fd, EV_SYN, SYN_REPORT, 1);
   sleep(1);
   emit(fd, EV_KEY, KEY_D, 0);
   emit(fd, EV_SYN, SYN_REPORT, 0);

   emit(fd, EV_KEY, KEY_U, 1);
   emit(fd, EV_SYN, SYN_REPORT, 0);
   emit(fd, EV_KEY, KEY_U, 0);
   emit(fd, EV_SYN, SYN_REPORT, 0);

   emit(fd, EV_KEY, KEY_P, 1);
   emit(fd, EV_SYN, SYN_REPORT, 0);
   emit(fd, EV_KEY, KEY_P, 0);
   emit(fd, EV_SYN, SYN_REPORT, 0);

   emit(fd, EV_KEY, KEY_A, 1);
   emit(fd, EV_SYN, SYN_REPORT, 0);
   emit(fd, EV_KEY, KEY_A, 0);
   emit(fd, EV_SYN, SYN_REPORT, 0);

   sleep(5);
}
   ioctl(fd, UI_DEV_DESTROY);

   close(fd);
   return 0;
}
