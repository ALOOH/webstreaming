#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "gpio.h"

#define GPIO_AD_POWER 	     78      // GPIO_C14
#define GPIO_AD1_ENABLE      63      // GPIO_B31
#define GPIO_AD2_ENABLE      64      // GPIO_C1
#define GPIO_AD3_ENABLE      65      // GPIO_C2
#define GPIO_AD4_ENABLE      66      // GPIO_C3

#define GPIO_SENSOR_LED      6 
#if 1
#define DBG(...) //fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...)
#endif

void AD_OnoffLed(int value)
{
       gpio_export(GPIO_SENSOR_LED);
       gpio_set_output(GPIO_SENSOR_LED, value);
       gpio_unexport(GPIO_SENSOR_LED);
}
	
int AD_EnableModule(int midx)
{
#if 1 
	unsigned int gpio; 
		
	switch(midx)
	{
		case 1: 
		gpio = GPIO_AD1_ENABLE;
		break; 
		
		case 2:
		gpio = GPIO_AD2_ENABLE; 
		break; 
		
		case 3: 
		gpio = GPIO_AD3_ENABLE; 
		break; 
		
		case 4:
		gpio = GPIO_AD4_ENABLE; 
		break; 
		
		default: 
			return -1; 	
		break; 
	}
	
	gpio_export(gpio); 
	gpio_set_output(gpio, 1); 
	gpio_unexport(gpio); 

	//DBG("Enable_ADModule : %d\n", midx); 	
#endif 
	return 0; 	
}
		
int AD_DisableModule(int midx)
{
#if 1 
	unsigned int gpio; 
	
	switch(midx)
	{
		case 1:
		gpio = GPIO_AD1_ENABLE; 
		break; 
		
		case 2:
		gpio = GPIO_AD2_ENABLE;
		break; 
		
		case 3:
		gpio = GPIO_AD3_ENABLE; 
		break; 
		
		case 4:
		gpio = GPIO_AD4_ENABLE; 
		break; 
		
		default:
		return -1; 
		break; 
	}
	
	gpio_export(gpio); 
	gpio_set_output(gpio, 0); 
	gpio_unexport(gpio); 

//	printf("AD_DisableModule : %d\n", midx); 	
#endif 
	return 0; 
}
	
int AD_PowerCheck(int verbose)
{
	unsigned int val; 
	
	gpio_export(GPIO_AD_POWER);
//        gpio_set_output(GPIO_AD_POWER);
        gpio_get_val(GPIO_AD_POWER, &val);
        gpio_unexport(GPIO_AD_POWER);
        
	if ( verbose )
        	printf("power : %d\n", val); 
        
        return val; 
}

int AD_PowerOn()
{
//	gpio_export(GPIO_AD_POWER);
        gpio_set_output(GPIO_AD_POWER, 1);
//      gpio_unexport(GPIO_AD_POWER);
}

int AD_PowerOff() 
{
//	gpio_export(GPIO_AD_POWER); 
	gpio_set_output(GPIO_AD_POWER, 0); 
//	gpio_unexport(GPIO_AD_POWER); 
}
	
