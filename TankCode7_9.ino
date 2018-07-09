/**
Preliminary Code for running the tank
Serial is USB interface
Serial1 is GPS
Serial2 is to Tank Controller board
Serial3 is XBee
t
STAHP normally works but might not, mind your feet
*/

#include <Wire.h>
#include <LSM303.h>
#include <TinyGPS.h>

#define XBeebaud 9600
#define TankController 9600

#define margin 3

LSM303 compass;

#define GPSbaud 9600
TinyGPS gps;
unsigned long cur=0;
struct GPSdata{
  float GPSLat=0;
  float GPSLon=0;
  unsigned long GPSTime=0;
  long GPSAlt=0;
  int GPSSats=-1;
};

struct point{
	float lat;
	float lon;
	bool arr;
};
GPSdata gpsInfo;
GPSdata preserve;
bool newData;
bool stream;

//--- Active Variables
point startpoint;
point destinationloc;
point pos;//my pos
float distance;
float direction;
//---

point hornbake;
point secondary;

int logicState = 1;//1:init   2:recCmD   3:nav  4:Return/end
int navState=1;//1:toTarget   2:Sample

int pointnum;//Number of points being navigated between
point* points;

int counter=0;

void motor_control(int, int);

void setup() {
  Serial.begin(9600);
  Serial1.begin(GPSbaud);
  Serial3.begin(XBeebaud);
  Serial2.begin(9600);
  Wire.begin();
  compass.init();
  compass.enableDefault();
  compass.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
  compass.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};
  Serial.println("Init:-");
  
 
hornbake.lat = 38.988075;
hornbake.lon = -76.942629;
hornbake.arr = 0;
secondary.lat = ;
secondary.lon = ;
secondary.arr = 0;
}

void loop() {  
  //Sense Time
	delay(1);  
  preserve = gpsInfo;
  while (Serial1.available()){
   if (gps.encode(Serial1.read())){
     newData = true;
	 gpsInfo = getGPS();
	 pos.lat = gpsInfo.GPSLat;
	 pos.lon = gpsInfo.GPSLon;
	 break;
   }
  }
  
  switch(logicState){
	  case 1:{//Init			
		if(millis()>=(cur+1000)){
			cur=millis();
			Serial.println(gpsInfo.GPSSats);
			if(gpsInfo.GPSSats>0){//Waits for 
				Serial.println("GPS Lock");
				Serial3.println("GPS Lock");
				Serial.println("My Location: "+String(gpsInfo.GPSLat,6) + " , " + String(gpsInfo.GPSLon,6));
				logicState=2;	
			}else{
				Serial.println("GPSerr");
				Serial3.println("GPSerr");
			}
		}
	  }break;
	  case 2:{//receive Commands // Defaulted to Hornbake center for now
			Serial3.println("to3");
			if(!hornbake.arr){
				destinationloc = hornbake;
			}else{
				destinationloc = secondary;
			}
			logicState = 3;
			Serial.println("to3");
	  }break;
	  case 3:{//Navigate
			if(gpsInfo.GPSSats<0){
				Serial.println("GPSLCKERR");
			}else{
				if(millis()>=(cur+1000)){
					cur=millis();
					if(stream){
					Serial.println((String(gpsInfo.GPSLat,6) + "," + String(gpsInfo.GPSLon,6))+" , ");
					Serial3.println((String(gpsInfo.GPSLat,6) + "," + String(gpsInfo.GPSLon,6))+" , ");
					Serial3.println(getCompass());
					Serial.println(getCompass());
					}
				}
			}
			runCommand();
			nav();
	  }break;
	  case 4:{//end mission- do once home
			//probably actuate some stuff
			stop();
			delay(50);
			stop();
			Serial3.println("Mission Done, duration");
			Serial3.println((String)(millis()/1000));
			//disable stuff
			logicState=5;
	  }break;
	  case 5:delay(100);
			runCommand();
	  break;
  }
}

void nav(){
	switch(navState){
	  case 1://toTarget
		if((distance < margin)&&(distance>0)){
			navState=2;
			stop();
		}
		turntopoint(destinationloc);
		if(distance >= margin){
			if(distance>20){
				forward(9);
				//Serial.println("Forward Fast "+(String)distance);//DELETE
			}else if(distance>10){
				forward(5);
				//Serial.println("Forward Med  "+(String)distance);//DELETE
			}else{
				forward(3);
				//Serial.println("Forward Med  "+(String)distance);//DELETE
			}
		}
	  break;
	  case 2://Sample
		Serial3.println("Mission Done");
		Serial.println("Mission done");
		logicState = 4;
	  break;
  }
}

