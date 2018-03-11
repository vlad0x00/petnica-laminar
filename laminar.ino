#include <ACPWM.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

LiquidCrystal_PCF8574 lcd(0x27);

const int ZERO_PIN = 11; //Input pin from zero cross detector
const int PWM_PIN = 12; //Output pin to TRIAC / SSR

// Enkoder i njegov taster
const int ENCODER_PIN_A = 8;
const int ENCODER_PIN_B = 9;
const int ENCODER_BUTTON_PIN = 10;
const int ENCODER_DELAY = 120;
const int ENCODER_ROTATION_DELAY = 80;
const int UV_PIN_OUT = A2;

const int LATCH_BUTTON_DELAY = 600;

int encoder_pos = 0;
int encoder_pin_A_last = LOW;
bool uv_on = false;

const int MOTOR_BUTTON_PIN = 2;
const int MOTOR_SPEEDUP_DELAY = 90;
const int MOTOR_SLOWDOWN_DELAY = 10;

bool motor_on = false;
int motor_counter = 0;

const int GLASS_BUTTON_PIN_UP = 3;
const int GLASS_BUTTON_PIN_DOWN = 4;
const int GLASS_SENSOR_PIN_UP = 5;
const int GLASS_SENSOR_PIN_DOWN = 6;
const int GLASS_MOVE_PIN_UP = A0;
const int GLASS_MOVE_PIN_DOWN = A1;

void setup() {
    lcd.setBacklight(255);
    lcd.clear();
    lcd.begin(16, 2);
    lcd.print("     Welcome to   ");
    lcd.setCursor(0, 1);
    lcd.print("Utepuvatch v1.0");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("Made by Aleksa, ");
    lcd.setCursor(0, 1);
    lcd.print("Steva & Vlado   ");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("UV timer: 0  min");

    pinMode(ENCODER_BUTTON_PIN, INPUT);
    pinMode(ENCODER_PIN_A, INPUT);
    pinMode(ENCODER_PIN_B, INPUT);
    pinMode(UV_PIN_OUT, OUTPUT);

    pinMode(GLASS_BUTTON_PIN_UP, INPUT);
    pinMode(GLASS_BUTTON_PIN_DOWN, INPUT);
    pinMode(GLASS_SENSOR_PIN_UP, INPUT);
    pinMode(GLASS_SENSOR_PIN_DOWN, INPUT);
    pinMode(GLASS_MOVE_PIN_UP, OUTPUT);
    pinMode(GLASS_MOVE_PIN_DOWN, OUTPUT);

    pinMode(MOTOR_BUTTON_PIN, INPUT_PULLUP);

    // ACPWM setup
    pinMode(ZERO_PIN, INPUT);
    pinMode(PWM_PIN, OUTPUT);
    //Initialize PWM operation.
    //Mains frequency: 50Hz.
    //Zero crossing point reached whenever pulse to PIN2 changes
    //Duty cycle = 0..255. 0:always off. 255: always on. 150: 59% on.
    ACpwm.initialize(50, ZERO_PIN, CHANGE, PWM_PIN, 255);
    //Latching on HIGH zero state: 3 microseconds.
    //Latching on LOW zero cross state: 5 microseconds.
    ACpwm.setLatch(3, 5);
}

void uv_turn_on() {
    lcd.setCursor(0, 0);
    lcd.print("UV timer:    ");
    lcd.setCursor(13, 0);
    lcd.print("min");
    lcd.setCursor(0, 1);
    lcd.print("Burn, baby, burn");
    update_encoder_num();

    digitalWrite(UV_PIN_OUT, HIGH);

    uv_on = true;
}

void uv_turn_off() {
    digitalWrite(UV_PIN_OUT, LOW);

    encoder_pos = 0;
    uv_on = false;

    update_encoder_num();
    lcd.setCursor(0, 1);
    lcd.print("UV Done         ");

    delay(2000);
}

void update_encoder_num() {
    lcd.setCursor(10, 0);
    lcd.print("   ");
    lcd.setCursor(10, 0);
    lcd.print(encoder_pos);
}

