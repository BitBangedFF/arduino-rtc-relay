#include <SparkFunDS1307RTC.h>
#include <Wire.h>


#define PIN_LED (13)
#define PIN_SQW (2)

#define LOOP_DELAY (200)


static uint8_t last_minute = 0;


static void rtc_config( void )
{
    rtc.begin();

    rtc.set24Hour( true );

    rtc.writeSQW( SQW_SQUARE_1 );
}


static void led_update( void )
{
    digitalWrite( PIN_LED, digitalRead( PIN_SQW ) );
}


void setup( void )
{
    pinMode( PIN_SQW, INPUT_PULLUP );
    pinMode( PIN_LED, OUTPUT );

    led_update();

    rtc_config();
}


void loop( void )
{
    rtc.update();

    const uint8_t time_minute = rtc.minute();

    if( time_minute != last_minute )
    {
        
    }

    last_minute = time_minute;

    led_update();

    delay( LOOP_DELAY );
}
