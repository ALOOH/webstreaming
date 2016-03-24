#include <stdio.h>         // printf()
#include <string.h>        // strlen()
#include <fcntl.h>         // O_WRONLY
#include <unistd.h>        // write(), close()
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <sys/socket.h>

#include "analogSensor.h"
#include "httpd.h"
static unsigned int midx; 
#if 1
#define DBG(...)  fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#define DBG_A(...) fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__, __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...)
#define DBG_A(...)
#endif

//jonsama
//#define DEBUG
//#define SAMPLECNT_DEBUG

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define setClock() (clock() + 500000)

clock_t clk_start;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.1";
static const char *filename = "data.dat";
static uint32_t mode;
static uint8_t bits;
static uint32_t speed = 500000;
static uint16_t delay = 0;
static int verbose;
static int8_t runf; 
static uint8_t printOption; 
static int sampleNumber = 0; 
static int gTimeout = 3600*24; 
static int rnumber = -1; 
static uint32_t rdata; 
static int flag = 1;  // all 

/* time */
#define MILLION 1000000L

/* regster */ 

#define REGISTER_ADMODE_ID 99 

#define MODE_TIME 1
#define MODE_FFT 2 
#define MODE_FIR 3 
#define MODE_RMS 4 

int gADMode = MODE_TIME; 

#define NB_REGISTER 4
static uint32_t REGDATA_inner[NB_REGISTER] = {MODE_TIME, };

int deviceflag = 0; 	

#define  FLAG_AD1 1
#define  FLAG_AD2 2
#define  FLAG_AD3 4
#define  FLAG_AD4 8

/* processing */

#define PROCESS_RECODING        1
#define PROCESS_SETREG          2
#define PROCESS_READREG         3
#define PROCESS_MODESET 		4
#define PROCESS_AUTOSAVE 		5 
#define PROCESS_STREAM			6
/* For SPI */
#define DATA_HEADER_SIZE 4
#define BUFFER_SIZE (DATA_HEADER_SIZE + 2048) 
#define MAX_SAMPLE 32 
//#define BUFFER_SIZE (DATA_HEADER_SIZE + 1024) 
//#define MAX_SAMPLE 32 

#define CMD_TEST            0x10101010
#define CMD_DATA            0x29380000

#define CMD_SET_MODE 	    0x68390284 
#define CMD_GET_MODE 	    0x68380284 

#define CMD_STATUS          0x68390384
#define CMD_END             0x68390484

#define CMD_READ_REG	    0x30380000
#define CMD_READ			0x30380100

#define CMD_SET_REG			0x30390000
#define CMD_SET_REGA        0x30390100

#define CMD_CHANGE_TRMODE   0x40400000
#define CMD_REQ_CNT 	    0x28380000


#define RC_SYN              0x55AA55AA
#define RC_RDY              0x12345678

struct request {
  uint32_t cmd; 
  uint32_t data[NB_REGISTER]; 
};

static struct request reg; 

uint8_t default_rx[BUFFER_SIZE] = { 0x00, }; 
uint32_t send_rx[2];  

uint32_t CMD_READREGDATA[NB_REGISTER] = {0x00, }; 
uint8_t CMD_READDATA_tx[BUFFER_SIZE] = {0x84, 0x03, 0x39, 0x68}; 

uint32_t CMD_SETREG_rx[2]; 
uint32_t CMD_REQ_CNT_tx[2] = {CMD_REQ_CNT, };
uint32_t CMD_TEST_tx[2] = { CMD_TEST, };
uint32_t CMD_DATA_tx[2] = { CMD_DATA, }; 
uint32_t CMD_STATUS_tx[2] = { CMD_STATUS, }; 
uint32_t CMD_END_tx[2] = { CMD_END, }; 
uint32_t CMD_READREG_tx[2] = { CMD_READ_REG, }; 
uint32_t CMD_READREGA_tx[2] = { CMD_READ, }; 
uint32_t CMD_SETREG_tx[2] = { CMD_SET_REG, };
uint32_t CMD_SETREGA_tx[2] = { CMD_SET_REGA, }; 

char *input_tx;
int stream_state = -1;

