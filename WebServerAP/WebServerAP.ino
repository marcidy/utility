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
const char* ssid = "jay-net";  // Enter SSID here
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
  /* web-ui_managed_code */ 
  return "<!DOCTYPE html> <html>   <head>     <meta charset=\"UTF-8\">     <title>Webpack App</title>   </head>   <body>   <script type=\"text/javascript\">!function(e){var t={};function n(r){if(t[r])return t[r].exports;var o=t[r]={i:r,l:!1,exports:{}};return e[r].call(o.exports,o,o.exports,n),o.l=!0,o.exports}n.m=e,n.c=t,n.d=function(e,t,r){n.o(e,t)||Object.defineProperty(e,t,{enumerable:!0,get:r})},n.r=function(e){\"undefined\"!=typeof Symbol&&Symbol.toStringTag&&Object.defineProperty(e,Symbol.toStringTag,{value:\"Module\"}),Object.defineProperty(e,\"__esModule\",{value:!0})},n.t=function(e,t){if(1&t&&(e=n(e)),8&t)return e;if(4&t&&\"object\"==typeof e&&e&&e.__esModule)return e;var r=Object.create(null);if(n.r(r),Object.defineProperty(r,\"default\",{enumerable:!0,value:e}),2&t&&\"string\"!=typeof e)for(var o in e)n.d(r,o,function(t){return e[t]}.bind(null,o));return r},n.n=function(e){var t=e&&e.__esModule?function(){return e.default}:function(){return e};return n.d(t,\"a\",t),t},n.o=function(e,t){return Object.prototype.hasOwnProperty.call(e,t)},n.p=\"\",n(n.s=0)}([function(e,t){document.write(\"success\");const n=new WebSocket(\"ws://192.168.1.1:81\"),r=document.body;n.onmessage=function(e){console.debug(\"WebSocket message received:\",e),r.textContent=String(e.data)}}]);</script></body> </html>";
  }
