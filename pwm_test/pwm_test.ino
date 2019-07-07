void setup()
{
    Serial.begin(115200);
    Serial.println("setup");
    pinMode(6, OUTPUT);
}

int pulse_width = 0;
int pulse_step = 25;
int dir = 1;

void loop()
{
    if (pulse_width >= 255 - pulse_step)
    {
        dir = -1;
    }
    if (pulse_width <= 0 + pulse_step)
    {
        dir = 1;
    }
    analogWrite(6, pulse_width);
    pulse_width += dir * pulse_step;
    delay(1000);
    Serial.println(pulse_width);
}
