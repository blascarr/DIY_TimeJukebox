#include <Arduino.h>

#include <menu.h>
#include <menuIO/liquidCrystalOut.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/encoderIn.h>
#include <menuIO/keyIn.h>
#include <menuIO/chainStream.h>
#include <Chrono.h> 
#include <LcdProgressBar.h>

using namespace Menu;

#include <DFRobotDFPlayerMini.h>
#define FAN_PIN 12
#define bps 9600

DFRobotDFPlayerMini mp3;

// MP3 /////////////////////////////////////////
int track = 1;
int volume = 20;

// Chrono /////////////////////////////////////////
Chrono tempo;
long tick = 1000;
long timeelapsed = tick;
int addSeconds = 10;

// LCD /////////////////////////////////////////
#define RS 2
#define EN 3
LiquidCrystal lcd(RS, EN, 4, 5, 6, 7);
LcdProgressBar lpg(&lcd, 3, 20);
// Encoder /////////////////////////////////////
#define encA 8
#define encB 9

//this encoder has a button here
#define encBtn 10

encoderIn<encA,encB> encoder;//simple quad encoder driver
encoderInStream<encA,encB> encStream(encoder,4);// simple quad encoder fake Stream

//a keyboard with only one key as the encoder button
keyMap encBtn_map[]={{-encBtn,defaultNavCodes[enterCmd].ch}};//negative pin numbers use internal pull-up, this is on when low
keyIn<1> encButton(encBtn_map);//1 is the number of keys

//input from the encoder + encoder button + serial
serialIn serial(Serial);
menuIn* inputsList[]={&encStream,&encButton,&serial};
chainStream<3> in(inputsList);//3 is the number of inputs

// Schedule Tracks Definition
struct scheduletrack{
  long timestamp;
  int track;
  scheduletrack( long timeflag, int t ):timestamp(timeflag),track(t){}
};

scheduletrack scheduleList[] = {
  scheduletrack(5000,5),
  scheduletrack(30000,2),
  scheduletrack(50000,7), 
  scheduletrack(90000,9) 
};

//-------Class Clock Schedule
class clockscheduler{
  public:

    struct chrono_time{
      long h,m,s;
    };
    scheduletrack * list;
    int size;

    int current_track=0;
    
    chrono_time clock_time;
    chrono_time current_time;
    chrono_time next_time;

    typedef void ( clockscheduler::*_f_execution )( menuOut& out, LcdProgressBar& lcd_pg );
    _f_execution p_update = &clockscheduler::executePrint ;
    
    Chrono & timer;
    DFRobotDFPlayerMini & player;
    enum {clock,countDown} type=clock;
    
    clockscheduler( DFRobotDFPlayerMini &mp3, Chrono &tempo, scheduletrack * schedulerpointer, int schedulersize ):player(mp3), timer( tempo ), list( schedulerpointer ), size( schedulersize ){
      next_time.s = list[ current_track ].timestamp/1000;
    }

    void init(){
      current_track=0;
    }
    
    void update_screen( menuOut& out, LcdProgressBar& lcd_pg ){
      (this->*p_update)(out, lcd_pg);
    }

    void executePrint( menuOut& out, LcdProgressBar& lcd_pg ){
      // Print Chrono Progress
      printclock( out, 0 );

      //Paint DFPlayer Status
      statemp3( out, 2);
      
      //Paint Progress Bar
      progressbar( out, lcd_pg, 3 );


      //Hay que cambiar si el estado del sistema esta parado que no ejecute nada
      //Update tempo for activation and next track
      //Que aparezca el tiempo de final a la derecha
      if( timer.elapsed() >= list[ current_track ].timestamp ){

        //Print only when change
        if( current_track < size ){
          //If clock is extra 60 sec formt hh:mm:ss
          printNext( out, 1 );
        }else{
          
        }
      }else {
        //Print once the first track status
        printNext( out, 1 );
      }
    }
    
    void idle(  menuOut& out, LcdProgressBar& lcd_pg  ){
    
    }

    void toIdle(){
      p_update = &clockscheduler::idle;
    }
    void toWork(){
      p_update = &clockscheduler::executePrint;
    }
    
    void update( menuOut& out ){

      if( timer.elapsed() >= list[ current_track ].timestamp ){
        player.play( list[ current_track ].track );
        current_track++;
        if( current_track < size ){
          next_time.s = list[ current_track ].timestamp/1000;
        }
      }
      
      if( timer.elapsed() >= ( chronoToSeconds( clock_time )*1000 ) ){
        timer.stop();
        //Establecer todo al estado inicial
        //mainMenu[4].enabled=enabledStatus;
      }
      
    }

