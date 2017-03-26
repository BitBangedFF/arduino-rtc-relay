#include <SparkFunDS1307RTC.h>
#include <Wire.h>


#define PIN_LED (13)
#define PIN_SQW (2)
#define PIN_R0 (3)
#define PIN_R1 (4)

#define LOOP_DELAY (200)
#define STARTUP_DELAY (2000)

#define RELAY0 (0)
#define RELAY1 (1)


typedef struct
{
    uint8_t hour_on;
    uint8_t hour_off;
} relay_schedule_s;


static const relay_schedule_s RELAY_SCHEDULES[2] =
{
    {
        .hour_on = 8,
        .hour_off = 19  
    },
    {
        .hour_on = 1,
        .hour_off = 5  
    }
};

static uint8_t last_minute = 0;


static void rtc_config(
        void )
{
    rtc.begin();

    rtc.set24Hour( true );

    rtc.writeSQW( SQW_SQUARE_1 );
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


static void r0_set(
        const bool state )
{
    digitalWrite( PIN_R0, !state );
}


static void r1_set(
        const bool state )
{
    digitalWrite( PIN_R1, !state );
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
    
   r0_set( relay_schedule_get( RELAY0, time_hour ) );
   r1_set( relay_schedule_get( RELAY1, time_hour ) );
}


void setup( void )
{
    pinMode( PIN_SQW, INPUT_PULLUP );
    pinMode( PIN_LED, OUTPUT );
    pinMode( PIN_R0, OUTPUT );
    pinMode( PIN_R1, OUTPUT );

    r0_set( true );
    r1_set( true );

    led_set( true );

    delay( STARTUP_DELAY );

    // TODO - should they be kept on for a short period to prevent
    // a quick transition on multiple resets?

    r0_set( false );
    r1_set( false );
    
    rtc_config();

    while( rtc.update() == false )
    {
        led_set( false );        
        delay( LOOP_DELAY );
    }

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
    }

    last_minute = time_minute;

    led_update();

    delay( LOOP_DELAY );
}
