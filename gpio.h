#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
 
#define GPIO_OUTPUT 0
#define GPIO_INPUT 1
#define GPIO_HIGH  1
#define GPIO_LOW  0
 
#define SYSFS_GPIO_DIR "/sys/class/gpio"
 
#define MAX_BUF 64
 
int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_get_dir(unsigned int gpio, unsigned int *dir);
int gpio_set_dir(unsigned int gpio, unsigned int dir, unsigned int val);
int gpio_get_val(unsigned int gpio, unsigned int *val);
int gpio_set_val(unsigned int gpio, unsigned int val);
int gpio_close(int fd);
int gpio_set_input(unsigned int gpio);
int gpio_set_output(unsigned int gpio, unsigned int val);
