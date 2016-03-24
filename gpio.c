#include "gpio.h"
 
int gpio_export(unsigned int gpio)
{
     int fd, len;
     char buf[MAX_BUF];
 
     fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't export GPIO %d pin: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     len = snprintf(buf, sizeof(buf), "%d", gpio);
     write(fd, buf, len);
     close(fd);
 
     return 0;
}
 
int gpio_unexport(unsigned int gpio)
{
     int fd, len;
     char buf[MAX_BUF];
 
     fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't unexport GPIO %d pin: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     len = snprintf(buf, sizeof(buf), "%d", gpio);
     write(fd, buf, len);
     close(fd);
 
     return 0;
}
 
int gpio_get_dir(unsigned int gpio, unsigned int *dir)
{
     int fd, len;
     char buf[MAX_BUF];
 
     len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);
 
     fd = open(buf, O_RDONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't get GPIO %d pin direction: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     read(fd, &buf, MAX_BUF);
     close(fd);
 
     if (strncmp(buf, "in", 2) == 0)
         *dir = GPIO_INPUT;
     else
         *dir = GPIO_OUTPUT;
 
     return 0;
}
 
int gpio_set_dir(unsigned int gpio, unsigned int dir, unsigned int val)
{
     int fd, len;
     char buf[MAX_BUF];
 
     len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
     fd = open(buf, O_WRONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't set GPIO %d pin direction: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     if (dir == GPIO_OUTPUT) {
         if (val == GPIO_HIGH)
             write(fd, "high", 5);
         else
             write(fd, "out", 4);
     } else {
         write(fd, "in", 3);
     }
 
     close(fd);
 
     return 0;
}
 
int gpio_get_val(unsigned int gpio, unsigned int *val)
{
     int fd, len;
     char buf[MAX_BUF];
 
     len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
     fd = open(buf, O_RDONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't get GPIO %d pin value: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     read(fd, buf, 1);
     close(fd);
 
     if (*buf != '0')
         *val = GPIO_HIGH;
     else
         *val = GPIO_LOW;
 
     return 0;
}
 
int gpio_set_val(unsigned int gpio, unsigned int val)
{
     int fd, len;
     char buf[MAX_BUF];
 
     len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
     fd = open(buf, O_WRONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't set GPIO %d pin value: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     if (val == GPIO_HIGH)
         write(fd, "1", 2);
     else
         write(fd, "0", 2);
 
     close(fd);
 
     return 0;
}

#if 0  
int gpio_set_edge(unsigned int gpio, char *edge)
{
     int fd, len;
     char buf[MAX_BUF];
 
     len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
 
     fd = open(buf, O_WRONLY);
 
     if (fd < 0) {
         fprintf(stderr, "Can't set GPIO %d pin edge: %s\n", gpio, strerror(errno));
         return fd;
     }
 
     write(fd, edge, strlen(edge)+1);
     close(fd);
 
     return 0;
}
int gpio_open(unsigned int gpio)
{
     int fd, len;
     char buf[MAX_BUF];
 
     len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
     fd = open(buf, O_RDONLY|O_NONBLOCK);
 
     if (fd < 0)
         fprintf(stderr, "Can't open GPIO %d pin: %s\n", gpio, strerror(errno));
 
     return fd;
}
#endif  
int gpio_close(int fd)
{
     return close(fd);
}
#if 0  
int gpio_read(int fd, char ch, unsigned int *val)
{
     int ret;
//     char ch; 
 
     lseek(fd, 0, SEEK_SET);
 
     ret = read(fd, &ch, 1);
 
     if (ret != 1) {
         fprintf(stderr, "Can't read GPIO %d pin: %s\n", gpio, strerror(errno));
         return ret;
     }
 
     if (ch != '0')
         *val = GPIO_HIGH;
     else
         *val = GPIO_LOW;
 
     return 0;
}
#endif 

int gpio_set_input(unsigned int gpio)
{
 gpio_set_dir(gpio, GPIO_INPUT, 0); 
 return 0; 
}

int gpio_set_output(unsigned int gpio, unsigned int val)
{
 gpio_set_dir(gpio, GPIO_OUTPUT, val);  
 return 0; 
}

#if 0
int main(int argc, char *argv[])
{
     unsigned int gpio;
     unsigned int val;
     char *end_ptr;
 
     if (argc < 3)
         exit(1);
 
     if (strcmp(argv[1], "r") == 0) {
         gpio = strtoul(argv[2], &end_ptr, 0);
         gpio_export(gpio);
         gpio_set_input(gpio); // gpio_set_dir(gpio);
         gpio_get_val(gpio, &val);
         gpio_unexport(gpio);
         fprintf(stderr, "gpio%d = %d\n", gpio, val);
     } else if (strcmp(argv[1], "w") == 0) {
         if (argc < 4)
             exit(1);
 
         gpio = strtoul(argv[2], &end_ptr, 0);
         val = strtoul(argv[3], &end_ptr, 0);
         gpio_export(gpio);
         gpio_set_output(gpio, val);
         gpio_unexport(gpio);
     } 
     /* else if (strcmp(argv[1], "i") == 0) {
         int fd;
         struct pollfd fdset[2];
         int ret;
 
         if (argc < 4)
             exit(1);
 
         gpio = strtoul(argv[2], &end_ptr, 0);
         gpio_export(gpio);
         gpio_set_input(gpio);
         gpiof_set_edge(gpio, argv[3]); // "none", "falling", "rising", "both"
         fd = gpio_open(gpio);
         gpio_read(fd, gpio, &val);
 
         while (1) {
             memset(fdset, 0, sizeof(fdset));
             fdset[0].fd = STDIN_FILENO;
             fdset[0].events = POLLIN;
             fdset[1].fd = fd;
             fdset[1].events = POLLPRI;
             ret = poll(fdset, 2, 3*1000);
 
             if (ret < 0) {
                 perror("poll");
                 break;
             }
 
             fprintf(stderr, ".");
 
             if (fdset[1].revents & POLLPRI) {
                 fprintf(stderr, "\nGPIO %d interrupt occurred!\n", gpio);
                 gpio_read(fdset[1].fd, &val);
             }
 
             if (fdset[0].revents & POLLIN)
                 break;
 
             fflush(stdout);
         }
 
         gpio_close(fd);
         gpio_unexport(gpio);
     } */ else {
         exit(1);
     }
 
     return 0;
}
#endif 
     