GPSdata getGPS(){
  GPSdata gpsInfo;
  newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  unsigned long start = millis();
  
    float GPSLat, GPSLon;
    int GPSSats;
    long GPSAlt;
    unsigned long date,fix_age,GPSTime;
    gps.f_get_position(&GPSLat, &GPSLon, &fix_age);
    GPSSats = gps.satellites();
    gps.get_datetime(&date, &GPSTime, &fix_age);
    GPSAlt = gps.altitude()/100.;

    gpsInfo.GPSLat = GPSLat;
    gpsInfo.GPSLon = GPSLon;
    gpsInfo.GPSTime = GPSTime;
    gpsInfo.GPSSats = GPSSats;
    gpsInfo.GPSAlt = GPSAlt;
  
  return gpsInfo;
}

void runCommand(){
	//Serial.println("go");DELETE
	int uplink;
	String message;
	//delay(10);
	//UPLINK (GND-->Tank)
	//Intake commands to be interpreted (might not work, TEST)
	if(Serial3.available()>0){//Lets you send commands direct via USB
		uplink = Serial3.read();
		Serial.print("Serial Command: ");
	}else if(Serial.available()>0){//Over Xbee
		Serial.print("Xbee Command: ");
		uplink = Serial.read();
	}
	if(Serial2.available()>0){//Over Xbee
		Serial.print("Error Message");
		Serial3.print("Error Message");
		String mes = Serial2.readString();
		delay(50);
		Serial3.println(mes);
		Serial.println(mes);
	}
	
	//---
	//The switch (predefined passthroughs n stuff)
	switch(uplink){
		case	't': Serial.println("TEST");
						Serial3.println("TEST");
						delay(1000);break;
		
		case	'p':	message = (String(gpsInfo.GPSLat,6) + "," + String(gpsInfo.GPSLon,6));
						Serial3.println(message);
						Serial.println("P command");
						Serial.println(message);
						break;
		
		case	'r': rth();
					Serial.println("RTH");
					Serial3.println("RTH");
					break;//return to home
		
		case	'q': sethome();
					Serial.println("HomeSet");
					Serial3.println("HomeSet");
					break;//return to home
		
		case	's' : stop();
					delay(50);
					stop();
					Serial.println("STAHP");
					Serial3.println("STAHP");
					logicState = 5;
					break;

		case	'n' : Serial.println("Restart");
					Serial3.println("Restart");
					logicState = 1;
					navState=1;
					delay(500);break;

		case	'g' : Serial.println("Toggle");
					Serial3.println("Toggle");
					if(stream){
						stream = 0;
					}else{
						stream = 1;
					}
					delay(500);break;						
				
		default	: break;
	}
}

///---Nav Methods---///
void rth(){
	destinationloc = startpoint;
}
void sethome(){
	startpoint = pos;
}

///---Control Methods---///
void forward(int power){
	Serial2.write('F');
	Serial2.write(power+'0');
}

void backward(int power){
	Serial2.write('B');
	Serial2.write(power+'0');
}

void stop(){
	Serial2.write('S');
}

void right(int power){
	Serial2.write('R');
	Serial2.write(power+'0');
}
void left(int power){
	Serial2.write('L');
	Serial2.write(power+'0');
}

void turntopoint(point target){//heading-current    direction-desired
	bearing(pos.lat,pos.lon,target.lat,target.lon, distance, direction);
	float change = direction - getCompass();
	if((abs(change)>5)&&(abs(change)<355)){
		if((change>0)&&(change<=180)){
			if(change<=30){
				right(1);
			}else if(change<90){
				right(2);
			}else{
				right(6);
			}
		}else if(change>180){
			if(change<195){
				left(1);
			}else if(change<270){
				left(2);
			}else{
				left(6);
			}
		}else if((change<0)&&(change>=-180)){
			if(change>-30){
				left(1);
			}else if(change>-90){
				left(2);
			}else{
				left(6);
			}
		}else if(change<180){
			right(2);
		}
		change = direction - getCompass();
	}
}

float getCompass(){
	compass.read();
	float t1 = compass.heading();
	compass.read();
	float t2 = compass.heading();
	compass.read();
	float t3 = compass.heading();
	compass.read();
	float t4 = compass.heading();
	float heading = (t1+t2+t3+t4)/4.0;
	return heading;
}
