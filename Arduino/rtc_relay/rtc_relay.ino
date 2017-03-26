#include <SparkFunDS1307RTC.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SerLCD.h>


#define PIN_LED (13)
#define PIN_SQW (4)
#define PIN_LCD_TX (2)
#define PIN_LCD_RX (3)
#define PIN_R0 (6)
#define PIN_R1 (7)

#define LOOP_DELAY (200U)
#define STARTUP_DELAY (2000U)

#define LCD_RATE (9600U)
#define LCD_BACKLIGHT (157)

#define RELAY0 (0)
#define RELAY1 (1)
#define RELAY_COUNT (2)


typedef struct
{
    uint8_t pin;
    uint8_t hour_on;
    uint8_t hour_off;    
} relay_schedule_s;


static const relay_schedule_s RELAY_SCHEDULES[RELAY_COUNT] =
{
    {
        .pin = PIN_R0,
        .hour_on = 8,
        .hour_off = 19
    },
    {
        .pin = PIN_R1,
        .hour_on = 1,
        .hour_off = 15  
    }
};

static uint8_t last_minute = 0;

SoftwareSerial SERIAL_LCD( PIN_LCD_RX, PIN_LCD_TX );
SerLCD LCD( SERIAL_LCD );


static void rtc_config(
        void )
{
    rtc.begin();
    rtc.set24Hour( true );
    rtc.writeSQW( SQW_SQUARE_1 );
}


static void lcd_config(
        void )
{
    SERIAL_LCD.begin( LCD_RATE );
    LCD.begin();    
    LCD.clear();
    LCD.displayOn();
    LCD.setBacklight( LCD_BACKLIGHT );
}


static void relays_config(
        void )
{
    uint8_t idx;
    for( idx = 0; idx < RELAY_COUNT; idx += 1 )
    {
        pinMode( RELAY_SCHEDULES[idx].pin, OUTPUT );
    }
}


static bool sqw_get(
        void )
{
    return (bool) digitalRead( PIN_SQW );
}


static void led_set(
        const bool state )
{
    digitalWrite( PIN_LED, state );
}


static void led_update(
        void )
{
    led_set( sqw_get() );
}


static void relay_set(
        const uint8_t relay,
        const bool state )
{
    digitalWrite( RELAY_SCHEDULES[relay].pin, !state );
}


static void relays_set(
        const bool state )
{
    uint8_t idx;
    for( idx = 0; idx < RELAY_COUNT; idx += 1 )
    {
        digitalWrite( RELAY_SCHEDULES[idx].pin, !state );
    }
}


static bool relay_schedule_get(
        const uint8_t relay,
        const uint8_t time_hour )
{
    bool enabled = false;

    if( time_hour >= RELAY_SCHEDULES[relay].hour_on )
    {
        if( time_hour < RELAY_SCHEDULES[relay].hour_off )
        {
            enabled = true;
        }
    }    

    return enabled;
}


static void relays_update(
        void )
{
    const uint8_t time_hour = rtc.hour();

    uint8_t idx;
    for( idx = 0; idx < RELAY_COUNT; idx += 1 )
    {
        relay_set( idx, relay_schedule_get( idx, time_hour ) );
    }
}


static void lcd_update(
        void )
{
    const uint8_t time_hour = rtc.hour();

    LCD.clear();
    LCD.setPosition( 1, 0 );
    LCD.print( "R0: " );
    LCD.setPosition( 1, 4 );
    if( relay_schedule_get( RELAY0, time_hour ) == true )
    {
        LCD.print( "ON" );
    }
    else
    {
        LCD.print( "OFF" );
    }
    LCD.setPosition( 1, 9 );
    LCD.print( "R1: " );
    LCD.setPosition( 1, 13 );
    if( relay_schedule_get( RELAY1, time_hour ) == true )
    {
        LCD.print( "ON" );
    }
    else
    {
        LCD.print( "OFF" );
    }
    LCD.setPosition( 2, 0 );
    if( rtc.minute() < 10 )
    {
        const String time_str =
                (String(rtc.hour()) +
                ":0" +
                String(rtc.minute()));
        LCD.print( time_str );
    }
    else
    {
        const String time_str =
                (String(rtc.hour()) +
                ":" +
                String(rtc.minute()));
        LCD.print( time_str );
    }    
    LCD.setPosition( 2, 8 );
    const String date_str =
            (String(rtc.month()) +
            "/" +
            String(rtc.date()) +
            "/" +
            String(rtc.year()));
    LCD.print( date_str );
}


void setup( void )
{
    pinMode( PIN_SQW, INPUT_PULLUP );
    pinMode( PIN_LED, OUTPUT );

    relays_config();
    relays_set( true );

    led_set( true );

    lcd_config();
    LCD.print( "startup delay" );

    delay( STARTUP_DELAY );

    // TODO - should they be kept on for a short period to prevent
    // a quick transition on multiple resets?

    relays_set( false );
        
    rtc_config();

    while( rtc.update() == false )
    {
        led_set( false );
        LCD.clear();
        LCD.print( "waiting for RTC" );
        delay( LOOP_DELAY );
    }

    LCD.clear();

    led_update();
}


void loop( void )
{
    led_update();
    
    rtc.update();
    
    const uint8_t time_minute = rtc.minute();

    if( time_minute != last_minute )
    {
        relays_update();
        lcd_update();
    }

    last_minute = time_minute;

    led_update();

    delay( LOOP_DELAY );
}
