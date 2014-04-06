#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>

//define static ioctl commands
#define SEGMENTPORT_BASE				0xbb
#define SEGMENT_WRITE_LOW		 _IOW(SEGMENTPORT_BASE,0,4)
#define SEGMENT_WRITE_HIGH	     _IOW(SEGMENTPORT_BASE,1,4)
#define SEGMENT_TIMER_START      _IOW(SEGMENTPORT_BASE,2,4)
#define SEGMENT_TIMER_STOP	     _IOW(SEGMENTPORT_BASE,3,4)
#define SEGMENT_TIMER_INIT       _IOW(SEGMENTPORT_BASE,4,4)
#define SEGMENT_TIMER_OFF        _IOW(SEGMENTPORT_BASE,5,4)

#define TEXTLCDPORT_BASE				0xbc
#define TEXTLCD_COMMAND_SET     _IOW(TEXTLCDPORT_BASE,0,32)
#define TEXTLCD_FUNCTION_SET    _IOW(TEXTLCDPORT_BASE,1,32)
#define TEXTLCD_DISPLAY_CONTROL _IOW(TEXTLCDPORT_BASE,2,32)
#define TEXTLCD_CURSOR_SHIFT    _IOW(TEXTLCDPORT_BASE,3,32)
#define TEXTLCD_ENTRY_MODE_SET  _IOW(TEXTLCDPORT_BASE,4,32)
#define TEXTLCD_RETURN_HOME     _IOW(TEXTLCDPORT_BASE,5,32)
#define TEXTLCD_CLEAR           _IOW(TEXTLCDPORT_BASE,6,32)
#define TEXTLCD_DD_ADDRESS	    _IOW(TEXTLCDPORT_BASE,7,32)
#define TEXTLCD_WRITE_BYTE      _IOW(TEXTLCDPORT_BASE,8,32)

#define DISCRETE_BASE				0xbd
#define DISCRETE_VALUMEUP		_IOW(DISCRETE_BASE,0,8)
#define DISCRETE_VALUMEDOWN		_IOW(DISCRETE_BASE,1,8)
#define DISCRETE_VALUMEREAD		_IOW(DISCRETE_BASE,2,8)
#define DISCRETE_VALUMEWRITE		_IOW(DISCRETE_BASE,3,8)

#define XPOS1 0
#define YPOS1 0
#define XPOS2 640
#define YPOS2 480
void sub_init(int pow);

//static variables define
static int fd_push;
static int fd_segLed;
static int fd_textLcd;
static int fd_discreteLed;
static int fd_gpio;
static int fd_frameBuffer;
static int fd_touch;
// devicefile FD
static struct fb_var_screeninfo fbvar;
static struct fb_fix_screeninfo fbfix;
static int playing;
static unsigned char vkey;// it will save the btn input data
static unsigned int current_valume;
static unsigned short pixel;
static int power=0;
static pid_t id, i;
static int quit;
static int current_track = 0;
char buf1[20] = "Now palying music.. ";
char buf2[20] = "Pause..             ";
char buf3[20] = "track 1. revert     ";
char buf4[20] = "track 2. caffeine   ";
char buf5[20] = "track 3. bad person ";
char buf6[20] = "track 4. officially ";
char buf7[20] = "";
char bufinit1[20] = "MP3 Player Power ON ";
char bufinit2[20] = "Now in init         ";
int t,tt,offset;
int i;
unsigned short *pfbdata;

//type def & struct def
struct ts{
	short x;
	short y;
	short pressure;
}; struct ts location; 

struct strcommand_variable{
	char rows; char nfonts; char display_enable; char cursor_enable;
	char nblink; char set_screen; char set_rightshift; char increase; char nshift; char pos;
	char command; char strlength; char buf[20];
};struct strcommand_variable strcommand;

typedef struct {
	unsigned int     bfSize;
	unsigned short   bfReserved1;
	unsigned short   bfReserved2;
	unsigned int  bfOffBits;
}BITMAPFILEHEADER;

typedef struct {
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	unsigned int biXPelsPerMeter;
	unsigned int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
}BITMAPINFOHEADER;

void usrsignal(int sig) {
	printf("SIGUSR1\n");
	read(fd_push,&vkey,1);

}
void usrsignal2(int sig) {
	printf("SIGUSR2\n");
	read(fd_gpio,NULL,0);
	if(power == 0){
		power = 1;
		printf("Power off\n");
		sub_init(1);
	}else{
		power = 0;
		printf("Power on\n");
		sub_init(0);
	}
}//signal handling func ( signal by button input )