unsigned char value;
char p[4];
FILE *fpx, *fp1, *fp2, *fp3, *fp4;
int fdx, fd1, fd2, fd3, fd4; 

static void hex_dump_p(const void *src, size_t length, size_t line_size, char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;
	printf("%s | ", prefix);
	
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

static void hex_dump(FILE* f_p, const void *src, size_t length, size_t line_size, char *prefix, uint8_t flag)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

  switch (flag) 
  {
    case 0 :  // binary
//    DBG("hex_dump write \n");
     // write(f_p, src, length);
       fwrite(src, length, 1, f_p);
    break; 
    #if 0
    case 2 :  // float
    {
      uint32_t data32; 
      
      while (length > 0)
      {
        data32 = *address;
        data32 = data32 | *(address+1) << 8; 
        data32 = data32 | *(address+2) << 16; 
        data32 = data32 | *(address+3) << 24; 
        address = address +4; 
        
        //fwrite((float)data32, sizeof(data32), 4, f_p);       
        fprintf(f_p, "%x : %f\n", data32, (float)data32); 
        length = length - 4; 
        
      }
    }
    
    break; 
    
    case 1 :    // hex 
    default:
    {  
    	fprintf(f_p, "%s | ", prefix);
    	
    	while (length-- > 0) {
    		fprintf(f_p, "%02X ", *address++);
    		if (!(++i % line_size) || (length == 0 && i % line_size)) {
    			if (length == 0) {
    				while (i++ % line_size)
    					fprintf(f_p, "__ ");
    			}
    		#if 0	
    			fprintf(f_p, " | ");  /* right close */
    			while (line < address) {
    				c = *line++;
    				fprintf(f_p, "%c", (c < 33 || c == 255) ? 0x2E : c);
    			}
        #endif 
    			fprintf(f_p, "\n");
    			if (length > 0)
    				fprintf(f_p, "%s | ", prefix);
    		}
    	}
    }
    break; 
	#endif
  }
}

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
static int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			sscanf(src + 2, "%2x", &ch);
			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

struct spi_ioc_transfer tr_cmd[2]; 
struct spi_ioc_transfer tr_data; 

static int transfer(int fd, FILE* fp, uint8_t const *tx, uint8_t const *rx, size_t len, uint8_t flag)
{
	int ret;
	
	struct spi_ioc_transfer tr_data = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len, 
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	
	if (mode & SPI_TX_QUAD)
		tr_data.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr_data.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr_data.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr_data.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr_data.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr_data.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr_data);
	
	if (ret < 1)
		pabort("can't send spi message");

//  if (verbose)
//		  hex_dump(tx, len, len, "TX", flag);

#if 1 

	if ( rx[0] == 0x00 && rx[1] == 0x00 && rx[2] == 0x38 & rx[3] == 0x29)
  	{
  		hex_dump(fp, &rx[4], (len-DATA_HEADER_SIZE), (len-DATA_HEADER_SIZE), "RX", flag);
  	}
  	else 
  	{
  		DBG(" spi data error\n"); 

#ifdef DEBUG
  		printf(" %d spi data error\n", fd); 
  		printf("%x:%x:%x:%x\n", rx[0], rx[1], rx[2], rx[3]); 
#endif 
  		return -1; 
  	}
#else  		
  hex_dump(rx, len, len, "RX", flag);
#endif 
	return 0; 
}

static int transferNet(int fd, int fp, uint8_t const *tx, uint8_t const *rx, size_t len, uint8_t flag)
{
	int ret;
	
	struct spi_ioc_transfer tr_data = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len, 
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	
	if (mode & SPI_TX_QUAD)
		tr_data.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr_data.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr_data.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr_data.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr_data.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr_data.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr_data);
	
	if (ret < 1)
		pabort("can't send spi message");

//  if (verbose)
//		  hex_dump(tx, len, len, "TX", flag);

#if 1 
	//DBG("data\n"); 

	if ( rx[0] == 0x00 && rx[1] == 0x00 && rx[2] == 0x38 & rx[3] == 0x29)
  	{
  		hex_dump(fp, &rx[4], (len-DATA_HEADER_SIZE), (len-DATA_HEADER_SIZE), "RX", flag);
  	}
  	else 
  	{

#ifdef DEBUG
  		printf(" %d spi data error\n", fd); 
  		printf("%x:%x:%x:%x\n", rx[0], rx[1], rx[2], rx[3]); 
#endif 
  		return -1; 
  	}
