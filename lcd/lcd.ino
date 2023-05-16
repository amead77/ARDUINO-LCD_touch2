/*####################################################

# https://github.com/amead77/LCD_touch
# arduino touchscreen LCD is:
# https://www.amazon.co.uk/gp/product/B075CXXL1M
# uses Arduino UNO attached to LCD. 
# Python sends data to arduino, which puts them into boxes on lcd.
# LCD detects press and matches to a box, returns that box number
# Python receives that data then decides what to do with it.
#

TODO:
-implement checksum on received data
-big changes:
	change box pos to python defined
	refresh only box changed

BUGS:
-everything
-my life
-i hate this so much
-	//Fill is the problem, but worked before. wtf bitrot is this
	//attacking the cpp lib hasn't helped. LCDWIKI_KBV.cpp causes glitches when
	//trying to inspect what it's doing. Either the serial input works or I
	//see some debugging data, not both. trying to put delays in the cpp
	//causes corruption of incoming data.
	//at this point i'm ready to punt this lcd down the garden.
-I've removed all unnessisary delays and serial.println, still it 
	fails.
	Resets on trying to printboxed(), yet the other branches this works.

-OMG was it zeros as xy co-ords that was the problem? 
NO, IT'S ME, I'M THE PROBLEM IT'S ME. but also the zeros
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
	int sx = boxdata[boxnum].startx;
	int sy = boxdata[boxnum].starty;
	int ex = boxdata[boxnum].endx;
	int ey = boxdata[boxnum].endy;

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
	for (int bx=0; bx < numboxes; bx++) {
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


void cmdDefault(int ibox, String sData) {
/**
 * set box/button data
 * first 12 chars are position
 * 
*/
	boxdata[ibox].startx = sData.substring(0,3).toInt();
	//Serial.println(sData.substring(0,3));
	boxdata[ibox].starty = sData.substring(3,6).toInt();
	//Serial.println(sData.substring(3,6));
	boxdata[ibox].endx = sData.substring(6,9).toInt();
	//Serial.println(sData.substring(6,9));
	boxdata[ibox].endy = sData.substring(9,12).toInt();
	//Serial.println(sData.substring(9,12));
	boxdata[ibox].sboxdata = sData.substring(12);
	//Serial.println(sData.substring(12));
	Serial.println("<-"+sData+" # "+String(boxdata[ibox].startx)+":"+String(boxdata[ibox].starty)+":"+String(boxdata[ibox].endx)+":"+String(boxdata[ibox].endy)+">"+boxdata[ibox].sboxdata);
//	delay(10);
}


void cmdB() {
//	Serial.println("clr");
	mylcd.Fill_Screen(BLACK);
//	delay(250);
}


void cmdC() {
	/**
	 * TODO: there is no reason to update ALL boxes every time.
	 * change python side to include a box number
	 * change here to check for box number.
	 * OR, change the default switch and type structure to include a 'changed'
	 * ex: boxdata[iButton].changed = true;
	*/
	Serial.println("refresh");
//	delay(100);
	for (int ii = 0; ii < boxcount; ii++) {
		printboxed(boxdata[ii].sboxdata, ii, 4);
//		Serial.println("boxdata: "+boxdata[ii].sboxdata);
//		delay(5); //25
	}
}


void cmdD() {
	send_header();
//	delay(50);
}


void get_ser_data() {
/**
* receives data from serial
* format is: [X]yyy
* where X is either A,B,C (commands) or 0..9 (lcd box)
* where yyy is the data that comes with it
*/

	int iData = 0;
	int iButton = -1;
	String iButtType = "A";
	String sData = "";
	String bData;
	String sButton = "";
	bool isok = false;
	int chkPos;
	String chkData;
	int checks = 0;

	while (Serial.available() > 0) {
		iData = Serial.read();
		if ((iData != 10) && (iData != 13)) {
			sData += char(iData);
		} else {
			break;
		}
	} //while

	/**
	 * TODO: move checksum checks to here, break up sData string and ask resend if incorrect.
	*/

	/*
	chkPos = sData.indexOf("!");
	
	isok = (sData.length() > 2) ? true : false;
	isok = (true && (chkPos > 0)) ? true : false;
	isok = (true && (chkPos < sData.length())) ? true : false;
	*/

	isok = (sData.length() > 2) ? true : false;
	iButton = sData.indexOf("]");
	isok = (true && (iButton >= 0)) ? true : false;

	if (isok) {
//		chkData = sData.substring(0, chkPos);
//		checks = computeChecksum(chkData);
		if (iButton >= 0) {
			//boxcount = 7; //because i don't want to set how many boxes from python yet "[A]7"
			bData = sData[1];
			iButton = bData.toInt(); //cannot use sData[1].toInt() as it extracts a char, but toInt() is only avail on string.
			switch (sData[1]) {
				case 'A': //define how many boxes (can ignore for now)
					//bData = sData[3];
					//boxcount = bData.toInt();
//						Serial.println("configurator [boxes]: "+String(boxcount));
//						Serial.println("sData: "+sData);

					break;
				case 'B': //clear screen
					cmdB();
					break;
				case 'C': //refresh screen
					if (boxcount > -1) {
						cmdC();
					}
					break;
				case 'D': //send simple lcd/arduino header info for id dev.
					cmdD();
					break;
				default:
				/**
				 * not a command so must be a box data
				*/
					if (sData.length() >= 12) {
						cmdDefault(iButton, sData.substring(3));
					}
					break;
			} //switch
		} //if (iButton > 1) {
	} //if (sData.length() > 2) {
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
				printboxed(boxdata[lastpressed].sboxdata, lastpressed, 4);
			}
			debounce = 0;
			lastpressed = pressed;
			sendit = true;
		}
		if (sendit) {
			//printboxed(String(pressed), 5, 4);
			printboxedhighlight(boxdata[pressed].sboxdata, pressed, 4);
			Serial.println("B*"+leadingzero(pressed)+"$"+boxdata[pressed].sboxdata);

			/**
			 * TODO: remove this delay, find another way.
			*/
//			delay(200); //remove this, modify below to compensate

			printboxed(boxdata[pressed].sboxdata, pressed, 4);
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
	
	if (Serial.available() > 0) {
		get_ser_data();
	}  
	
	CheckButtonPress(); //refactored from here out to function

	delay(10); //no delay causes no data to be received over serial?!?!

}