    void printclock( menuOut& out, int row ){
      out.setCursor( 0,row );
      current_time.s = tempo.elapsed()/1000;
      current_time.h = current_time.s/(60*60);
      current_time.s -= current_time.h*60*60;
      current_time.m = current_time.s/60;
      current_time.s -= current_time.m*60;
      
      printAlarm( out, current_time, "C: " );
    }

    void printNext( menuOut& out , int row ){
      out.setCursor( 0, row );
      printAlarm( out, next_time, "Next: " );
    }

    void statemp3( menuOut& out, int row ){
      
      if (player.available()) {
        out.setCursor( 0, row );
        out.print("                  ");
        out.setCursor( 0, row );
        int state_type = player.readType();
        char* msg;
        switch (state_type) {
          case TimeOut:
            msg = "Time Out!";
          break;
          case WrongStack:
            msg = "Stack Wrong!";
          break;
          case DFPlayerCardInserted:
            msg = "Card Inserted!";
          break;
          case DFPlayerCardRemoved:
            msg = "Card Removed!";
          break;
          case DFPlayerCardOnline:
            msg = "Card Online!";
          break;
          case DFPlayerUSBInserted:
            msg = "USB Inserted!";
          break;
          case DFPlayerUSBRemoved:
            msg = "USB Removed!";
          break;
          case DFPlayerPlayFinished:
            msg = "Song Finished!";
          break;
          case DFPlayerError:
            msg = "ERROR!";
            break;
          default:
            break;
        }
        out.print( msg );
      }
      
    }
    
    void progressbar( menuOut& out, LcdProgressBar& lcd_pg, int row ){
      out.setCursor( 0, row );
      long total_sec = chronoToSeconds( clock_time )*1000;
      int progress_value = ( ( total_sec - tempo.elapsed() )*100/total_sec);
      lcd_pg.draw( 100 - progress_value );
    }
    
    void printAlarm( menuOut& out, chrono_time ct, char* msg ){
      int len = 12;
      switch(type) {
            case clock:
              
              out.print( msg );
              len-=out.print(ct.h);
              len-=out.printRaw(F(":"),len);
              if (ct.m<10) len-=out.print("0");
              len-=out.print(ct.m);
              len-=out.printRaw(F(":"),len);
              if (ct.s<10) len-=out.print("0");
              len-=out.print(ct.s);
              break;
            // Reloj Marcha atras
            case countDown:
              out.print( msg );
              
              if (ct.h<10) len-=out.print("Next ");
              len-=out.print(ct.h);
              len-=out.printRaw(F(":"),len);
              if (ct.m<10) len-=out.print("0");
              len-=out.print(ct.m);
              len-=out.printRaw(F(":"),len);
              if (ct.s<10) len-=out.print("0");
              len-=out.print(ct.s);
              break;
      }
    }

    long chronoToSeconds( chrono_time ct ){
      return ( ct.h*3600+ct.m*60+ct.s );
    }

};

clockscheduler clocksystem( mp3 , tempo, scheduleList, sizeof( scheduleList )/sizeof( scheduletrack ) );

/////////////////////////////////////////////////////////////////////////
// MENU DEFINITION

int clockCtrl=LOW;
int fanCtrl=LOW;

result stopClock(eventMask e,navNode& nav,prompt& item) {
  tempo.stop();
  mp3.stop();
  return proceed;
}

result resumeClock(eventMask e,navNode& nav,prompt& item) {
  tempo.resume();
  return proceed;
}

result playMP3(eventMask e,navNode& nav,prompt& item) {
  //Detect SD Card
    mp3.play(track);
  
  return proceed;
}

result resetMP3(eventMask e,navNode& nav,prompt& item) {
  mp3.reset();
  return proceed;
}

result stopMP3(eventMask e,navNode& nav,prompt& item) {
  mp3.stop();
  return proceed;
}

result randomTime(eventMask e,navNode& nav,prompt& item) {
  mp3.randomAll();
  return proceed;
}

result fanEvent(eventMask e,navNode& nav,prompt& item) {
  digitalWrite(FAN_PIN, HIGH);
  return proceed;
}

bool mp3Serial(){
  return mp3.begin( Serial );
}

result setVolume (eventMask e,navNode& nav,prompt& item) {
  mp3.volume( volume );
  return proceed;
}

TOGGLE(fanCtrl,fanToggle,"FAN: ",doNothing,noEvent,noStyle
  ,VALUE("ON",HIGH,fanEvent,exitEvent)
  ,VALUE("OFF",LOW,fanEvent,exitEvent)
);

TOGGLE(clockCtrl,pauseclock,"Pause: ",doNothing,noEvent,noStyle
  ,VALUE("Pause",HIGH,stopClock,enterEvent)
  ,VALUE("Resume",LOW,resumeClock,enterEvent)
);