#else  		
  hex_dump(rx, len, len, "RX", flag);
#endif 
	return 0; 
}


static void transfer_cmd(int fd, void *tx, void *rx, size_t len)
{
	int ret;
	
	struct spi_ioc_transfer tr_cmd = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	
	if (mode & SPI_TX_QUAD)
		tr_cmd.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr_cmd.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr_cmd.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr_cmd.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr_cmd.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr_cmd.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr_cmd);
	
	if (ret < 1)
		{
		pabort("can't send spi messageaaaaaaaaaaaaaa");
		DBG("++++++++++++++++++++++++++++++++++++++++++++");
		}

#ifdef DEBUG
	if (verbose)
		hex_dump_p(tx, len, len, "TX");
		
	hex_dump_p(rx, len, len, "RX");
#endif 


}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DSIRfcspovnd]\n", prog);
	puts("  -D --device       device to use (default /dev/spidev1.1)\n"
	     "  -I                Initialize device's registers and set value to a register \n"
	     "  -S                Save sensor data to File.\n"       
	     "  -R                Read device's All registers \n"
	     "  -s --speed        max speed (Hz)\n"
	     "\n"
//	     "  -p              Send data (e.g. \"1234\\xde\\xad\")\n"
	     "  -c --count        Numbfer of Recoding Sample\n"
	     "  -f --filename     File for Recoding or Initializing device\n" 
	     "  -p --printOption  Option of Recoding file (0:binary, 1:hex, 2:float)\n"
	     "  -n --rnumber      module register number\n" 
	     "  -d --data         register data\n"
	     "  -t --timeout      timeout\n"
	     "  -v --verbose      Verbose (show tx buffer)\n"
	     "  -A                autosave AD1, AD2, AD3, AD4 \n"
		 "  -M --mode		  Set AD data mode"
	);
	exit(1);
}


