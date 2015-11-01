#include <LiquidCrystal.h>

#define ledPin 13

volatile short seconds = 0;
volatile short minutes = 0;
volatile short hours = 0;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  
  OCR1A = 62449;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();

  lcd.begin(16, 2);
}

ISR(TIMER1_COMPA_vect) {
  seconds++;
  if(seconds == 60) {
    seconds = 0;
    minutes++;
  }
  if(minutes == 60) {
    minutes = 0;
    hours++;
  }
  if(hour == 24) {
    hour = 0;
  }
}

short lastSecond = -1;

void loop() {
  if(lastSecond == seconds)
    return;

  lcd.setCursor(0,0);
  char time_string[10];
  sprintf(time_string, "%02d:%02d:%02d", hours, minutes, seconds);

  lcd.print(time_string);

  lastSecond = seconds;  
}
