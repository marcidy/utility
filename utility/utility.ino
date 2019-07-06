#define FLOW_METER 14
#define PWM_PIN 13

volatile int pwm_value = 0;
volatile int prev_time = 0;
uint8_t last_pin;
uint16_t last_time = millis();
uint16_t pulse_step = 25;
uint16_t max_width = 1024;
uint16_t min_width = 10;
uint16_t dir = 1;
uint16_t pulse_width = 0;

ICACHE_RAM_ATTR void rising()
{
    attachInterrupt(digitalPinToInterrupt(FLOW_METER), &falling, FALLING);
    prev_time = micros();
}

ICACHE_RAM_ATTR void falling() 
{
    pwm_value = micros()-prev_time;
    Serial.print(pulse_width); Serial.print(" "); Serial.println(pwm_value);
    attachInterrupt(digitalPinToInterrupt(FLOW_METER), &rising, RISING);
}

void setup() 
{
    pinMode(FLOW_METER, INPUT);
    digitalWrite(FLOW_METER, HIGH);
    Serial.begin(115200);
    attachInterrupt(digitalPinToInterrupt(FLOW_METER), &rising, RISING);
    ESP.wdtDisable();
}

void loop() 
{
    if(millis() - last_time > 330)
    {
        pulse_width += dir*pulse_step;
        analogWrite(PWM_PIN, pulse_width);
        last_time = millis();

        if (max_width - pulse_width <= pulse_step)
        {
            dir = -1;
        }
        if (pulse_width - min_width <= pulse_step)
        {
            dir = 1;
        }
    }   
}
