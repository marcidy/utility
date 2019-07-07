#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>

//
// MATTS STUFF BELOW
//
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
//
// MATTS STUFF ABOVE
//

/* Put your SSID & Password */
const char* ssid = "lenny-net";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

long interval = 200;     // websocket data interval
long lastPingMillis = 0; // stores last time clients were pinged with data


/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

uint8_t D6 = 12;
uint8_t D7 = 13;

uint8_t LED1pin = D7;
bool LED1status = LOW;

uint8_t LED2pin = D6;
bool LED2status = LOW;

uint8_t AnalogPin = A0;


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
      
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
        
    case WStype_CONNECTED: 
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        
        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
      
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      // send message to client
      // webSocket.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
      
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
  pinMode(LED1pin, OUTPUT);
  pinMode(LED2pin, OUTPUT);

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.on("/led2on", handle_led2on);
  server.on("/led2off", handle_led2off);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Websocket server started");

  pinMode(flow_meter, INPUT);
  digitalWrite(flow_meter, HIGH);
  attachInterrupt(digitalPinToInterrupt(flow_meter), &rising, RISING);
  ESP.wdtDisable();

  //
  // MATTS STUFF BELOW...
  //
  pinMode(flow_meter, INPUT);
  digitalWrite(flow_meter, HIGH);
  attachInterrupt(digitalPinToInterrupt(flow_meter), &rising, RISING);
  ESP.wdtDisable();
  //
  // ...MATTS STUFF ABOVE
  //

  Serial.println("Done");
}
void loop() {
  server.handleClient();
  if(LED1status)
  {digitalWrite(LED1pin, HIGH);}
  else
  {digitalWrite(LED1pin, LOW);}
  
  if(LED2status)
  {digitalWrite(LED2pin, HIGH);}
  else
  {digitalWrite(LED2pin, LOW);}

  webSocket.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - lastPingMillis > interval) { 
    lastPingMillis = currentMillis;   

    pinMode(D6, INPUT);
    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);
    int d6_sensor_val = analogRead(AnalogPin);
    d6_sensor_val = map(d6_sensor_val, 4, 218, 0, 50);

    pinMode(D7, INPUT);
    pinMode(D6, OUTPUT);
    digitalWrite(D6, LOW);
    int d7_sensor_val = analogRead(AnalogPin);
    d7_sensor_val = map(d7_sensor_val, 4, 218, 0, 50);

    // 
    // MATTS STUFF BELOW...
    //
    inputVal = analogRead(acmeter);

    if ( millis() - print_timer > print_delay ) {
      noInterrupts();
      Serial.print("pwm: "); 
      Serial.print(pwm_value);
      Serial.print(" - anmeter: ");
      Serial.println(inputVal);
      print_timer = millis();
      interrupts();
    }
    // 
    // ...MATTS STUFF ABOVE
    //

    String json = "{ ";
    json += "\"water\": ";
    json += d6_sensor_val;
    json += ",";
    json += "\"electric\": ";
    json += d7_sensor_val;
    json += " }";

    Serial.println(json);
    webSocket.broadcastTXT(json);
  }
}

void handle_OnConnect() {
  LED1status = LOW;
  LED2status = LOW;
  Serial.println("GPIO7 Status: OFF | GPIO6 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,LED2status)); 
}

void handle_led1on() {
  LED1status = HIGH;
  Serial.println("GPIO7 Status: ON");
  server.send(200, "text/html", SendHTML(true,LED2status)); 
}

void handle_led1off() {
  LED1status = LOW;
  Serial.println("GPIO7 Status: OFF");
  server.send(200, "text/html", SendHTML(false,LED2status)); 
}

void handle_led2on() {
  LED2status = HIGH;
  Serial.println("GPIO6 Status: ON");
  server.send(200, "text/html", SendHTML(LED1status,true)); 
}

void handle_led2off() {
  LED2status = LOW;
  Serial.println("GPIO6 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,false)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(uint8_t led1stat,uint8_t led2stat){

  #define 
  
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #1abc9c;}\n";
  ptr +=".button-on:active {background-color: #16a085;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  //ptr +="<h1>ESP8266 Web Server</h1>\n";
  //ptr +="<h3>Using Access Point(AP) Mode</h3>\n";
  ptr +="<pre id=\"out\"></pre>";
  
  //if(led1stat)
  //{ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";}
  //else
  //{ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";}

  //if(led2stat)
  //{ptr +="<p>LED2 Status: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";}
  //else
  //{ptr +="<p>LED2 Status: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";}


  ptr += "<script>";
  
  ptr += "const socket = new WebSocket(\"ws://192.168.1.1:81\");";
  ptr += "const out = document.getElementById(\"out\");";
  
  ptr += "socket.onmessage = function(event) {";
  ptr += "  console.debug(\"WebSocket message received:\", event);";
  ptr += "  out.textContent = String(event.data);";
  ptr += "}";

  ptr += "</script>";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}