///////////////////////////////////////////////////////////////////////////////////////
//Frame buffer using code
///////////////////////////////////////////////////////////////////////////////////////
void read_bmp(char *filename, char **pDib, char **data, int *cols, int *rows)
{
	BITMAPFILEHEADER bmpHeader;     
	BITMAPINFOHEADER *bmpInfoHeader;
	unsigned int size;
	unsigned char ID[2];
	int nread;
	FILE *fp;
	
	printf("%s\n",filename);
	fp = fopen(filename,"rb"); 
	if(fp == NULL) {
		printf("ERROR\n");
		return;
	}

	ID[0] = fgetc(fp);
	ID[1] = fgetc(fp);      //fget은 한문자씩 읽어오는 함수

	if(ID[0] != 'B' && ID[1]!='M') {
		fclose(fp);
		return;      //BMP파일이 맞는지 화인
	}

	nread = fread(&bmpHeader.bfSize,1,sizeof(BITMAPFILEHEADER),fp);
	size = bmpHeader.bfSize-sizeof(BITMAPFILEHEADER);

	*pDib = (unsigned char *)malloc(size);
	fread(*pDib,1,size,fp);

	bmpInfoHeader = (BITMAPINFOHEADER*)*pDib;

	if(24 != bmpInfoHeader->biBitCount)
	{
		printf("bfSize = %d\n",bmpHeader.bfSize);
		printf("bisize = %d\n",bmpInfoHeader->biSize);  
		printf("biWidth = %d\n",bmpInfoHeader->biWidth);  
		printf("biHight = %d\n",bmpInfoHeader->biHeight);  
		printf("biPlanes = %d\n",bmpInfoHeader->biPlanes);  
		printf("bitcount = %d\n",bmpInfoHeader->biBitCount);
		printf("biCompression = %d\n",bmpInfoHeader->biCompression);
		printf("biSizeImage = %d\n",bmpInfoHeader->biSizeImage);
		printf("It supports only 24bit bmp!\n");
		fclose(fp);
		return;
	}

	*cols = bmpInfoHeader->biWidth;
	*rows = bmpInfoHeader->biHeight;

	*data = (char *)(*pDib + bmpHeader.bfOffBits - sizeof(bmpHeader)-2);
	fclose(fp);
}

void close_bmp(char **pDib)
{
	free(*pDib);
}

///////////////////////////////////////////////////////////////////////////
void tftlcd(){
	int cols, rows;
	char *pData,*data;
	char r,g,b;
	unsigned short bmpdata1[640*480];
	unsigned short pixel;
	unsigned short *pfbmap;
	struct fb_var_screeninfo fbvar;
	int fbfd;
	int i,j,k,t;

	char pic1[17]="track_1_play.bmp";
	char pic2[17]="track_2_play.bmp";
	char pic3[17]="track_3_play.bmp";
	char pic4[17]="track_4_play.bmp";
	char pic5[17]="track_1_stop.bmp";
	char pic6[17]="track_2_stop.bmp";
	char pic7[17]="track_3_stop.bmp";
	char pic8[17]="track_4_stop.bmp";

	pfbmap = (unsigned short *)mmap(0, YPOS2*XPOS2*2, PROT_READ|PROT_WRITE, MAP_SHARED, fd_frameBuffer, 0);
	if((unsigned)pfbmap == (unsigned)-1)
	{
		perror("fd_frameBuffer mmap");
		exit(1);
	}

	switch(current_track){
	case 0:
		if(playing == 0) { read_bmp(pic1,&pData,&data, &cols, &rows);}
		else { read_bmp(pic5,&pData,&data, &cols, &rows); }		
		break;
	case 1:
		if(playing == 0) { read_bmp(pic2,&pData,&data, &cols, &rows);}
		else { read_bmp(pic6,&pData,&data, &cols, &rows); }
		break;
	case 2:
		if(playing == 0) { read_bmp(pic3,&pData,&data, &cols, &rows);}
		else { read_bmp(pic7,&pData,&data, &cols, &rows); }
		break;
	case 3:
		if(playing == 0) { read_bmp(pic4,&pData,&data, &cols, &rows);}
		else { read_bmp(pic8,&pData,&data, &cols, &rows); }
		break;
	default:
		break;	
	}

	for(j=0;j<rows;j++){
		k = j*cols*3;
		t = (rows -1 -j)*cols;
		for(i=0;i<cols;i++)
		{
			b = *(data+(k+i*3));
			g = *(data + (k+i*3+1));
			r = *(data + (k+i*3+2));
			pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
			bmpdata1[t+i] = pixel;
		}
		//memcpy(pfbmap,bmpdata1,XPOS2*2);
	}
	memcpy(pfbmap,bmpdata1,XPOS2*YPOS2*2);
	munmap(pfbmap,XPOS2*YPOS2*2);	
}
void textlcdsetting(){
	//text lcd



	if(playing == 0) {memcpy(&strcommand.buf[0],buf2,40);}
		else { memcpy(&strcommand.buf[0],buf1,40); }

	write(fd_textLcd,&strcommand,32);

	//둘쨋
	switch(current_track){
	case 0:	
		memcpy(&buf7,&buf3,20);
		break;
	case 1:
		memcpy(&buf7,&buf4,20);
		break;
	case 2:
		memcpy(&buf7,&buf5,20);
		break;
	case 3:
		memcpy(&buf7,&buf6,20);
		break;
	default:
		break;	
	}

	strcommand.pos = 40 ;
	ioctl(fd_textLcd,TEXTLCD_DD_ADDRESS,&strcommand,32);	
	for(i=0;i<20;i++){
		memcpy(&strcommand.buf[0],&buf7[i],1);
		
		ioctl(fd_textLcd,TEXTLCD_WRITE_BYTE,&strcommand,32);
	}
}

