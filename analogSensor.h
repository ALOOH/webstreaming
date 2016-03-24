#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "gpio.h"

#define LED_ERROR_STATUE 6 	// GPIO_A6

#define AD_POWER_EN     78      // GPIO_C14
#define AD1_ENABLE      63      // GPIO_B31
#define AD2_ENABLE      64      // GPIO_C1
#define AD3_ENABLE      65      // GPIO_C2
#define AD4_ENABLE      66      // GPIO_C3

#define MODULE_AD1 	1	
#define MODULE_AD2	2
#define MODULE_AD3	3
#define MODULE_AD4	4

int AD_PowerCheck();
int AD_PowerOn();
int AD_PowerOff(); 

int AD_EnableModule(int midx);
int AD_DisableModule(int midx); 
void AD_OnoffLed(int value);

