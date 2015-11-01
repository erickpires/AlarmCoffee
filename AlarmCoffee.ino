// --- Variable types defined by length ---
typedef char int8;
typedef byte uint8;
typedef short int16;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;

// --- The input buttons are stored sequentially on the PORTC
#define BUTTONS_PORT PINC

// --- Masks for bitwise operations
#define ON_BUTTON_MASK   0x01
#define MODE_BUTTON_MASK 0x02
#define UP_BUTTON_MASK   0x04
#define DOWN_BUTTON_MASK 0x08

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

// --- Adjust mode related definitions ---
#define EDIT_ALARM_ON 0
#define EDIT_SECONDS  0
#define EDIT_MINUTES  1
#define EDIT_HOURS    2

// --- Stay on time related definitions ---
#define DEFAULT_STAY_ON_TIME 15
#define STAY_ON_UPPER_LIMIT  30
#define STAY_ON_LOWER_LIMIT   5

// --- Modes related definitions ---
#define CLOCK_MODE          0
#define ADJUST_MODE         1
#define ADJUST_ALARM_MODE   2
#define ADJUST_STAY_ON_MODE 3
#define MODE_COUNT          4

#define update_lcd_title() {lcd.setCursor(0,0); lcd.print(mode_titles[mode_state]);}
// --- Modes titles ---

static char* mode_titles[MODE_COUNT] =
// README: Workaround to clear the entire line when the title is changed
{   "Clock",
    "Adjust time",
    "Adjust alarm",
    "Stay on for"};

// --- System clock variables ---
// Since these variables are accessed from the main loop and from timer 1 interrupt routine, they must
// be declared as volatile.
volatile int8 seconds = 0;
volatile int8 minutes = 0;
volatile int8 hours = 0;


uint8 alarm_minutes = 0;
uint8 alarm_hours = 6;
uint8 is_alarm_on = LOW;

uint8 output_state = LOW;
uint8 mode_state = CLOCK_MODE;


uint8 editing_position;
uint8 adjust_seconds;
uint8 adjust_minutes;
uint8 adjust_hours;

uint8 stay_on_time = DEFAULT_STAY_ON_TIME;

uint8 has_been_edited = LOW;
uint8 needs_redisplay = LOW;
uint8 needs_update_clock = LOW;

// --- LCD object ---
// Why is it an object?
// TODO: get rid of LCD library and control the display directly
#include <LiquidCrystal.h>
LiquidCrystal lcd(rs_pin, enable_pin, d4_pin, d5_pin, d6_pin, d7_pin);

// TODO: This variable needs a better name;
char time_string[10] = {0};
void update_clock_display() {
    sprintf(time_string, "%02d:%02d:%02d", hours, minutes, seconds);
    lcd.setCursor(4,1);
    lcd.print(time_string);
}

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
    update_lcd_title();
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

// --- Variables used to remember the last state ---
int8 last_second = -1;
// The states of all the buttons are stored in the same variable.
// Since we are interested in falling-edge changes on the button state
// these values are initialized to HIGH
uint8 last_buttons_state = 0x0f;

void adjust_editing_position(uint8 go_up) {
    int8 increment = go_up ? 1 : -1;

    switch (editing_position) {
    case EDIT_SECONDS :
        adjust_seconds = (adjust_seconds + increment + 60) % 60;
        break;
    case EDIT_MINUTES :
        adjust_minutes = (adjust_minutes + increment + 60) % 60;
        break;
    case EDIT_HOURS :
        adjust_hours = (adjust_hours + increment + 24) % 24;
        break;
    }
}

void adjust_alarm_time(uint8 go_up) {
    int8 increment = go_up ? 1 : -1;

    switch (editing_position) {
    case EDIT_ALARM_ON :
        is_alarm_on = !is_alarm_on;
        break;
    case EDIT_MINUTES :
        alarm_minutes = (alarm_minutes + increment + 60) % 60;
        break;
    case EDIT_HOURS :
        alarm_hours = (alarm_hours + increment + 60) % 60;
        break;
    }
}

void adjust_stay_on_time(uint8 go_up) {
    if(go_up) {
        if(stay_on_time < STAY_ON_UPPER_LIMIT) stay_on_time++;
    }
    else {
        if(stay_on_time > STAY_ON_LOWER_LIMIT) stay_on_time--;
    }
}

