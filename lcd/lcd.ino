/*####################################################

# https://github.com/amead77/LCD_touch2
# arduino touchscreen LCD is:
# https://www.amazon.co.uk/gp/product/B075CXXL1M
# uses Arduino UNO attached to LCD. 


TODO:
-create struct data containing:
	-box number //probably not needed, array position will set
	-box text
	-box return value
	-box start x
	-box start y
	-box end x
	-box end y
-create 8 boxes on the lcd touch screen
	-load data into box from struct/text
-pressing the box on screen will check debounce, then send return value by serial.print
TODO much later:
-utilise EEPROM to store box data
	-will need to get data from serial to store in EEPROM, this time avoiding the
	nightmare that was mod-lcd and the random bitrot errors. (totally was me all along)
-implement checksum on received data


####################################################*/


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

#define boxsize 2

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
int pagecount = 7;


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
/*	for (int ii = 0; ii <= 7; ii++) {
		boxdata[ii].sboxdata = "";
		boxdata[ii].iboxnum = ii;
		boxdata[ii].startx = 0;
		boxdata[ii].starty = 0;
		boxdata[ii].endx = 0;
		boxdata[ii].endy = 0;
	}
*/
}

void printboxed(String msgstr, int boxnum) {
	int sx = boxdata[boxnum].startx;
	int sy = boxdata[boxnum].starty;
	int ex = boxdata[boxnum].endx;
	int ey = boxdata[boxnum].endy;

	Serial.println("msgstr in printboxed(): "+msgstr+"!sx:"+String(sx)+" sy:"+String(sy)+" ex:"+String(ex)+" ey:"+String(ey));
	//delay(100);
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


void printboxedhighlight(String msgstr, int boxnum) {
	int sx = boxdata[boxnum].startx+1;
	int sy = boxdata[boxnum].starty+1;
	int ex = boxdata[boxnum].endx+1;
	int ey = boxdata[boxnum].endy+1;
	mylcd.Set_Text_Back_colour(YELLOW);
    mylcd.Set_Draw_color(YELLOW);
	mylcd.Fill_Rectangle(sx,sy,ex,ey);  	
	mylcd.Set_Text_Size(boxsize);
	mylcd.Set_Text_colour(BLACK);
	mylcd.Print_String(msgstr, sx+4, sy+3);
}

String leadingzero(byte tx) {
/**
 * returns a string from byte tx, if 0..9, includes a leading zero.
 */
	String lz="0";
	if (tx < 10) {
		lz+=String(tx);
	} else
	{
		lz=String(tx);
	}
	return lz;
}

void setdisp() {
	//printboxed("Test", 3, 1);
	//printboxed(String(mylcd.Get_Display_Height()), 4);
	//printboxed(String(mylcd.Get_Display_Width()), 5);
}

void dispcoord(int xx, int yy) {
/**
 * used in debugging, prints xx,yy at location
*/
	mylcd.Set_Text_colour(GREEN);
	mylcd.Print_Number_Int(xx, 0, 200, 3, ' ',10);
	mylcd.Print_Number_Int(yy, 0, 220, 3, ' ',10);
}


int boxnum(int px, int py) {
/**
 * maps x,y co-ords to onscreen box
*/
	String buildstr;
	for (int bx=0; bx < boxcount; bx++) {
		buildstr=">"+String(bx);
		if ((px >= boxdata[bx].startx) && (px <= boxdata[bx].endx) && (py >= boxdata[bx].starty) && (py <= boxdata[bx].endy)) {
			return bx;
			break;
		}
	}
	return -1;
}


void send_header() {
/**
 * sends header info to serial port
*/
	Serial.println(header);
}


int computeChecksum(String svalue) {
/**
 * calculates a checksum by xor the string 'value'
 * returns integer
 **/	
	int checksum = 0;
	byte chkByte = 0;
	for (int x = 0; x < svalue.length(); x++) {
		chkByte = byte(svalue[x]);
		checksum ^= chkByte;
	}
	return checksum;
}




void CheckButtonPress() {
/**
 * check for LCD press, map to xy coords. if match box pos, highlight box
 */
	sendit = false;
	//-------here
	digitalWrite(13, HIGH);
	TSPoint p = ts.getPoint();
	digitalWrite(13, LOW);
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);
	//-------to here, MUST be in this order. do not move pinMode() out to setup()
	// something resets it and the screen stops working properly.
	if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
		p.x = map(p.x, TS_MINX, TS_MAXX, mylcd.Get_Display_Width(), 0);
		p.y = map(p.y, TS_MINY, TS_MAXY, mylcd.Get_Display_Height(),0);
		//dispcoord(p.x, p.y);
		//printboxnum(p.x, p.y);
		pressed=boxnum(p.x, p.y);
		if (pressed != lastpressed) {
			if (debounce > -1) {
				printboxed(boxdata[lastpressed].sboxdata, lastpressed);
			}
			debounce = 0;
			lastpressed = pressed;
			sendit = true;
		}
		if (sendit) {
			//printboxed(String(pressed), 5, 4);
			printboxedhighlight(boxdata[pressed].sboxdata, pressed);
			//Serial.println("B*"+leadingzero(pressed)+"$"+boxdata[pressed].sboxdata);

			/**
			 * TODO: remove this delay, find another way.
			*/
//			delay(200); //remove this, modify below to compensate

			printboxed(boxdata[pressed].sboxdata, pressed);
		}
	} //if p.z
}  



//###############################################################################
//# main
//###############################################################################
void loop() {
	if (firsttime) {
		//setdisp(); //only used when testing
//		Serial.println("ft");
//		delay(5);
		send_header();
		firsttime = false;
	}
	if (debounce != -1) {
		debounce++;
	}
	if (debounce > 150) { //100-150 if delay(10)
		debounce=-1;
		//printboxed(boxmsg[pressed], pressed, 4);
		lastpressed = -1;
	}
	 
	
	CheckButtonPress(); //refactored from here out to function

	delay(10); //no delay causes no data to be received over serial?!?!

}
