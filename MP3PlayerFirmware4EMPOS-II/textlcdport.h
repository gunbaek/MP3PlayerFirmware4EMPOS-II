#define TEXTLCDPORT_MAJOR   0
#define TEXTLCDPORT_NAME    "TEXT LCD PORT"
#define TEXTLCDPORT_MODULE_VERSION "TEXT LCD PORT V0.1"
#define TEXTLCDPORT_ADDRESS 0xf1700000
#define TEXTLCDPORT_ADDRESS_RANGE 2

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


//Global variable
static int textlcdport_usage = 0;
static int textlcdport_major = 0;

struct strcommand_varible {
	char rows;
	char nfonts;
	char display_enable;
	char cursor_enable;
	char nblink;
	char set_screen;
	char set_rightshift;
	char increase;
	char nshift;
	char pos;
	char command;
	char strlength;
	char buf[20];
};

void setcommand(unsigned short command);
void initialize_textlcd();
void write_string(int row, char *str, int length);
//void usr_wait(unsigned long delay_factor);
void setcommand(unsigned short command);
void writebyte(char ch);
void initialize_textlcd();
void write_string(int row, char *str,int length);
int function_set(int rows, int nfonts);
int display_control(int display_enable, int cursor_enable, int nblink);
int cusrsor_shift(int set_screen, int set_rightshit);
int entry_mode_set(int increase, int nshift);
int return_home();
int clear_display();
int set_ddram_address(int pos);