void loop() {
    // --- Variable to store the current state of the buttons ---
    uint8 buttons_state = BUTTONS_PORT;
    // We need just four input pins.
    buttons_state &= 0x0f;

    // The current state and the previous state are XOR'd together.
    // This will give us the difference between the two states
    uint8 buttons_difference = last_buttons_state ^ buttons_state;
    uint8 falling_edges = buttons_difference & last_buttons_state; // Little bit of bitwise magic

    if(buttons_difference) {
        //Poorly made debouncing
        delay(20);
    }

    if(falling_edges) {
        if(buttons_difference & ON_BUTTON_MASK) {
            switch(mode_state) {
            case CLOCK_MODE :
                output_state = !output_state;
                break;
            case ADJUST_MODE : // fall-through
            case ADJUST_ALARM_MODE :
                editing_position = (editing_position + 1) % 3; //TODO: Magic number found!!!!
                break;
            default :
                break;
            }
        }

        if(buttons_difference & MODE_BUTTON_MASK) {
            mode_state = (mode_state + 1) % MODE_COUNT;
            // The mode has changed. The display is cleared and will be filled latter.
            lcd.clear();
            needs_redisplay = HIGH;
            update_lcd_title();

            switch (mode_state) {
            case CLOCK_MODE :
                break;
            case ADJUST_MODE :
                // Reset the adjust related variables
                editing_position = EDIT_SECONDS;
                adjust_seconds = seconds;
                adjust_minutes = minutes;
                adjust_hours = hours;
                break;
            case ADJUST_ALARM_MODE :
                // We have adjusted the clock, it needs to be updated!!!
                if(needs_update_clock) {
                    seconds = adjust_seconds;
                    minutes = adjust_minutes;
                    hours = adjust_hours;
                    needs_update_clock = LOW;
                }

                editing_position = EDIT_ALARM_ON;
                break;
            case ADJUST_STAY_ON_MODE :
                break;
            default :
                break;
            }
        }

        // Both the up and down buttons have the same behavior.
        // In the case of both buttons been pressed at the same time, only the up button is considered
        if(buttons_difference & (UP_BUTTON_MASK | DOWN_BUTTON_MASK)) {
            uint8 is_up_pressed = buttons_difference & UP_BUTTON_MASK;
            has_been_edited = HIGH;

            switch (mode_state) {
            case ADJUST_MODE :
                needs_update_clock = HIGH;
                adjust_editing_position(is_up_pressed);
                break;
            case ADJUST_ALARM_MODE :
                adjust_alarm_time(is_up_pressed);
                break;
            case ADJUST_STAY_ON_MODE :
                adjust_stay_on_time(is_up_pressed);
                break;
            default:
                break;
            }
        }
    }

    // Since we already have the difference between states, we can now update this variable.
    last_buttons_state = buttons_state;

    digitalWrite(output_pin, output_state);

    // --- Setting the second display line based on the current mode ---
    switch (mode_state) {
    case CLOCK_MODE:
        // The display is updated only when the time has changed
        if(last_second != seconds || needs_redisplay) {
            update_clock_display();
            last_second = seconds;
        }
        break;
    case ADJUST_MODE :
        // The display
        if(has_been_edited || needs_redisplay) {
            sprintf(time_string, "%02d:%02d:%02d", adjust_hours, adjust_minutes, adjust_seconds);
            lcd.setCursor(4,1);
            lcd.print(time_string);
        }
        break;
    case ADJUST_ALARM_MODE :
        if(has_been_edited || needs_redisplay) {
            sprintf(time_string, "%02d:%02d", alarm_hours, alarm_minutes);
            char alarm_on_char = is_alarm_on ? 'i' : 'o';
            lcd.setCursor(5, 1);
            lcd.print(time_string);
            lcd.setCursor(14, 1);
            lcd.print(alarm_on_char);
        }
        break;
    case ADJUST_STAY_ON_MODE :
        if(has_been_edited || needs_redisplay) {
            sprintf(time_string, "%02d min", stay_on_time);
            lcd.setCursor(0,1);
            lcd.print(time_string);
        }
    default:
        break;
    }

    //The display has already been redisplayed.
    needs_redisplay = LOW;
    has_been_edited = LOW;
}
