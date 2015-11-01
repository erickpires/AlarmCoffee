// --- Buttons and output pins ---
#define on_button   A0
#define mode_button A1
#define up_button   A2
#define down_button A3

#define output_pin  A5

// --- LCD related pins ---
#define rs_pin     12
#define enable_pin 11
#define d4_pin      5
#define d5_pin      4
#define d6_pin      3
#define d7_pin      2


// --- System clock variables ---
// Since these variables are accessed from the main loop and from timer 1 interrupt routine, they must
// be declared as volatile.
volatile short seconds = 0;
volatile short minutes = 0;
volatile short hours = 0;

// --- LCD object ---
// Why is it an object?
// TODO: get rid of LCD library and control the display directly
#include <LiquidCrystal.h>
LiquidCrystal lcd(rs_pin, enable_pin, d4_pin, d5_pin, d6_pin, d7_pin);

void setup() {

    // --- Timer 1 initialization ---
     {
        noInterrupts();
        TCCR1A = 0;
        TCCR1B = 0;
        TCNT1 = 0;

        OCR1A = 62449;
        TCCR1B |= (1 << WGM12);
        TCCR1B |= (1 << CS12);
        TIMSK1 |= (1 << OCIE1A);
        interrupts();
    }

    // --- Input and output pins mode ---
    pinMode(on_button,   INPUT);
    pinMode(mode_button, INPUT);
    pinMode(up_button,   INPUT);
    pinMode(down_button, INPUT);
    pinMode(output_pin,  OUTPUT);

    // --- Enable input pins pull-up resistors ---
    digitalWrite(on_button,   HIGH);
    digitalWrite(mode_button, HIGH);
    digitalWrite(up_button,   HIGH);
    digitalWrite(down_button, HIGH);

    // --- LCD initialization ---
    lcd.begin(16, 2);
}

// --- Timer 1 interrupt routine ---
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
    if(hours == 24) {
        hours = 0;
    }
}

short lastSecond = -1;

char time_string[10];
char buttons_state[5] = {0};

void loop() {
    buttons_state[0] = digitalRead(on_button) ? '+' : '-';
    buttons_state[1] = digitalRead(mode_button) ? '+' : '-';
    buttons_state[2] = digitalRead(up_button) ? '+' : '-';
    buttons_state[3] = digitalRead(down_button) ? '+' : '-';

    lcd.setCursor(0, 1);
    lcd.print(buttons_state);

    if(lastSecond == seconds)
        return;


    if(!(seconds % 2))
        digitalWrite(output_pin, !digitalRead(output_pin));

    lcd.setCursor(0,0);

    sprintf(time_string, "%02d:%02d:%02d", hours, minutes, seconds);
    lcd.print(time_string);


    lastSecond = seconds;
}
