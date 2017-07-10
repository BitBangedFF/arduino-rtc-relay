/**
 * @file rtc_relay.ino
 * @brief RTC Relay.
 *
 * Board: 5v Nano
 *
 */


#include <SparkFunDS1307RTC.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SerLCD.h>


#define PIN_LED (13)
#define PIN_SQW (3)
#define PIN_LCD_TX (2)
#define PIN_LCD_RX (8)
#define PIN_R0 (6)
#define PIN_R1 (7)

#define LOOP_DELAY (1000UL)
#define STARTUP_DELAY (2000UL)

#define LCD_RATE (9600U)
#define LCD_BACKLIGHT (157)

#define RELAY0 (0)
#define RELAY1 (1)
#define RELAY_COUNT (2)

#define HOUR_ALL_OFF (253)
#define HOUR_ALL_ON (254)

#define RTC_ERROR_MINUTE (165)
#define RTC_ERROR_YEAR (0)


typedef struct
{
    uint8_t pin;
    uint8_t hour_on;
    uint8_t hour_off;
} relay_schedule_s;


static const relay_schedule_s RELAY_SCHEDULES[RELAY_COUNT] =
{
    {
        // 7 am to 7 pm
        .pin = PIN_R0,
        .hour_on = 7,
        .hour_off = 19
    },
    {
        // 4 pm to 10 am
        .pin = PIN_R1,
        .hour_on = 16,
        .hour_off = 10
    }
};


static SoftwareSerial SERIAL_LCD(PIN_LCD_RX, PIN_LCD_TX);
static SerLCD LCD(SERIAL_LCD);


static void rtc_config(void)
{
    rtc.begin();
    rtc.enable();
    rtc.set24Hour(true);
    rtc.writeSQW(SQW_SQUARE_1);
}


static bool rtc_check(void)
{
    bool check_pass = true;

    if(rtc.getMinute() == RTC_ERROR_MINUTE)
    {
        check_pass = false;
    }
    else if(rtc.getYear() == RTC_ERROR_YEAR)
    {
        check_pass = false;
    }

    return check_pass;
}


static void lcd_config(void)
{
    SERIAL_LCD.begin(LCD_RATE);
    LCD.begin();
    LCD.clear();
    LCD.displayOn();
    LCD.setBacklight(LCD_BACKLIGHT);
}


static void relays_config(void)
{
    uint8_t idx;
    for(idx = 0; idx < RELAY_COUNT; idx += 1)
    {
        pinMode(RELAY_SCHEDULES[idx].pin, OUTPUT);
    }
}


static bool sqw_get(void)
{
    return (bool) digitalRead(PIN_SQW);
}


static void led_set(
        const bool state)
{
    digitalWrite(PIN_LED, state);
}


static void led_update(void)
{
    led_set(sqw_get());
}


static void relay_set(
        const uint8_t relay,
        const bool state)
{
    digitalWrite(RELAY_SCHEDULES[relay].pin, !state);
}


static void relays_set(
        const bool state)
{
    uint8_t idx;
    for(idx = 0; idx < RELAY_COUNT; idx += 1)
    {
        digitalWrite(RELAY_SCHEDULES[idx].pin, !state);
    }
}


static bool relay_schedule_get(
        const uint8_t relay,
        const uint8_t time_hour)
{
    bool enabled = false;

    if(RELAY_SCHEDULES[relay].hour_on == HOUR_ALL_OFF)
    {
        enabled = false;
    }
    else if(RELAY_SCHEDULES[relay].hour_off == HOUR_ALL_OFF)
    {
        enabled = false;
    }
    else if(RELAY_SCHEDULES[relay].hour_on == HOUR_ALL_ON)
    {
        enabled = true;
    }
    else if(RELAY_SCHEDULES[relay].hour_off == HOUR_ALL_ON)
    {
        enabled = true;
    }
    else
    {
        if(RELAY_SCHEDULES[relay].hour_on <= RELAY_SCHEDULES[relay].hour_off)
        {
            if(time_hour >= RELAY_SCHEDULES[relay].hour_on)
            {
                if(time_hour < RELAY_SCHEDULES[relay].hour_off)
                {
                    enabled = true;
                }
            }
        }
        else
        {
            // wraps over to next day
            if(time_hour < RELAY_SCHEDULES[relay].hour_off)
            {
                enabled = true;
            }
            else if(time_hour >= RELAY_SCHEDULES[relay].hour_on)
            {
                enabled = true;
            }
        }
    }

    return enabled;
}


static void relays_update(void)
{
    const uint8_t time_hour = rtc.hour();

    uint8_t idx;
    for(idx = 0; idx < RELAY_COUNT; idx += 1)
    {
        relay_set(idx, relay_schedule_get(idx, time_hour));
    }
}


static void sqw_interrupt_handler(void)
{
    led_update();
}


static void lcd_update(void)
{
    const uint8_t time_hour = rtc.hour();

    LCD.clear();
    LCD.setPosition(1, 0);
    LCD.print("R1: ");
    LCD.setPosition(1, 4);
    if(relay_schedule_get(RELAY0, time_hour) == true)
    {
        LCD.print("ON");
    }
    else
    {
        LCD.print("OFF");
    }
    LCD.setPosition(1, 9);
    LCD.print("R2: ");
    LCD.setPosition(1, 13);
    if(relay_schedule_get(RELAY1, time_hour) == true)
    {
        LCD.print("ON");
    }
    else
    {
        LCD.print("OFF");
    }
    LCD.setPosition(2, 0);
    if(rtc.minute() < 10)
    {
        const String time_str =
                (String(rtc.hour()) +
                ":0" +
                String(rtc.minute()));
        LCD.print(time_str);
    }
    else
    {
        const String time_str =
                (String(rtc.hour()) +
                ":" +
                String(rtc.minute()));
        LCD.print(time_str);
    }
    LCD.setPosition(2, 8);
    const String date_str =
            (String(rtc.month()) +
            "/" +
            String(rtc.date()) +
            "/" +
            String(rtc.year()));
    LCD.print(date_str);
}


void setup(void)
{
    pinMode(PIN_SQW, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);

    relays_config();
    relays_set(false);

    led_set(true);

    lcd_config();
    LCD.setPosition(1, 0);
    LCD.print("startup delay");

    delay(STARTUP_DELAY);

    rtc_config();

    while(rtc_check() == false)
    {
        led_set(false);
        LCD.clear();
        LCD.print("waiting for RTC");
        delay(STARTUP_DELAY);
    }

    LCD.clear();

    led_update();

    attachInterrupt(
            digitalPinToInterrupt(3),
            sqw_interrupt_handler,
            CHANGE);
}


void loop(void)
{
    static uint8_t last_minute = RTC_ERROR_MINUTE;

    rtc.update();

    const uint8_t time_minute = rtc.minute();

    if(time_minute != last_minute)
    {
        relays_update();
        lcd_update();
    }

    last_minute = time_minute;

    delay(LOOP_DELAY);
}