TOGGLE(clocksystem.type,setType,"Type: ",doNothing,noEvent,noStyle
  ,VALUE("Alarm",clockscheduler::clock,doNothing,enterEvent)
  ,VALUE("Countdown",clockscheduler::countDown,doNothing,enterEvent)
);

MENU(testMenu,"Test menu",doNothing,anyEvent,wrapStyle
  ,FIELD(track,"Play Track","",1,100,1,1,playMP3,exitEvent ,wrapStyle)
  ,OP("Stop MP3",stopMP3,exitEvent)
  ,OP("Reset MP3",resetMP3,exitEvent)
  ,OP("Random Time",randomTime,enterEvent)
  ,EXIT("<Back")
)

MENU(settingsMenu,"Settings menu",doNothing,anyEvent,wrapStyle
  ,FIELD(volume,"Volume","",0,30,5,1,setVolume,exitEvent ,wrapStyle)
  ,OP("Stop MP3",stopMP3,enterEvent)
  ,SUBMENU(setType)
  ,SUBMENU(fanToggle)
  ,EXIT("<Back")
)

//menu with list of alarms
MENU(mainMenu,"Main Menu",doNothing,noEvent,wrapStyle
  ,EXIT("Info")
  ,FIELD(clocksystem.clock_time.s,"Set Seconds","",0,59,5,1,doNothing,exitEvent,wrapStyle)
  ,FIELD(clocksystem.clock_time.m,"Set Minutes","",0,59,1,0,doNothing,exitEvent,wrapStyle)
  ,FIELD(clocksystem.clock_time.h,"Set Hours","",0,23,1,0,doNothing,exitEvent,wrapStyle)
  ,OP("START",clockStart,enterEvent)
  ,OP("STOP",clockStop,enterEvent)
  ,SUBMENU(settingsMenu)
  ,SUBMENU(testMenu)
);


result restartClock(eventMask e,navNode& nav,prompt& item) {
  tempo.restart();
  return proceed;
}

result addTime(eventMask e,navNode& nav,prompt& item) {
  tempo.add(addSeconds*1000);
  return proceed;
}

#define MAX_DEPTH 2

MENU_OUTPUTS(out, MAX_DEPTH
  ,LIQUIDCRYSTAL_OUT(lcd,{0,0,20,4})
  ,NONE
);
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);//the navigation root object

//Set static Functions for ClockScheduler Task
void clockStop() {
  tempo.stop();
  mp3.stop();
  mainMenu[4].enabled=enabledStatus;
  clocksystem.toIdle();
  fanCtrl=LOW; 
  digitalWrite( FAN_PIN, fanCtrl);
}

void clockStart() {
  tempo.restart();
  clocksystem.init();
  mainMenu[4].enabled=disabledStatus;
  nav.idleOn();
  
  fanCtrl=HIGH; 
  digitalWrite( FAN_PIN, fanCtrl);
  
  //print EndClock
  out[0].setCursor( 13, 0 );
  clocksystem.printAlarm( out[0], clocksystem.clock_time, "" );
  clocksystem.toWork();
}

//---------------- Time Events- Update ---------------
long timestamp = 100;
long oldtime = 0 ;
long debugstamp = 500;
long debugtime = 0 ;
long playtime = 0 ;
long playstamp = 500;


void setup() {
  pinMode( FAN_PIN, OUTPUT );
  Serial.begin(bps);
  while(!Serial);

  //-------------- INPUT- OUTPUT INIT ----------------
  lcd.begin(20,4);
  lpg.setMinValue(0);
  lpg.setMaxValue(100);
  
  pinMode(encBtn,INPUT_PULLUP);
  encoder.begin();

  //-------------- TIMER INITIALIZATION --------------
  tempo.stop();
  
  //-------------- MP3 INITIALIZATION ----------------
  if(  mp3Serial() ){
    Serial.println("Success");
  }else{
    Serial.println("Fail MP3");
  }
  mp3.volume(volume);
  
  Serial.println("Clock Menu");
  delay(500);
  out.clear();
  
}

void loop() {
  
    //Si se ha salido del menu , estamos en estado parado
    if(nav.sleepTask) {
      
      if( millis() - debugtime > debugstamp ){
        clocksystem.update_screen( out[0], lpg );
        debugtime = millis();
      }
    }
    
    //Define task for update songs
    if( millis() - playtime > playstamp ){
        clocksystem.update( out[0]);
        playtime = millis();
    }
    
    if( millis() - oldtime > timestamp ){
      nav.poll();
      oldtime = millis();
      //
    }

    if (Serial.available()) {
      Serial.flush();
    }

}