void playPrev(void){
	if(current_track == 0) {current_track = 3;}
	else {current_track--;}

	//segment led setting
	ioctl(fd_segLed, SEGMENT_TIMER_STOP, NULL, 4);
	ioctl(fd_segLed, SEGMENT_TIMER_INIT, NULL, 4);
	ioctl(fd_segLed, SEGMENT_TIMER_START, NULL, 4);
	
	//fb setting

	playing = 1;
	tftlcd();
	textlcdsetting();
	
}
void playNext(void){
	current_track = (current_track+1)%4;

	//segment led setting
	ioctl(fd_segLed, SEGMENT_TIMER_STOP, NULL, 4);
	ioctl(fd_segLed, SEGMENT_TIMER_INIT, NULL, 4);
	ioctl(fd_segLed, SEGMENT_TIMER_START, NULL, 4);
	
	//fb setting
	playing = 1;
	tftlcd();
	textlcdsetting();
}

void stop(void){
	playing = 0;
	ioctl(fd_segLed, SEGMENT_TIMER_STOP, NULL, 4);
	tftlcd();
	textlcdsetting();
	
}
void play(void){
	playing = 1;
	ioctl(fd_segLed, SEGMENT_TIMER_START, NULL, 4);
	tftlcd();
	textlcdsetting();
}
void valumeUp(void){
	ioctl(fd_discreteLed, DISCRETE_VALUMEUP, NULL, 8);
	ioctl(fd_discreteLed, DISCRETE_VALUMEREAD, &current_valume, 8);
	
	//textLcd setting

	//fb setting
}
void valumeDown(void){
	ioctl(fd_discreteLed, DISCRETE_VALUMEDOWN, NULL, 8);
	ioctl(fd_discreteLed, DISCRETE_VALUMEREAD, &current_valume, 8);
	//textLcd setting

	//fb setting

}
void init(void){
	//device file open
	fd_push = open("/dev/pushbuttonport", O_RDWR);
	if(fd_push < 0) { printf("application : push button driver open fails!\n");	exit(-1);}
	fd_segLed = open("/dev/segmentport", O_RDWR);
	if(fd_segLed < 0) { printf("application : segment led driver open fails!\n"); exit(-1);}
	fd_textLcd = open("/dev/textlcdport", O_RDWR);
	if(fd_textLcd < 0) { printf("application : textlcd driver open fails!\n");exit(-1);}
	fd_discreteLed = open("/dev/discreteledport", O_RDWR);
	if(fd_discreteLed < 0) { printf("application : discrete led driver open fails!\n");exit(-1);}
	fd_frameBuffer = open("/dev/fb", O_RDWR);
	if(fd_frameBuffer < 0) { printf("application : frame buffer driver open fails!\n");	exit(-1);}
	fd_gpio = open("/dev/gpioport", O_RDWR);
	if(fd_gpio < 0) { printf("application : gpio driver open fails!\n");	exit(-1);}
	fd_touch = open("/dev/touch", O_RDWR);
	if(fd_touch < 0) { printf("application : touch screen driver open fails!\n");	exit(-1);}

	/*
	int status;
	status = ioctl(fd_frameBuffer, FBIOGET_VSCREENINFO, &fbvar);
	if(status < 0){ printf("Error fbdev ioctl(FSCREENINFO)₩n"); exit(-1);}
	status = ioctl(fd_frameBuffer, FBIOGET_FSCREENINFO, &fbfix);
	if(status < 0){ printf("Error fbdev ioctl(FSCREENINFO)₩n"); exit(-1);}
	*/

	//strcommand init
	strcommand.rows = 0;
	strcommand.nfonts = 0;
	strcommand.display_enable = 1;
	strcommand.cursor_enable = 0;
	strcommand.nblink = 0;
	strcommand.set_screen = 0;
	strcommand.set_rightshift = 1;
	strcommand.increase = 1;
	strcommand.nshift = 0;
	strcommand.pos = 0;
	strcommand.command = 1;
	strcommand.strlength = 20;

		
	power = 1;
	sub_init(power);
}
void sub_init(int pow){
	if(pow ==1)
		//1.fb
		current_track = 0;
		playing = 0;
		tftlcd();

		//2.text LCD

		//첫줄
		memcpy(&strcommand.buf[0],bufinit1,40);
		write(fd_textLcd,&strcommand,32);

		//둘쨋
		strcommand.pos = 40 ;
		ioctl(fd_textLcd,TEXTLCD_DD_ADDRESS,&strcommand,32);	
		for(i=0;i<20;i++){
			memcpy(&strcommand.buf[0],&bufinit2[i],1);
			
			ioctl(fd_textLcd,TEXTLCD_WRITE_BYTE,&strcommand,32);
		}

		//3.discrete LED
		ioctl(fd_discreteLed, DISCRETE_VALUMEWRITE, 3, 8);

		//4.segment LED
		ioctl(fd_segLed, SEGMENT_TIMER_INIT, NULL, 4);
	}else{
		//1.fb
		pfbdata = (unsigned short *)mmap(0, YPOS2*XPOS2*2, PROT_READ|PROT_WRITE, MAP_SHARED, fd_frameBuffer, 0);	
		for(t = YPOS1; t < YPOS2; t++){
			offset = t * XPOS2;
			for(tt = XPOS1; tt < XPOS2; tt++){
				*(pfbdata + offset + tt) = 0;
			}
		}
		munmap(pfbdata,XPOS2*YPOS2*2);	
			
		//2.text LCD
		ioctl(fd_textLcd,TEXTLCD_CLEAR,&strcommand,32);
	
		//3.discrete LED
		ioctl(fd_discreteLed, DISCRETE_VALUMEWRITE, 0, 8);

		//4.segment LED
		if(playing == 1){
			ioctl(fd_segLed, SEGMENT_TIMER_STOP, NULL, 4);
			playing = 0;
		}
		ioctl(fd_segLed, SEGMENT_TIMER_OFF, NULL, 4);
	}
}
int main(void){
	//signal handler mapping
	(void)signal(SIGUSR1,usrsignal);
	(void)signal(SIGUSR2,usrsignal2);

	//var define 
	id = getpid();	//pid init
	quit = 1;	//loop control
	
	init();
	
	write(fd_push,&id,4);
	write(fd_gpio,&id,4);

	//button input sencing start
	printf("Start monitoring the input button.\n");

	//start busyWaiting
	while(quit){
		if(power = 0){ 
		}else{
			//touch screen work
			read(fd2, &location, sizeof(location));	// Receive touch input 
			if( (localtion.x >= 160) && (location.x <= 230) && (location.y >=410) && (location.y <=460) ){
				printf("Pressed the play prev screen\n");
				playPrev();
			}else if( (localtion.x >= 280) && (location.x <= 370) && (location.y >=400) && (location.y <=470) ){
				if(playing == 1 ){
					stop();
					printf("Pressed the stop screen");				
				} else {
					play();
					printf("Pressed the play screen\n");
				}
			}else if( (localtion.x >= 440) && (location.x <= 500) && (location.y >=410) && (location.y <=460) ){
				printf("Pressed the play next screen\n");
				playNext();
			}
			
			//busy waiting
			switch(vkey){
			case 0x0: break;
				//no input
			case 0x1: //1st btn
				valumeUp();
				printf("Pressed the valume up button \n");
				vkey = 0; 
				break;
			case 0x2: //2nd btn
				valumeDown();
				printf("Pressed the valume down button\n"); 
				vkey = 0; 
				break;
			case 0x4: //3th btn
				printf("3th btn : Did not yet define the function of this button.\n");
				vkey = 0; 
				break;
			case 0x8: //4th btn
				printf("4th btn : Did not yet define the function of this button.\n");
				vkey = 0;
				break;
			case 0x10: //5th btn
				printf("5th btn : Did not yet define the function of this button.\n");
				vkey = 0; 
				break;
			case 0x20: //6th btn
				playNext();
				printf("Pressed the play next button\n");
				vkey = 0; 
				break;
			case 0x40: //7th btn
				if(playing == 1 ){
					stop();
					printf("Pressed the stop button");				
				} else {
					play();
					printf("Pressed the play button\n");
				}
				sleep(1);
				vkey = 0; 
				break;
			case 0x80: //8th btn
				playPrev();
				printf("Pressed the play prev button\n");
				vkey = 0;
				break;
			default : 
				quit = 0; 
				printf("Good bye!! Quit the program.\n");
				break;
			}// end of switch
		}//end of if
	}// end of while

	//device file close
	close(fd_push);
	close(fd_segLed);
	close(fd_textLcd);
	close(fd_discreteLed);
	close(fd_gpio);
	close(fd_frameBuffer);
	close(fd_touch);
	return 0;
}
