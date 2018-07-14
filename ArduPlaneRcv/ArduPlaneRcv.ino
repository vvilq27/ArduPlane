/*
  http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
  1 - GND
  2 - VCC 3.3V !!! NOT 5V
  3 - CE to Arduino pin 9
  4 - CSN to Arduino pin 10
  5 - SCK to Arduino pin 13
  6 - MOSI to Arduino pin 11
  7 - MISO to Arduino pin 12
  8 - UNUSED
  Receive a series of zeros and ones to the sender radio.
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

//comment this to disable reset
#define RADIO_RESET_ENABLE
#define DEBUG

#define CE_PIN   7
#define CSN_PIN 8
#define RESET_PIN 10

typedef struct {
  uint8_t spd;
  uint8_t srv1;
  uint8_t srv2;
} Bird_msg;

// NOTE: the "LL" at the end of the constant is "LongLong" type
const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

Servo rudder;
Servo elevator;
Servo motor;

//char num[3];
Bird_msg msg;

volatile int rstCnt;
volatile uint8_t OneSecCnt;
volatile uint8_t tim1, tim2, tim3;

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void timer2init(void){
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // presc 1024 16khz
  TIMSK2 |= (1<<TOIE2); // ovf int enable
}

// nrf uses this timer
void timer0init(void){
  TCCR0B |= (1 << CS02) | (1 << CS00); // presc 1024 16khz
  TIMSK0 |= (1<<TOIE0); // ovf int enable
}

/**************************************************************
 * WARNING if radio not connected, ardu will reset on and on
 *************************************************************/

 
/********************************
 *            SETUP
 *********************************/
void setup() {
  cli();
  
  rstCnt = 0;
  tim1 = 15;
  tim2 = 1;
  tim3 = 15;
  
  motor.attach(3);
  rudder.attach(4);//rudder
  elevator.attach(5);//elevator

  msg.spd = 4;
  msg.srv1 = 125;
  msg.srv2 = 125;

  radio.begin();
  radio.setPALevel(RF24_PA_MIN);
  radio.openReadingPipe(1, pipe);
  radio.startListening();
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);

  Serial.begin(115200);
  Serial.print("channel: " );
  Serial.println(radio.getChannel());
  Serial.print("data rate: " );
  Serial.println(radio.getDataRate());

  timer2init();
  sei();
}//--(end setup )---

/********************************
 *          MAIN LOOP
 *********************************/
void loop(){

  if ( radio.available() ) {
    // Fetch the data payload
    radio.read(&msg, sizeof(msg));
    radio.flush_rx();
  }

#ifdef DEBUG
  if(!tim2){
    Serial.print(msg.spd); Serial.print(" ");
    Serial.print(msg.srv1); Serial.print(" ");
    Serial.println(msg.srv2);
    tim2 = 1;
  }
#endif
    if(!tim3){
     #ifdef DEBUG
     Serial.println("reset");
     #endif
     
    #ifdef RADIO_RESET_ENABLE
      resetFunc();
    #endif
    tim3 = 15;
  }

  if(!tim1){
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(76);

    Serial.println("reset channel");
    Serial.print("channel: " );
    Serial.println(radio.getChannel());
    Serial.print("data rate: " );
    Serial.println(radio.getDataRate());
    tim1 = 25;
  }
  motor.write(map(msg.spd, 0, 255, 34, 180));
  rudder.write(map(msg.srv1, 255, 0, 15, 180)); //trick to reverse wing move
  elevator.write(map(msg.srv2, 255, 0, 0, 179));
}// end loop


/********************************
 *            ISR
 *********************************/
ISR(TIMER2_OVF_vect){
  //if lost signal for long enough, reset arduino
  //disable this feature for start
  #ifdef RADIO_RESET_ENABLE
  if (msg.spd == 0 && msg.srv1 == 0 && msg.srv2 == 0) {
    if (rstCnt++ > 20) {
      //        motor.write(0);
      //        rudder.write(0);
      resetFunc();
    }
  }
  #endif
  
  //hold timer until 1 sec passes
  if(OneSecCnt++ < 30)
    return;
    
  OneSecCnt = 0;
 
  uint16_t cnt;

  cnt = tim1;
  if(cnt) tim1 = --cnt;

  cnt = tim2;
  if(cnt) tim2 = --cnt;

  cnt = tim3;
  if(cnt) tim3 = --cnt;
}