static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			// value name, option, 
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "count",   1, 0, 'c' }, 
			{ "filename", 1, 0, 'f'}, 
			{ "printOption", 1, 0, 'p'},
			{ "rnumber", 1, 0, 'n'},
			{ "gTimeout", 1, 0, 't'}, 
			{ "gMode", 1, 0, 'M'}, 
			{ NULL, 0, 0, 0 },
		};
		
		int c;

		c = getopt_long(argc, argv, "D:SIATRf:c:s:p:vn:d:t:M:m", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'A' : 
		  runf = PROCESS_AUTOSAVE; 
		  break; 		
		case 'S': 
		  runf = PROCESS_RECODING; 
		  break; 
		 case 'T':
		 	runf = PROCESS_STREAM;
		  break;		  
		case 'I':
		  runf = PROCESS_SETREG; 
		  flag = 1; 
		  break; 
		case 'R' :
		  runf = PROCESS_READREG;
	    	  flag = 1; 
		  break; 

 		case 'f': 
		  filename = optarg; 
		  break; 
		case 'c' : 
		  sampleNumber = atoi(optarg); 
		  break; 
		case 'p' : 
		  printOption = atoi(optarg); 
		  break; 
		case 'D':
		  device = optarg;
		  break; 
		case 's':
		  speed = atoi(optarg);
			break;
		case 'n' : 
		  rnumber = atoi(optarg); 
		  if(rnumber >= 0) 
		    flag = 0; 
		  else 
		    flag = 1; 
		  break; 
		case 'v':
			verbose = 1;
			break;		  
		case 't':
			gTimeout = atoi(optarg); 
			break; 
		case 'd':
		  rdata = atoi(optarg); 
		  break; 
		case 'M' : 
			runf = PROCESS_MODESET; 
			flag = 1; 
			gADMode = atoi(optarg); 
			break; 
		case 'm':
			gADMode = atoi(optarg); 
			break; 
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

int AnalogModule_Error()
{
//	execl("$SPATH/LED_error.sh", "$SPATH/LED_error.sh", "on", NULL); 
//	printf("Error AnalogModule!!\n"); 	
	AD_OnoffLed(1); 
}

int AnalogModule_Stop(int fd)
{
    // SPI Disable
        
    //AD_DisableModule(midx);     
    close(fd); 
}

int AnalogModule_Start(const char *devicename)
{
	int fd; 
	int ret; 
	
#ifdef DEBUG	
  printf("- Device : %s\n", devicename); 
#endif 

	if(!strncmp(devicename, "/dev/spidev0.2", 14) ) {
		midx = MODULE_AD1;
	}
	else if (!strncmp(devicename, "/dev/spidev0.3", 14)) {
		midx = MODULE_AD2;  
	}
	else if (!strncmp(devicename, "/dev/spidev0.4", 14)) {
		midx = MODULE_AD3; 
	} else if (!strncmp(devicename, "/dev/spidev0.5", 14)) {
		midx = MODULE_AD4; 
	}

	AD_EnableModule(midx);	
	
	fd = open(devicename, O_RDWR); 
	
	if (fd < 0) 
		pabort("can't open device"); 

	/*
	 * spi mode
	 */
	 
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	 
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	if (verbose)
	{
	  	printf("SPI Info \n"); 
	  	printf("- spi device : %s\n", devicename); 
		printf("- spi mode: %d\n", mode);
		printf("- bits per word: %d\n", bits);
		printf("- max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	}

	AD_DisableModule(midx);	
	return fd; 
}

void Start(int indexM) 
{
  // module power on 
}

int GetStatus(int indexM)
{
  int onoff = 0; 
  onoff = 1; 
  
  return onoff; 
}

int setADMode(int fd, int value)
{
	uint32_t CMD_tx[2]; 
	
	if ( value < MODE_TIME || value > MODE_RMS )
		value = MODE_TIME; 
		
	CMD_tx[0] = CMD_SET_MODE; 
	CMD_tx[1] = value; 

	transfer_cmd(fd, CMD_tx, send_rx, sizeof(CMD_STATUS_tx));
	printf("\n Send Mode Data : %d\n", value); 
	transfer_cmd(fd, CMD_STATUS_tx, send_rx, sizeof(CMD_STATUS_tx)); 
}

int getADMode(int fd)
{
	uint32_t CMD_tx[2]; 
	
	CMD_tx[0] = CMD_GET_MODE; 
	
	transfer_cmd(fd, CMD_tx, send_rx, sizeof(CMD_STATUS_tx));
	transfer_cmd(fd, CMD_STATUS_tx, send_rx, sizeof(CMD_STATUS_tx)); 

//	printf("getADMode : %d\n", (int)send_rx[1]); 	
	return send_rx[1]; 
}

void Stop(int indexM) 
{
  // module power off 
}


int32_t requestCnt(int fd)
{
	int32_t cnt; 
 
	transfer_cmd(fd, CMD_REQ_CNT_tx, send_rx, sizeof(CMD_STATUS_tx)); 
	transfer_cmd(fd, CMD_STATUS_tx, send_rx, sizeof(CMD_STATUS_tx)); 

	cnt = (int32_t)send_rx[1]; 
 
	return cnt;  
}

int32_t requestState_Read(int fd)
{
  int32_t status; 
  
  transfer_cmd(fd, CMD_STATUS_tx, send_rx, sizeof(CMD_STATUS_tx)); 
  transfer_cmd(fd, CMD_STATUS_tx, send_rx, sizeof(CMD_STATUS_tx));
    
  status = (int32_t) send_rx[1];
#ifdef DEBUG
  printf(" requestState_Read : %x(%d)\n", status, status);
#endif        

/*  if ( (int32_t) send_rx[0] == 0 )
 {
	return -1;  
 }
 else 
 {

  	status = (int32_t) send_rx[1];
  	#ifdef DEBUG
    	printf(" requestState_Read : %x(%d)\n", status, status);
    	#endif
 } */ 
                                                                                                                       
  return status; 
}

void request_ready(fd)
{
  transfer_cmd(fd, CMD_TEST_tx, send_rx, sizeof(CMD_TEST_tx)); 
}

void requestData_Start(int fd, int samplecount)
{
	//DBG("transfer_cmd start\n");

  CMD_DATA_tx[0] = CMD_DATA_tx[0] | samplecount ; 
  transfer_cmd(fd, CMD_DATA_tx, send_rx, sizeof(CMD_STATUS_tx)); 
}

int requestData_ReadNet(int fd, int fp, uint8_t option)
{
  int ret; 
   
  ret = transferNet(fd, fp, CMD_READDATA_tx, default_rx, sizeof(CMD_READDATA_tx), option);
  
  return ret; 
}
int requestData_Read(int fd, FILE* fp, uint8_t option)
{
  int ret; 
   
  ret = transfer(fd, fp, CMD_READDATA_tx, default_rx, sizeof(CMD_READDATA_tx), option);
  
  return ret; 
}


void requestData_End(int fd)
{
//DBG("transfer_cmd end\n");

  transfer_cmd(fd, CMD_END_tx, send_rx, sizeof(CMD_END_tx)); 
}

void requestReg_Read(int fd, uint8_t number)
{
#ifdef DEBUG	
	printf(" Start to read R%d. \n", number); 
#endif 
	CMD_READREG_tx[0] = CMD_READREG_tx[0] | number; 
	transfer_cmd(fd, CMD_READREG_tx, send_rx, sizeof(send_rx));
	transfer_cmd(fd, CMD_READREG_tx, send_rx, sizeof(send_rx));
	REGDATA_inner[number] = send_rx[1];

#ifdef DEBUG	  
	printf(" Finish to read R%d. \n", number); 
#endif   
}

void requestReg_ReadAll(int fd)
{
  int i; 
  
#ifdef DEBUG	
  printf(" Start to read registers. \n"); 
#endif 

  transfer_cmd(fd, CMD_READREGA_tx, send_rx, sizeof(send_rx));
  
  reg.cmd = CMD_CHANGE_TRMODE; 
  transfer_cmd(fd, &reg, default_rx, sizeof(reg));
  
  for(i=0;i<NB_REGISTER;i++)
  {
    REGDATA_inner[i] = (*(default_rx+i*4+0));
    REGDATA_inner[i] = REGDATA_inner[i] | (*(default_rx+i*4+1)) << 8;
    REGDATA_inner[i] = REGDATA_inner[i] | (*(default_rx+i*4+2)) << 16;
    REGDATA_inner[i] = REGDATA_inner[i] | (*(default_rx+i*4+3)) << 24;
  } 
  
#ifdef DEBUG	
  printf(" Finish to read registers. \n"); 
#endif   
}

void requestReg_Set(int fd, uint8_t number, uint32_t data)
{
#ifdef DEBUG	
  printf(" Start to set Register. \n"); 
#endif 

  CMD_SETREG_tx[0] = CMD_SETREG_tx[0] | number; 
  CMD_SETREG_tx[1] = data; 
  
  transfer_cmd(fd, CMD_SETREG_tx, CMD_SETREG_rx, sizeof(CMD_SETREG_tx));
  transfer_cmd(fd, CMD_READREG_tx, send_rx, sizeof(send_rx));

#ifdef DEBUG	  
  printf(" Finish to set Register. \n"); 
#endif 
}

void requestReg_SetAll(int fd, uint32_t *regbuf)
{
  int i; 
  
#ifdef DEBUG	
  printf(" Start to initialiaze Registers. \n"); 
#endif 

  transfer_cmd(fd, CMD_SETREGA_tx, send_rx, sizeof(send_rx));
  
  reg.cmd = CMD_SET_REGA; 
  
  for ( i = 0 ; i < NB_REGISTER; i++ )
  {
    reg.data[i] = regbuf[i]; 
  }
  
  transfer_cmd(fd, &reg, default_rx, sizeof(reg));
#ifdef DEBUG	  
  printf(" Finished to initialiazie Registers. \n"); 
#endif   
} 

int saveData(int fd, FILE* fp) 
{
  int i, j, ret; 
   
#ifdef DEBUG	
  printf(" Recodig Start to %s. \n", filename); 
#endif 
 
//  request_ready(fd); 

/*  gADMode = getADMode(fd); 
  
  if ( gADMode == 2 )
       setADMode(fd, gADMode);   */
       
  sampleNumber =  requestCnt(fd); 

#ifdef SAMPLECNT_DEBUG
  	printf(" SampleCnt : %d\n", sampleNumber); 
#endif 

//jonsama end

  if ( sampleNumber > 0 && sampleNumber <= MAX_SAMPLE )
  { 
 	if ( sampleNumber == ( MAX_SAMPLE -1 ) ) 
 		printf(" %d 777 OVERRUN \n", fd); 
 		
  	requestData_Start(fd, sampleNumber); 
    
  	for ( i = 0 ; i < 1 ; i++) // 512 sample(2048byte) * 400 = 204800 sample/s/ch = 51.2kS/s 
  	{
  	  ret = requestData_Read(fd, fp, printOption);
    
  	  if( ret < 0 )
  	  {
  //  		requestData_End(fd); 	
  //  		break; 
  	  }	
  	}
  	
  	requestData_End(fd); 
  } 
  else
  {
	if ( sampleNumber == 0 )
		printf("No Data\n"); 
		
  	//goto sample_count_error;
  }
  
#ifdef DEBUG	  
  printf(" Recoding Fininshed.\n"); 
#endif 
//  fclose(fp);   
  //jonsama
  //printf(" Recording Finished : %d\n",ret ); 
  
  return ret; 
//jonsama
sample_count_error:
 // printf(" sample count error\n"); 
  return 1;
}


int streamingData(int socket_fd, int channelIndex) 
{
  int i, j, ret; 
  int channel_fd;
#ifdef DEBUG	
  printf(" Recodig Start to %s. \n", filename); 
#endif 
 FILE *ffdx;

ffdx = fopen("ad_test.dat", "ab+");

	channel_fd = getADFileDiscription(channelIndex);
setADMode(channel_fd, 1); 

   	
do{
	
	AD_EnableModule(channelIndex);			
  sampleNumber =  requestCnt(channel_fd); 

#if 1//def SAMPLECNT_DEBUG
  	DBG(" SampleCnt : %d\n", sampleNumber); 
#endif 

//jonsama end

  if ( sampleNumber > 0 && sampleNumber <= MAX_SAMPLE )
  { 
 	if ( sampleNumber == ( MAX_SAMPLE -1 ) ) 
	{
 		DBG_A(" %d OVERRUN  \n", channel_fd); 
	}
 	else
	{
 		DBG_A(" %d sampleNumber \n", sampleNumber); 		
	}
  	requestData_Start(channel_fd, sampleNumber); 
    
  	for ( i = 0 ; i < sampleNumber ; i++) // 512 sample(2048byte) * 400 = 204800 sample/s/ch = 51.2kS/s 
  	{
		if(getRequestedStopStreamingStatus() == DEF_TRUE)
		{
			initRequestedStopStreamingStatus();
			DBG("Stopped by request PC\n");
			requestData_End(channel_fd); 
			
			AD_DisableModule(channelIndex); 		
			return 0;
		}
  	
  	//  ret = requestData_ReadNet(channel_fd, socket_fd, printOption);
  	  ret = requestData_Read(channel_fd, ffdx, printOption);
    
  	  if( ret < 0 )
  	  {
  //  		requestData_End(fd); 	
  //  		break; 
  	  }	
  	}
  	
  	requestData_End(channel_fd); 
	
  } 
  else
  {
	if ( sampleNumber == 0 )
		DBG("No Data\n"); 
		
  	//goto sample_count_error;
  }
  
  AD_DisableModule(channelIndex);		  

// request_ready(fdx); 
  
//  gADMode = getADMode(fdx); 
  
//  DBG(" gADMode : %d\n", gADMode); 
  //if ( gADMode == 2 )
 
//	   AD_DisableModule(channel_fd);
	 //  setADMode(fdx, 1); 
  
}while(1);
 // sleep(1);
  
 fclose(ffdx);
 DBG(" Streaming Fininshed.\n"); 
   // if ( gADMode == 2 )
       //setADMode(fdx, 1);  
#ifdef DEBUG	  
  printf(" Recoding Fininshed.\n"); 
#endif 
//  fclose(fp);   
  //jonsama
  //printf(" Recording Finished : %d\n",ret ); 
  
  return ret; 
//jonsama
sample_count_error:
 // printf(" sample count error\n"); 
  return 1;
}



void readRegister(int fd, int flag)
{
	if ( flag ) 
	{
	  requestReg_ReadAll(fd);
	  PrintRegisters(fd); 
	}  
	else 
	{
		if (rnumber == REGISTER_ADMODE_ID) {
			gADMode = getADMode(fd); 
	
		#ifdef DEBUG	
			printf("Mode : %d\n", gADMode); 			
		#else
			printf("%d", gADMode); 	
		#endif 
		 
			
		} else  {
			requestReg_Read(fd, rnumber);
	  
			if (rnumber == 0) 
				printf("%d\n", REGDATA_inner[0]);
			else  
				PrintRegister(fd, rnumber); 	
		}
	}
}

int setRegister(int fd)
{
  int ret = 0; 
  
  if(!flag) 
    REGDATA_inner[rnumber] = rdata; 
  else 
  {
    char szData[50] = {0}; 
    int i, cnt;
    uint32_t data, index; 
    FILE* rf; 
        
    rf = fopen(filename,"r");
        
    if(rf) {
      
      if ( fgetc(rf) == '#') {
        
        for ( i = 0; i < NB_REGISTER ; i++)
        {
          fscanf(rf, "%d%d", &index, &data); 
          printf("  index : %d,\t data : %d\n", index, data);
              
          REGDATA_inner[index] = data; 
        }  
      }
      else {
        printf("This file is not register of module\n"); 
        ret = -1; 
      }
    }
    fclose(rf); 
  }  

  if(ret != -1)   
  {
    if (flag) 
    {
      requestReg_SetAll(fd, REGDATA_inner);
      setADMode(fd, REGDATA_inner[0]); 
    }
    else   
      requestReg_Set(fd, rnumber, REGDATA_inner[rnumber]);
  }
}

void PrintRegisters(int fd)
{
  int i; 
  
  //requestReg_ReadAll(fd);
  
  for(i=0;i<NB_REGISTER;i++)
  {
    printf("  R%d : 0x%x\n", i, REGDATA_inner[i]); 
  }
}

void PrintRegister(int fd, int num)
{
  printf("  R%d : 0x%x\n", num, REGDATA_inner[num]); 
}

void init(int fd, int argc, char *argv[])
{
  printf("\nRead Module Registers \n");
  printf("===================== \n"); 
  readRegister(fd, 1);
  
  if(argc == 1) {
    print_usage(argv[0]);
  }
}

char* getDeviceName(const char *device)
{
	if(!strcmp(device, "/dev/spidev0.2"))
		return "AD1";
	else  if(!strcmp(device, "/dev/spidev0.3"))
		return "AD2"; 
	else  if(!strcmp(device, "/dev/spidev0.4"))
		return "AD3"; 
	else  if(!strcmp(device, "/dev/spidev0.5"))
		return "AD4";
	else 
		return "NULL";	
} 

int getADFileDiscription(int channel)
{
	int ret_value;
	
	switch(channel)
	{
		case 1:
			ret_value = fd1;
		break;

		case 2:
			ret_value = fd2;
		break;

		case 3:
			ret_value = fd3;
			break;
		case 4:
			ret_value = fd4;
			break;
	}

	return ret_value;
}


int checkADModule(void)						
{
	char buf[256]; 
	int ret = 0; 
		
	fd1 = AnalogModule_Start("/dev/spidev0.2");  

	AD_EnableModule(MODULE_AD1);
	ret = requestState_Read(fd1); 
	
	if ( ret != 0 && ret != -1) { 
	//if ( requestState_Read(fd1) != 0 && requestState_Read ) {
		deviceflag = deviceflag | FLAG_AD1 ; 
		sprintf(buf, "%s/%s", filename, "AD1.dat");
		//printf("%s\n", buf); 	
		fp1 = fopen(buf, "ab+"); 
	} else {
		printf (" -- Not Found. %s.\n", getDeviceName("/dev/spidev0.2")); 		
		AnalogModule_Error(); 
		AnalogModule_Stop(fd1);
	}  
	AD_DisableModule(MODULE_AD1); 

	fd2 = AnalogModule_Start("/dev/spidev0.3"); 
	AD_EnableModule(MODULE_AD2); 
        ret = requestState_Read(fd2);
        
        if ( ret != 0 && ret != -1) {
              
	//if ( requestState_Read(fd2) != -1 ) {
		deviceflag = deviceflag | FLAG_AD2 ; 
		sprintf(buf, "%s/%s", filename, "AD2.dat"); 
		//printf("%s\n", buf); 
		fp2 = fopen(buf, "ab+"); 
	} else  {
		printf (" -- Not Found. %s.\n", getDeviceName("/dev/spidev0.3")); 		
 		AnalogModule_Error(); 
		AnalogModule_Stop(fd2);
	}  
	AD_DisableModule(MODULE_AD2);
	
	fd3 = AnalogModule_Start("/dev/spidev0.4"); 
	AD_EnableModule(MODULE_AD3); 
      	
      	ret = requestState_Read(fd3);
        if ( ret != 0 && ret != -1) {
//	if ( requestState_Read(fd3) != -1 ) {
		deviceflag = deviceflag | FLAG_AD3 ;
		sprintf(buf, "%s/%s", filename, "AD3.dat");
		//printf("%s\n", buf);
		fp3 = fopen(buf, "ab+"); 		
	} else {
		printf (" -- Not Found. %s.\n", getDeviceName("/dev/spidev0.4")); 		
		AnalogModule_Error(); 
		AnalogModule_Stop(fd3);

	}  
	AD_DisableModule(MODULE_AD3);
	
	fd4 = AnalogModule_Start("/dev/spidev0.5"); 
	AD_EnableModule(MODULE_AD4); 
      	
      	ret = requestState_Read(fd4);
        if ( ret != 0 && ret != -1) {
	//if ( requestState_Read(fd4) != -1 ) {
		deviceflag = deviceflag | FLAG_AD4 ; 
		sprintf(buf, "%s/%s", filename, "AD4.dat");
		//printf("%s\n", buf);
		fp4 = fopen(buf, "ab+"); 
	} else {
		printf (" -- Not Found. %s.\n", getDeviceName("/dev/spidev0.5")); 		
		AnalogModule_Error(); 
		AnalogModule_Stop(fd4);
	}  
	AD_DisableModule(MODULE_AD4);
	
	return deviceflag; 
}

int main(int argc, char *argv[])
{
	int ret = 0, i;
	int size;
	pthread_t server_pthread;
	int fd;
	char* serial[16]={0,};
	
	parse_opts(argc, argv);

	if(!AD_PowerCheck(verbose))
	{
		printf("now, AD_Power - Off\n"); 

		// Sensor ERR LED ON 
		//AnalogModule_Error();
		ret = -1; 
		
//		printf("%d\n", ret); 
		return ret; 
	}


		
		fd = open("/dev/mtd0", O_RDONLY);
		if(fd != -1)
		{
			lseek(fd, 0x30025, 0);
			read(fd, serial, 12);
			printf("DAQ ID  : %s\n", serial);
		}
		
		pthread_create(&server_pthread, NULL, server_thread, serial);
		pthread_join(server_pthread, NULL);
		
	printf("Exit App\n");

#if 0 //temp
		switch (runf) {
 
			case PROCESS_STREAM:
			{
				int fd;
				char* serial[16]={0,};
				
				fd = open("/dev/mtd0", O_RDONLY);
				if(fd != -1)
				{
					lseek(fd, 0x30025, 0);
					read(fd, serial, 12);
					DBG("Serial : %s\n", serial);
				}
				
				pthread_create(&server_pthread, NULL, server_thread, serial);
				pthread_join(server_pthread, NULL);
				
			}
			break;
					
			case PROCESS_SETREG: 
				AD_EnableModule(midx); 
				setRegister(fdx);
				AD_DisableModule(midx); 
			break; 
		
			case PROCESS_READREG: 
				AD_EnableModule(midx); 
	#ifdef DEBUG	
				printf("read reg : %d \n"); 
	#endif       
				readRegister(fdx, flag); 
				AD_DisableModule(midx); 
			break; 
			
			case PROCESS_MODESET:
                               AD_EnableModule(midx);
				setADMode(fdx, gADMode); 
			  	AD_DisableModule(midx); 
			break; 
		}  // switch 
#endif	
	return ret;
}

