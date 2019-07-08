#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>

#define flow_meter 12
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

/* Put your SSID & Password */
const char* ssid = "Silicon Cali";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

long interval = 20;     // websocket data interval
long lastPingMillis = 0; // stores last time clients were pinged with data

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


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

  

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Websocket server started");

  pinMode(acmeter, INPUT);
  
  pinMode(flow_meter, INPUT);
  digitalWrite(flow_meter, HIGH);
  attachInterrupt(digitalPinToInterrupt(flow_meter), &rising, RISING);
  ESP.wdtDisable(); 

  Serial.println("Done with `setup`");
}
void loop() {
  server.handleClient();
  webSocket.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - lastPingMillis > interval) { 
    lastPingMillis = currentMillis;   

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
    json += "\"pwm\": ";
    json += pwm_value;
    json += ",";
    json += "\"anmeter\": ";
    json += inputVal;
    json += " }";

    Serial.println(json);
    webSocket.broadcastTXT(json);
  }
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML()); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(){
  /* web-ui_managed_code */

return "<!DOCTYPE html> <html>   <head>     <meta charset=\"utf-8\"/>     <meta name=\"viewport\" content=\"width=device-width,minimum-scale=1,initial-scale=1,maximum-scale=2.0,user-scalable=2\">     <title>anmeter</title> <style>   body {      background-color: black;    color: white;     margin: 0;    padding: 0;   }   pre { text-align: center; } </style>   </head>   <body>     <h1>Silicon Cali Util Boss</h1>     <pre id=\"raw-websocket\"></pre>    <canvas id=\"water_canvas\" width=\"400\" height=\"150\"></canvas>    <canvas id=\"power_canvas\" width=\"400\" height=\"150\"></canvas>   <script type=\"text/javascript\">!function(t){var e={};function i(s){if(e[s])return e[s].exports;var a=e[s]={i:s,l:!1,exports:{}};return t[s].call(a.exports,a,a.exports,i),a.l=!0,a.exports}i.m=t,i.c=e,i.d=function(t,e,s){i.o(t,e)||Object.defineProperty(t,e,{enumerable:!0,get:s})},i.r=function(t){\"undefined\"!=typeof Symbol&&Symbol.toStringTag&&Object.defineProperty(t,Symbol.toStringTag,{value:\"Module\"}),Object.defineProperty(t,\"__esModule\",{value:!0})},i.t=function(t,e){if(1&e&&(t=i(t)),8&e)return t;if(4&e&&\"object\"==typeof t&&t&&t.__esModule)return t;var s=Object.create(null);if(i.r(s),Object.defineProperty(s,\"default\",{enumerable:!0,value:t}),2&e&&\"string\"!=typeof t)for(var a in t)i.d(s,a,function(e){return t[e]}.bind(null,a));return s},i.n=function(t){var e=t&&t.__esModule?function(){return t.default}:function(){return t};return i.d(e,\"a\",e),e},i.o=function(t,e){return Object.prototype.hasOwnProperty.call(t,e)},i.p=\"\",i(i.s=0)}([function(t,e,i){const{SmoothieChart:s,TimeSeries:a}=i(1),n=document.getElementById(\"raw-websocket\"),o=new WebSocket(\"ws://192.168.1.1:81\"),r=document.getElementById(\"water_canvas\"),l=document.getElementById(\"power_canvas\"),h=new s({labels:{disabled:!0}}),u=new s({labels:{disabled:!0}}),d=new a,m=new a;u.addTimeSeries(m,{lineWidth:2,strokeStyle:\"#ffff00\"}),u.streamTo(r,500),h.addTimeSeries(d,{lineWidth:2,strokeStyle:\"#0000ff\"}),h.streamTo(l,500),r.setAttribute(\"width\",window.innerWidth),l.setAttribute(\"width\",window.innerWidth);let c=0,p=0;o.onmessage=function(t){n.textContent=String(t.data);let e={};try{e=JSON.parse(t.data)}catch(t){return}const i=e.pwm,s=e.anmeter,a=(new Date).getTime();Math.abs(c-i)>1&&d.append(a,Math.abs(i)),Math.abs(p-s)>5&&m.append(a,Math.abs(s)),c=i,p=s}},function(t,e,i){!function(t){Date.now=Date.now||function(){return(new Date).getTime()};var e={extend:function(){arguments[0]=arguments[0]||{};for(var t=1;t<arguments.length;t++)for(var i in arguments[t])arguments[t].hasOwnProperty(i)&&(\"object\"==typeof arguments[t][i]?arguments[t][i]instanceof Array?arguments[0][i]=arguments[t][i]:arguments[0][i]=e.extend(arguments[0][i],arguments[t][i]):arguments[0][i]=arguments[t][i]);return arguments[0]},binarySearch:function(t,e){for(var i=0,s=t.length;i<s;){var a=i+s>>1;e<t[a][0]?s=a:i=a+1}return i}};function i(t){this.options=e.extend({},i.defaultOptions,t),this.disabled=!1,this.clear()}function s(t){this.options=e.extend({},s.defaultChartOptions,t),this.seriesSet=[],this.currentValueRange=1,this.currentVisMinValue=0,this.lastRenderTimeMillis=0,this.lastChartTimestamp=0,this.mousemove=this.mousemove.bind(this),this.mouseout=this.mouseout.bind(this)}i.defaultOptions={resetBoundsInterval:3e3,resetBounds:!0},i.prototype.clear=function(){this.data=[],this.maxValue=Number.NaN,this.minValue=Number.NaN},i.prototype.resetBounds=function(){if(this.data.length){this.maxValue=this.data[0][1],this.minValue=this.data[0][1];for(var t=1;t<this.data.length;t++){var e=this.data[t][1];e>this.maxValue&&(this.maxValue=e),e<this.minValue&&(this.minValue=e)}}else this.maxValue=Number.NaN,this.minValue=Number.NaN},i.prototype.append=function(t,e,i){for(var s=this.data.length-1;s>=0&&this.data[s][0]>t;)s--;-1===s?this.data.splice(0,0,[t,e]):this.data.length>0&&this.data[s][0]===t?i?(this.data[s][1]+=e,e=this.data[s][1]):this.data[s][1]=e:s<this.data.length-1?this.data.splice(s+1,0,[t,e]):this.data.push([t,e]),this.maxValue=isNaN(this.maxValue)?e:Math.max(this.maxValue,e),this.minValue=isNaN(this.minValue)?e:Math.min(this.minValue,e)},i.prototype.dropOldData=function(t,e){for(var i=0;this.data.length-i>=e&&this.data[i+1][0]<t;)i++;0!==i&&this.data.splice(0,i)},s.tooltipFormatter=function(t,e){for(var i=[(this.options.timestampFormatter||s.timeFormatter)(new Date(t))],a=0;a<e.length;++a)i.push('<span style=\"color:'+e[a].series.options.strokeStyle+'\">'+this.options.yMaxFormatter(e[a].value,this.options.labels.precision)+\"</span>\");return i.join(\"<br>\")},s.defaultChartOptions={millisPerPixel:20,enableDpiScaling:!0,yMinFormatter:function(t,e){return parseFloat(t).toFixed(e)},yMaxFormatter:function(t,e){return parseFloat(t).toFixed(e)},yIntermediateFormatter:function(t,e){return parseFloat(t).toFixed(e)},maxValueScale:1,minValueScale:1,interpolation:\"bezier\",scaleSmoothing:.125,maxDataSetLength:2,scrollBackwards:!1,displayDataFromPercentile:1,grid:{fillStyle:\"#000000\",strokeStyle:\"#777777\",lineWidth:1,sharpLines:!1,millisPerLine:1e3,verticalSections:2,borderVisible:!0},labels:{fillStyle:\"#ffffff\",disabled:!1,fontSize:10,fontFamily:\"monospace\",precision:2,showIntermediateLabels:!1,intermediateLabelSameAxis:!0},horizontalLines:[],tooltip:!1,tooltipLine:{lineWidth:1,strokeStyle:\"#BBBBBB\"},tooltipFormatter:s.tooltipFormatter,nonRealtimeData:!1,responsive:!1,limitFPS:0},s.AnimateCompatibility={requestAnimationFrame:function(t,e){return(window.requestAnimationFrame||window.webkitRequestAnimationFrame||window.mozRequestAnimationFrame||window.oRequestAnimationFrame||window.msRequestAnimationFrame||function(t){return window.setTimeout(function(){t(Date.now())},16)}).call(window,t,e)},cancelAnimationFrame:function(t){return(window.cancelAnimationFrame||function(t){clearTimeout(t)}).call(window,t)}},s.defaultSeriesPresentationOptions={lineWidth:1,strokeStyle:\"#ffffff\"},s.prototype.addTimeSeries=function(t,i){this.seriesSet.push({timeSeries:t,options:e.extend({},s.defaultSeriesPresentationOptions,i)}),t.options.resetBounds&&t.options.resetBoundsInterval>0&&(t.resetBoundsTimerId=setInterval(function(){t.resetBounds()},t.options.resetBoundsInterval))},s.prototype.removeTimeSeries=function(t){for(var e=this.seriesSet.length,i=0;i<e;i++)if(this.seriesSet[i].timeSeries===t){this.seriesSet.splice(i,1);break}t.resetBoundsTimerId&&clearInterval(t.resetBoundsTimerId)},s.prototype.getTimeSeriesOptions=function(t){for(var e=this.seriesSet.length,i=0;i<e;i++)if(this.seriesSet[i].timeSeries===t)return this.seriesSet[i].options},s.prototype.bringToFront=function(t){for(var e=this.seriesSet.length,i=0;i<e;i++)if(this.seriesSet[i].timeSeries===t){var s=this.seriesSet.splice(i,1);this.seriesSet.push(s[0]);break}},s.prototype.streamTo=function(t,e){this.canvas=t,this.delay=e,this.start()},s.prototype.getTooltipEl=function(){return this.tooltipEl||(this.tooltipEl=document.createElement(\"div\"),this.tooltipEl.className=\"smoothie-chart-tooltip\",this.tooltipEl.style.position=\"absolute\",this.tooltipEl.style.display=\"none\",document.body.appendChild(this.tooltipEl)),this.tooltipEl},s.prototype.updateTooltip=function(){var t=this.getTooltipEl();if(this.mouseover&&this.options.tooltip){for(var i=this.lastChartTimestamp,s=this.options.scrollBackwards?i-this.mouseX*this.options.millisPerPixel:i-(this.canvas.offsetWidth-this.mouseX)*this.options.millisPerPixel,a=[],n=0;n<this.seriesSet.length;n++){var o=this.seriesSet[n].timeSeries;if(!o.disabled){var r=e.binarySearch(o.data,s);r>0&&r<o.data.length&&a.push({series:this.seriesSet[n],index:r,value:o.data[r][1]})}}a.length?(t.innerHTML=this.options.tooltipFormatter.call(this,s,a),t.style.display=\"block\"):t.style.display=\"none\"}else t.style.display=\"none\"},s.prototype.mousemove=function(t){this.mouseover=!0,this.mouseX=t.offsetX,this.mouseY=t.offsetY,this.mousePageX=t.pageX,this.mousePageY=t.pageY;var e=this.getTooltipEl();e.style.top=Math.round(this.mousePageY)+\"px\",e.style.left=Math.round(this.mousePageX)+\"px\",this.updateTooltip()},s.prototype.mouseout=function(){this.mouseover=!1,this.mouseX=this.mouseY=-1,this.tooltipEl&&(this.tooltipEl.style.display=\"none\")},s.prototype.resize=function(){var t,e,i=this.options.enableDpiScaling&&window?window.devicePixelRatio:1;this.options.responsive?(t=this.canvas.offsetWidth,e=this.canvas.offsetHeight,t!==this.lastWidth&&(this.lastWidth=t,this.canvas.setAttribute(\"width\",Math.floor(t*i).toString()),this.canvas.getContext(\"2d\").scale(i,i)),e!==this.lastHeight&&(this.lastHeight=e,this.canvas.setAttribute(\"height\",Math.floor(e*i).toString()),this.canvas.getContext(\"2d\").scale(i,i))):1!==i&&(t=parseInt(this.canvas.getAttribute(\"width\")),e=parseInt(this.canvas.getAttribute(\"height\")),this.originalWidth&&Math.floor(this.originalWidth*i)===t||(this.originalWidth=t,this.canvas.setAttribute(\"width\",Math.floor(t*i).toString()),this.canvas.style.width=t+\"px\",this.canvas.getContext(\"2d\").scale(i,i)),this.originalHeight&&Math.floor(this.originalHeight*i)===e||(this.originalHeight=e,this.canvas.setAttribute(\"height\",Math.floor(e*i).toString()),this.canvas.style.height=e+\"px\",this.canvas.getContext(\"2d\").scale(i,i)))},s.prototype.start=function(){if(!this.frame){this.canvas.addEventListener(\"mousemove\",this.mousemove),this.canvas.addEventListener(\"mouseout\",this.mouseout);var t=function(){this.frame=s.AnimateCompatibility.requestAnimationFrame(function(){if(this.options.nonRealtimeData){var e=new Date(0),i=this.seriesSet.reduce(function(t,e){var i=e.timeSeries.data,s=Math.round(this.options.displayDataFromPercentile*i.length)-1;if(s=(s=s>=0?s:0)<=i.length-1?s:i.length-1,i&&i.length>0){var a=i[s][0];t=t>a?t:a}return t}.bind(this),e);this.render(this.canvas,i>e?i:null)}else this.render();t()}.bind(this))}.bind(this);t()}},s.prototype.stop=function(){this.frame&&(s.AnimateCompatibility.cancelAnimationFrame(this.frame),delete this.frame,this.canvas.removeEventListener(\"mousemove\",this.mousemove),this.canvas.removeEventListener(\"mouseout\",this.mouseout))},s.prototype.updateValueRange=function(){for(var t=this.options,e=Number.NaN,i=Number.NaN,s=0;s<this.seriesSet.length;s++){var a=this.seriesSet[s].timeSeries;a.disabled||(isNaN(a.maxValue)||(e=isNaN(e)?a.maxValue:Math.max(e,a.maxValue)),isNaN(a.minValue)||(i=isNaN(i)?a.minValue:Math.min(i,a.minValue)))}if(null!=t.maxValue?e=t.maxValue:e*=t.maxValueScale,null!=t.minValue?i=t.minValue:i-=Math.abs(i*t.minValueScale-i),this.options.yRangeFunction){var n=this.options.yRangeFunction({min:i,max:e});i=n.min,e=n.max}if(!isNaN(e)&&!isNaN(i)){var o=e-i-this.currentValueRange,r=i-this.currentVisMinValue;this.isAnimatingScale=Math.abs(o)>.1||Math.abs(r)>.1,this.currentValueRange+=t.scaleSmoothing*o,this.currentVisMinValue+=t.scaleSmoothing*r}this.valueRange={min:i,max:e}},s.prototype.render=function(t,e){var i=Date.now();if(!(this.options.limitFPS>0&&i-this.lastRenderTimeMillis<1e3/this.options.limitFPS)){if(!this.isAnimatingScale){var s=Math.min(1e3/6,this.options.millisPerPixel);if(i-this.lastRenderTimeMillis<s)return}this.resize(),this.updateTooltip(),this.lastRenderTimeMillis=i,t=t||this.canvas,e=e||i-(this.delay||0),e-=e%this.options.millisPerPixel,this.lastChartTimestamp=e;var a=t.getContext(\"2d\"),n=this.options,o={top:0,left:0,width:t.clientWidth,height:t.clientHeight},r=e-o.width*n.millisPerPixel,l=function(t){var e=t-this.currentVisMinValue;return 0===this.currentValueRange?o.height:o.height-Math.round(e/this.currentValueRange*o.height)}.bind(this),h=function(t){return n.scrollBackwards?Math.round((e-t)/n.millisPerPixel):Math.round(o.width-(e-t)/n.millisPerPixel)};if(this.updateValueRange(),a.font=n.labels.fontSize+\"px \"+n.labels.fontFamily,a.save(),a.translate(o.left,o.top),a.beginPath(),a.rect(0,0,o.width,o.height),a.clip(),a.save(),a.fillStyle=n.grid.fillStyle,a.clearRect(0,0,o.width,o.height),a.fillRect(0,0,o.width,o.height),a.restore(),a.save(),a.lineWidth=n.grid.lineWidth,a.strokeStyle=n.grid.strokeStyle,n.grid.millisPerLine>0){a.beginPath();for(var u=e-e%n.grid.millisPerLine;u>=r;u-=n.grid.millisPerLine){var d=h(u);n.grid.sharpLines&&(d-=.5),a.moveTo(d,0),a.lineTo(d,o.height)}a.stroke(),a.closePath()}for(var m=1;m<n.grid.verticalSections;m++){var c=Math.round(m*o.height/n.grid.verticalSections);n.grid.sharpLines&&(c-=.5),a.beginPath(),a.moveTo(0,c),a.lineTo(o.width,c),a.stroke(),a.closePath()}if(n.grid.borderVisible&&(a.beginPath(),a.strokeRect(0,0,o.width,o.height),a.closePath()),a.restore(),n.horizontalLines&&n.horizontalLines.length)for(var p=0;p<n.horizontalLines.length;p++){var f=n.horizontalLines[p],g=Math.round(l(f.value))-.5;a.strokeStyle=f.color||\"#ffffff\",a.lineWidth=f.lineWidth||1,a.beginPath(),a.moveTo(0,g),a.lineTo(o.width,g),a.stroke(),a.closePath()}for(var v=0;v<this.seriesSet.length;v++){a.save();var S=this.seriesSet[v].timeSeries;if(!S.disabled){var b=S.data,y=this.seriesSet[v].options;S.dropOldData(r,n.maxDataSetLength),a.lineWidth=y.lineWidth,a.strokeStyle=y.strokeStyle,a.beginPath();for(var w=0,x=0,T=0,P=0;P<b.length&&1!==b.length;P++){var M=h(b[P][0]),V=l(b[P][1]);if(0===P)w=M,a.moveTo(M,V);else switch(n.interpolation){case\"linear\":case\"line\":a.lineTo(M,V);break;case\"bezier\":default:a.bezierCurveTo(Math.round((x+M)/2),T,Math.round(x+M)/2,V,M,V);break;case\"step\":a.lineTo(M,T),a.lineTo(M,V)}x=M,T=V}b.length>1&&(y.fillStyle&&(a.lineTo(o.width+y.lineWidth+1,T),a.lineTo(o.width+y.lineWidth+1,o.height+y.lineWidth+1),a.lineTo(w,o.height+y.lineWidth),a.fillStyle=y.fillStyle,a.fill()),y.strokeStyle&&\"none\"!==y.strokeStyle&&a.stroke(),a.closePath()),a.restore()}}if(n.tooltip&&this.mouseX>=0&&(a.lineWidth=n.tooltipLine.lineWidth,a.strokeStyle=n.tooltipLine.strokeStyle,a.beginPath(),a.moveTo(this.mouseX,0),a.lineTo(this.mouseX,o.height),a.closePath(),a.stroke(),this.updateTooltip()),!n.labels.disabled&&!isNaN(this.valueRange.min)&&!isNaN(this.valueRange.max)){var N=n.yMaxFormatter(this.valueRange.max,n.labels.precision),F=n.yMinFormatter(this.valueRange.min,n.labels.precision),k=n.scrollBackwards?0:o.width-a.measureText(N).width-2,R=n.scrollBackwards?0:o.width-a.measureText(F).width-2;a.fillStyle=n.labels.fillStyle,a.fillText(N,k,n.labels.fontSize),a.fillText(F,R,o.height-2)}if(n.labels.showIntermediateLabels&&!isNaN(this.valueRange.min)&&!isNaN(this.valueRange.max)&&n.grid.verticalSections>0){var L=(this.valueRange.max-this.valueRange.min)/n.grid.verticalSections,B=o.height/n.grid.verticalSections;for(m=1;m<n.grid.verticalSections;m++){c=o.height-Math.round(m*B);n.grid.sharpLines&&(c-=.5);var W=n.yIntermediateFormatter(this.valueRange.min+m*L,n.labels.precision);intermediateLabelPos=n.labels.intermediateLabelSameAxis?n.scrollBackwards?0:o.width-a.measureText(W).width-2:n.scrollBackwards?o.width-a.measureText(W).width-2:0,a.fillText(W,intermediateLabelPos,c-n.grid.lineWidth)}}if(n.timestampFormatter&&n.grid.millisPerLine>0){var A=n.scrollBackwards?a.measureText(F).width:o.width-a.measureText(F).width+4;for(u=e-e%n.grid.millisPerLine;u>=r;u-=n.grid.millisPerLine){d=h(u);if(!n.scrollBackwards&&d<A||n.scrollBackwards&&d>A){var E=new Date(u),D=n.timestampFormatter(E),C=a.measureText(D).width;A=n.scrollBackwards?d+C+2:d-C-2,a.fillStyle=n.labels.fillStyle,n.scrollBackwards?a.fillText(D,d,o.height-2):a.fillText(D,d-C,o.height-2)}}}a.restore()}},s.timeFormatter=function(t){function e(t){return(t<10?\"0\":\"\")+t}return e(t.getHours())+\":\"+e(t.getMinutes())+\":\"+e(t.getSeconds())},t.TimeSeries=i,t.SmoothieChart=s}(e)}]);</script></body> </html> ";

  }
