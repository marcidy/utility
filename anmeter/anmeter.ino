#define flow_meter 14
#define acmeter A0

volatile uint16_t pwm_value = 0;
volatile uint16_t prev_time = 0;

uint16_t last_time = millis();
uint16_t pulse_width = 0;

uint16_t inputVal = 0;
uint16_t outputVal = 0;
int sample_f = 1/(60*4);
uint16_t avgAmp = 0;

uint16_t print_delay = 50;
uint16_t print_timer = millis();

ICACHE_RAM_ATTR void rising()
{
    attachInterrupt(digitalPinToInterrupt(flow_meter), &falling, FALLING);
    pwm_value = micros() - prev_time;
}

ICACHE_RAM_ATTR void falling()
{
    prev_time = micros();
    attachInterrupt(digitalPinToInterrupt(flow_meter), &rising, RISING);
}


void setup() {
    Serial.begin(115200);

    pinMode(flow_meter, INPUT);
    digitalWrite(flow_meter, HIGH);
    attachInterrupt(digitalPinToInterrupt(flow_meter), &rising, RISING);
    ESP.wdtDisable();

    Serial.println("Done");
}

void loop() {
    inputVal = analogRead(acmeter);

    if ( millis() - print_timer > print_delay )
    {
        noInterrupts();
        Serial.print("pwm: "); Serial.print(pwm_value);
        Serial.print(" - anmeter: ");Serial.println(inputVal);
        print_timer = millis();
        interrupts();
    }
}


