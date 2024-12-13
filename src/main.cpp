#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssidAP = "ESP-Access-Point";
const char* password = "12345678";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Декларирование глобальных переменных
String wifiIP = "";
String actionLog[10];
int logIndex = 0;
unsigned long timers[5] = {0, 0, 0, 0, 0};

// Декларирование функций
AsyncWebServer server(80);
String outputState(int output);
String setWifi(String ssid, String pass);
void setAP(String ssid, String pass);
void writeToLog(String pin, String state);

// Веб-страницы, располагающиеся в памяти программ
const char test_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="X-UA-Compatible" content="ie=edge">
    <title>My Website</title>
  </head>
  <body>
    <main>
        <h1>Сейчас ваше устройство выключит точку доступа. Оно будет доступно по адресу:</h1>
        %IPADDRESS%  
    </main>
  </body>
</html>
)rawliteral";
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="utf-8">
  <title>Реле</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Реле</h2>
  <form action="/wifi" method="post">
    <input type="text" name="ssid" placeholder="wifi name">
    <input type="password" name="pass" placeholder="wifi pass">
    <button id="connect" type="submit" value="Submit">Подключиться</button>
  </form>
  %BUTTONPLACEHOLDER%
  <div id="log">
   <h3>Журнал действий</h3>
   %LOGSPLACEHOLDER%
  </div>
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1"+"&value="+element.value, true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0"+"&value="+element.value, true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Лампочка 1</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" value = \"1\" id=\"21\" " + outputState(21) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Лампочка 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" value = \"2\" id=\"19\" " + outputState(19) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Лампочка 3</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" value = \"3\" id=\"18\" " + outputState(18) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Лампочка 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" value = \"4\" id=\"17\" " + outputState(17) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Лампочка 5</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" value = \"5\" id=\"16\" " + outputState(16) + "><span class=\"slider\"></span></label>";
    return buttons;
  } else if (var == "IPADDRESS")
  {
    String text = "";
    text += "<h2>"+ wifiIP +"</h2>";
    return text;
  } else if (var == "LOGSPLACEHOLDER")
  {
    String logHTML ="";
    for (int i=0;i<10;i++){
      if(actionLog[i] != ""){
        logHTML += "<p><h5>" + actionLog[i] + "</h5></p>";
      }
    }
    return logHTML;
  }
  
  
  return String();
}

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

String setWifi(String ssid, String pass){
  // Подключает к сети WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  String out = WiFi.localIP().toString();
  return out;
}

void setAP(String ssid, String pass){
  // Создает локальную точку доступа
  WiFi.softAP(ssid, pass);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);
}

void pinOutSetup(const int* pins, size_t size){
  // Инициализация выходных пинов
  for (size_t i = 0; i<size;i++){
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], LOW);
  }
}

void writeToLog(String pin, String state){
  if (logIndex < 10){
    actionLog[logIndex] = (state == "1")?"Включился ":"Выключился "; 
    actionLog[logIndex] += pin + " выход.";
    logIndex ++;
  }
  else{
    for(int i=0;i<logIndex;i++){
      actionLog[i] = actionLog[i+1];
    }
    actionLog[logIndex] = (state == "1")?"Включился ":"Выключился "; 
    actionLog[logIndex] += pin + " выход.";
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  const int pins[] = {21, 19, 18, 17, 16};
  pinOutSetup(pins, sizeof(pins) / sizeof(pins[0]));
  
  // 
  WiFi.mode(WIFI_MODE_APSTA);
  setAP(ssidAP, password);

  // Используется для отладки
  setWifi("ASUS_58_2G", "123qwe123");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1; // Номер пина
    String inputMessage2; // Состояние
    String inputMessage3; // Порядковый номер выхода
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2) && request->hasParam("value")) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputMessage3 = request->getParam("value")->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
      writeToLog(inputMessage3, inputMessage2);
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  server.on("/setTimer", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value") && request->hasParam("time")){
      int pin = request->getParam("value")->value().toInt();
      unsigned long time = request->getParam("time")->value().toInt()*1000;
      timers[pin] = millis() + time;
      Serial.println(pin);
      Serial.println(time);
    }
    request->send(200, "text/plain", "Timer set");
    
  });

  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    String input1;
    String input2;
    
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)){
      input1 = request->getParam("ssid", true)->value();
      input2 = request->getParam("pass", true)->value();
    }
    else{
      input1 = "NO MESSAGE";
      input2 = "NO MESSAGE";
    }

    Serial.println(input1);
    Serial.println(input2);
    wifiIP = setWifi(input1, input2);
    request->send_P(200, "text/html", test_html, processor);
  });

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", test_html);
  });

  // Start server
  server.begin();
}

void loop() {

}