#include "LCDWIKI_GUI.h" //Core graphics library
#include "LCDWIKI_KBV.h" //Hardware-specific library
#include "TouchScreen.h" // only when you want to use touch screen 
//if the IC model is known or the modules is unreadable,you can use this constructed function
LCDWIKI_KBV mylcd(ILI9486,A3,A2,A1,A0,A4); //model,cs,cd,wr,rd,reset

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

//param calibration from kbv
#define TS_MINX 906
#define TS_MAXX 116

#define TS_MINY 92 
#define TS_MAXY 952

#define MINPRESSURE 5 //10
#define MAXPRESSURE 1000
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

//define some colour values
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define header "-LCD0-"

typedef struct s_boxdata {
	int iboxnum; //not actually required, array position will set
	String sboxdata;
	int startx;
	int starty;
	int endx;
	int endy;
};

s_boxdata boxdata[7];

int boxcount = 7;
int debounce = 0;
int lastpressed = -1;
int pressed = -1;
bool sendit = false;
bool firsttime = true;
bool refresh = false;
int numboxes = 7;


void setup() {
	Serial.begin(57600); //115200 was causing corruption, slower wasn't really fast enough
	mylcd.Set_Rotation(0);
//rotating doesn't affect touch. Keep orientaion in the same direction or you'll drive yourself crazy.
//
//  320x480 in portrait with usb at top
//
	mylcd.Init_LCD();

//	Serial.println(mylcd.Read_ID(), HEX);
	mylcd.Fill_Screen(BLACK);
	//Serial.println(header);
	
	//initialise boxdata
	for (int ii = 0; ii <= 7; ii++) {
		boxdata[ii].sboxdata = "";
		boxdata[ii].iboxnum = ii;
		boxdata[ii].startx = 0;
		boxdata[ii].starty = 0;
		boxdata[ii].endx = 0;
		boxdata[ii].endy = 0;
	}
}

void printboxed(String msgstr, int boxnum, byte boxsize) {
	int sx = 1; //boxdata[boxnum].startx+1;
	int sy = 1;//boxdata[boxnum].starty+1;
	int ex = 200;//boxdata[boxnum].endx+1;
	int ey = 100; //boxdata[boxnum].endy+1;

	Serial.println("msgstr in printboxed(): "+msgstr+"!sx:"+String(sx)+" sy:"+String(sy)+" ex:"+String(ex)+" ey:"+String(ey));
	delay(100);
//some fuckery is happening here. adding the above delay allows me to see it got this far
//but then craps out
	mylcd.Set_Text_Back_colour(BLACK);
//	Serial.println("1a");
//    mylcd.Set_Draw_color(YELLOW);
//	Serial.println("1b");
    mylcd.Set_Draw_color(BLACK);
//	Serial.println("1c");

	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
//	Serial.println("1d");
//	delay(50);
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Draw_Rectangle(sx,sy,ex,ey);  	
//	Serial.println("2");
//	delay(50);
	mylcd.Set_Text_Size(boxsize);
	mylcd.Set_Text_colour(YELLOW);
	mylcd.Print_String(msgstr, sx+4, sy+3);
//	Serial.println("3");
//	delay(50);
//	Serial.println("exit printboxed()");
//	delay(50);
}


void printboxedhighlight(String msgstr, int boxnum, byte boxsize) {
	int sx = 1; //boxdata[boxnum].startx+1;
	int sy = 1;//boxdata[boxnum].starty+1;
	int ex = 200;//boxdata[boxnum].endx+1;
	int ey = 100; //boxdata[boxnum].endy+1;
	mylcd.Set_Text_Back_colour(YELLOW);
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
	mylcd.Set_Text_Size(boxsize);
	mylcd.Set_Text_colour(BLACK);
	mylcd.Print_String(msgstr, sx+4, sy+3);
}

void loop() {
    printboxed("test", 0, 4);
    delay(500);
    printboxedhighlight("test", 0, 4);
    delay(500);
}