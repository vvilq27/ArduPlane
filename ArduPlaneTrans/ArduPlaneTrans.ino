#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

#define ADC_PORT C
#define PIN_PORT(x) sPIN_PORT(x)
#define sPIN_PORT(x) (DDR##x)

typedef struct{
   uint8_t spd;
   uint8_t srv1;
   uint8_t srv2;
} Bird_msg;

RF24 radio(7, 8); // CE, CSN
const uint64_t pipe = 0xE8E8F0F0E1LL;
volatile uint8_t intCnt = 0;
Bird_msg msg;

void timer2init(void){
  TCCR2B |= (1 << CS22) | (1 << CS21)| (1 << CS20); // presc 1024; f = 16khz
  TIMSK2 |= (1<<TOIE2); // ovf int enable //int f=60hz
}

void adc_init(int channels){
  ADMUX = 0;
  ADMUX |= ( 1<<REFS0 ) | (1<<ADLAR);//Vref - Vcc, left adjust result
  ADCSRA = 0;
  ADCSRA |= ( 1<<ADEN )|( 1<<ADPS0 )|( 1<<ADPS1 ); //ADC on, 2 prescaler
  //pomiary na kanale A0
  PIN_PORT(ADC_PORT) &= ~(1<<channels);
}

inline void meas(uint8_t channel){
  ADMUX = (ADMUX & 0xF8)|channel;
  ADCSRA |= (1<<ADSC);// measure start
  while( ADCSRA & (1<<ADSC) ); // wait for meas to complete
}

void setup() {
  Serial.begin(115200);
  adc_init(0);
  timer2init();
  sei();
  
  radio.begin();
  radio.openWritingPipe(pipe);
  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_250KBPS);
  radio.printDetails();
  radio.stopListening();
  Serial.print("channel: " );
  Serial.println(radio.getChannel());
    Serial.print("data rate: " );
  Serial.println(radio.getDataRate());
}
void loop() {
}

//without if block, nrf for some reason sends only single packet and stops
//to make it work faster maybe changing interrupt freq will help
ISR(TIMER2_OVF_vect){
  if(intCnt++ < 10)
    return;
  intCnt = 0;

  meas(0);
  meas(0);
  msg.spd = ADCH;

  meas(1);
  meas(1);
  msg.srv1 = ADCH;
  
  meas(2);
  meas(2);
  msg.srv2 = ADCH;
 
  radio.writeFast(&msg, sizeof(msg));

  Serial.print(msg.spd);Serial.print(" ");
  Serial.print(msg.srv1);Serial.print(" ");
  Serial.println(msg.srv2);
}