inline bool is_motor_stopped() {
    return !motor_on && motor_counter == 0;
}

void check_encoder_button() {
    if (digitalRead(ENCODER_BUTTON_PIN) == HIGH) {
        if (uv_on) {
            uv_turn_off();
        } else {
            uv_turn_on();
        }

        delay(LATCH_BUTTON_DELAY);
    }
}

inline void encoder_code() {
    int n = digitalRead(ENCODER_PIN_A);
    if ((encoder_pin_A_last == LOW) && (n == HIGH)) {
        if (digitalRead(ENCODER_PIN_B) == LOW) {
            encoder_pos--;
        } else {
            encoder_pos++;
        }

        if (encoder_pos < 0) {
            encoder_pos = 0;
        }

        delay(ENCODER_ROTATION_DELAY);
        update_encoder_num();
    }
    encoder_pin_A_last = n;

    check_encoder_button();

    if (uv_on) {
        if (encoder_pos > 0) {
            encoder_pos --;
            update_encoder_num();

            int i;
            for(i = 0; i < 60000 / ENCODER_DELAY; i++) {
                delay(ENCODER_DELAY);
                check_encoder_button();
                if (!uv_on) {
                    break;
                }
            }
        } else {
            uv_turn_off();
        }
    }
}

void update_motor_speed() {
    lcd.setCursor(12, 0);
    lcd.print("    ");
    lcd.setCursor(12, 0);
    lcd.print(motor_counter);
}

void motor_turn_on() {
    lcd.setCursor(0, 0);
    lcd.print("Motor speed:    ");
    lcd.setCursor(0, 1);
    lcd.print("Speeding up     ");
    update_motor_speed();

    motor_on = true;
}

void motor_turn_off() {
    lcd.setCursor(0, 1);
    lcd.print("Slowing down    ");
    update_motor_speed();

    motor_on = false;
    ACpwm.setDutyCycle(0);
}

inline void check_motor_button() {
    if (digitalRead(MOTOR_BUTTON_PIN) == LOW) {
        if (motor_on) {
            motor_turn_off();
        } else {
            if (motor_counter == 0) {
                motor_turn_on();
            }
        }

        delay(LATCH_BUTTON_DELAY);
    }
}

inline void motor_code() {
    check_motor_button();

    if (motor_on && motor_counter < 1024) {
        motor_counter++;
        if (motor_counter == 1024) {
            lcd.setCursor(0, 1);
            lcd.print("                ");
        }

        delay(MOTOR_SPEEDUP_DELAY);
        ACpwm.setDutyCycle(motor_counter);

        update_motor_speed();
    } else if (!motor_on && motor_counter > 0) {
        motor_counter--;
        delay(MOTOR_SLOWDOWN_DELAY);

        update_motor_speed();
    }
}

inline void glass_code() {
    bool glass_up = digitalRead(GLASS_BUTTON_PIN_UP) == HIGH;
    bool glass_down = digitalRead(GLASS_BUTTON_PIN_DOWN) == HIGH;
    digitalWrite(GLASS_MOVE_PIN_UP, (!glass_up && glass_down) ? HIGH : LOW);
    digitalWrite(GLASS_MOVE_PIN_DOWN, (glass_up != glass_down) ? HIGH : LOW);
}

inline bool is_glass_up() {
    return digitalRead(GLASS_SENSOR_PIN_UP) == HIGH;
}

inline bool is_glass_down() {
    return digitalRead(GLASS_SENSOR_PIN_DOWN) == HIGH;
}

void loop() {
    // If nothing is running, print appropriate messages
    if (is_motor_stopped() && !uv_on) {
        lcd.setCursor(0, 1);
        lcd.print("Press to start  ");

        if (encoder_pos == 0) {
            lcd.setCursor(0, 0);
            lcd.print("UV timer: 0  min");
        }
    }

    if (is_glass_up() && !uv_on) {
        motor_code();
    }

    if (!uv_on) {
        glass_code();
    }

    if (is_glass_down() && is_motor_stopped()) {
        encoder_code();
    }
}