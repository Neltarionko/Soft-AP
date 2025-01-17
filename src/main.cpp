#include <Arduino.h>

#define AP_SSID "ASUS_58_2G"
#define AP_PASS "123qwe123"

#define GH_INCLUDE_PORTAL 
#include <GyverHub.h>
GyverHub hub("MyDevices", "ESP", "");

#include <PairsFile.h>
PairsFile data(&GH_FS, "/data.dat", 3000);

#define FB_NO_FILE
#include <FastBot2.h>
FastBot2 bot;

const char _ui[] PROGMEM = R"json({"type":"row","width":1,"data":[{"id":"inp","type":"input"},{"id":"sld","type":"slider"},{"id":"btn","type":"button"}]})json";
const char* fetch_bytes = "fetch bytes\nLorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi dignissim tellus ligula. Vivamus id lacus ac tortor commodo aliquam. Mauris eget faucibus nunc. Vestibulum tempus eu lorem a dapibus. Nullam ac dapibus ex. Aenean faucibus dapibus porttitor. Sed vel magna id tellus mattis semper. Fusce a finibus ligula. In est turpis, viverra eget libero ut, pretium pellentesque velit. Praesent ultrices elit quis facilisis mattis. Donec eu iaculis est. Sed tempus feugiat ligula non ultricies. Cras a auctor nibh, sed sodales sapien.\n\nSed cursus quam vel egestas rhoncus. Curabitur dignissim lorem sed metus sollicitudin, non faucibus erat interdum. Nunc vitae lobortis dui, mattis dignissim orci. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Duis vel venenatis purus. Nunc luctus leo tincidunt felis efficitur ullamcorper. Aliquam semper rhoncus odio sed porta. Quisque blandit, dui vel imperdiet ultricies, dolor arcu posuere turpis, et gravida ante libero ut ex. Vestibulum sed scelerisque nibh, nec mollis urna. Suspendisse tortor sapien, congue at aliquam vitae, venenatis placerat enim. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nam posuere metus a est commodo finibus. Donec luctus arcu purus, sit amet sodales dolor facilisis id. Nullam consectetur sapien vitae nisi gravida, sed finibus dui hendrerit. In id pretium odio, imperdiet lacinia massa. Morbi quis condimentum ligula.";
const char fetch_pgm[] PROGMEM = "fetch pgm\nLorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi dignissim tellus ligula. Vivamus id lacus ac tortor commodo aliquam. Mauris eget faucibus nunc. Vestibulum tempus eu lorem a dapibus. Nullam ac dapibus ex. Aenean faucibus dapibus porttitor. Sed vel magna id tellus mattis semper. Fusce a finibus ligula. In est turpis, viverra eget libero ut, pretium pellentesque velit. Praesent ultrices elit quis facilisis mattis. Donec eu iaculis est. Sed tempus feugiat ligula non ultricies. Cras a auctor nibh, sed sodales sapien.\n\nSed cursus quam vel egestas rhoncus. Curabitur dignissim lorem sed metus sollicitudin, non faucibus erat interdum. Nunc vitae lobortis dui, mattis dignissim orci. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Duis vel venenatis purus. Nunc luctus leo tincidunt felis efficitur ullamcorper. Aliquam semper rhoncus odio sed porta. Quisque blandit, dui vel imperdiet ultricies, dolor arcu posuere turpis, et gravida ante libero ut ex. Vestibulum sed scelerisque nibh, nec mollis urna. Suspendisse tortor sapien, congue at aliquam vitae, venenatis placerat enim. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nam posuere metus a est commodo finibus. Donec luctus arcu purus, sit amet sodales dolor facilisis id. Nullam consectetur sapien vitae nisi gravida, sed finibus dui hendrerit. In id pretium odio, imperdiet lacinia massa. Morbi quis condimentum ligula.";
bool dsbl, nolbl, notab;
String token;

// Структура релейных выходов
struct Relay {
    bool state;
    int output;
    int led;
};

Relay relay[4] = {
    {false, 25, 21},
    {false, 26, 19},
    {false, 27, 18},
    {false, 4, 17}
};
const int relay_count = 4;

// Логгер
gh::Log hlog;

// Функция настройки выходных пинов контроллера
void pinSetup(){
    for (int i=0; i<relay_count; i++){
        pinMode(relay[i].output, OUTPUT);
        digitalWrite(relay[i].output, LOW);
        pinMode(relay[i].led, OUTPUT);
        digitalWrite(relay[i].led, LOW);
    }
}

// Функция подключения к сети WiFi
void WiFiSetup(char* ssid, char* pass){
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    return;
  } 
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

// Обновление состояния выходов
void update_outputs(){
    for (int i=0; i<relay_count; i++){
        digitalWrite(relay[i].output, relay[i].state?HIGH:LOW);
        digitalWrite(relay[i].led, relay[i].state?HIGH:LOW);
        Serial.print("Relay ");
        Serial.print(i);
        Serial.print(" state: ");
        Serial.println(relay[i].state);
    }
}

// Запись токена для Telegram бота в переменную
void set_token(gh::Build& b) {
    token = b.value;
    Serial.println(token);
}

// Обработка входящих сообщений от Telegram бота
void update(fb::Update& u) {
    Text chat_id = u.message().chat().id();
    Text message = u.message().text();

    if (message == "/start") {
        bot.sendMessage(fb::Message("Привет! Я бот для управления умным домом. Введите /menu для вызова меню управления", chat_id));
    } else if (message == "/menu"){ 
        fb::Message msg("Выберите номер реле, которое надо переключить", chat_id);
        
        fb::Menu menu;
        menu.text = "1 ; 2 ; 3 ; 4 ";
        menu.resize = 4;
        menu.placeholder = "Номер выхода";
        msg.setMenu(menu);

        bot.sendMessage(msg);
    } else {
        int relay_index = message.toInt() - 1;
        if (relay_index >= 0 && relay_index < 5){
            relay[relay_index].state = !relay[relay_index].state;
            fb::Message msg("Реле " + String(relay_index + 1) + " переключено.", chat_id);
            bot.sendMessage(msg);
            update_outputs();
        }
    }
}

// Функция построения страницы с кнопками реле
void build_common(gh::Builder& b) {
    b.Title("Реле");
    {
        gh::Row r(b);
        b.Switch_("relay1", &relay[0].state).label("1");
        b.Switch_("relay2", &relay[1].state).label("2");
        b.Switch_("relay3", &relay[2].state).label("3");
        b.Switch_("relay4", &relay[3].state).label("4");
    }
    update_outputs();
    if (b.changed()) b.refresh();
}

// Функция построения страницы с настройками Telegram
void build_telegram(gh::Builder& b){
    b.Title("Подключение Telegram");
    {
        gh::Row r(b);
        b.Input().size(3).label("token").attach(set_token);
        if (b.Button().label("Сохранить").size(1).click()) {
            data["token"] = token;
            data.update();
            bot.setToken(token);
            bot.begin();
            Serial.println("Bot started");
            Serial.println(token);
            bot.setPollMode(fb::Poll::Long, 20000);
        }
    }
}

// Функция построения веб-интерфейса
void build(gh::Builder& b) {
    if (b.build.isSet()) hlog.println(b.build.name);
    b.Menu(F("Реле;Telegram"));

    switch (hub.menu) {
        case 0:
            build_common(b);
            break;
        case 1:
            build_telegram(b);
            break;
    }
}

void setup() {
    // Инициализация
    Serial.begin(115200);
    hlog.begin();
    GH_FS.begin();

    pinSetup();
    WiFiSetup(AP_SSID, AP_PASS); // Для тестов

    hub.onBuild(build); //Подключение функции построения интерфейса

    // Попытка настроить MQTT, пока работает только на чтение
    hub.mqtt.config(IPAddress(192, 168, 1, 102), 1883, "emelya", "emelya");
    hub.sendGetAuto(true);

    // Обработчик консольных команд
    hub.onCLI([](String str) {
        Serial.println(str);
        hub.sendCLI(str + "!");
    });

    // Настройка инфо панели
    hub.onInfo([](gh::Info& info) {
        switch (info.type) {
            case gh::Info::Type::Version:
                info.add("ESP relay interface", "v0.1");
                break;
            case gh::Info::Type::Network:
                info.add(F("5G"), "50%");
                break;
            case gh::Info::Type::Memory:
                info.add(F("SD"), "10 GB");
                info.add(F("Int"), 100500);
                break;
            case gh::Info::Type::System:
                info.add(F("Battery"), 3.63, 2);
                break;
        }
    });

    // Обработчик запросов
    hub.onRequest([](gh::Request& req) -> bool {
        Serial.print("Request: ");
        Serial.print(gh::readConnection(req.client.connection()));
        Serial.print(',');
        Serial.print(req.client.id);
        Serial.print(',');
        Serial.print(gh::readCMD(req.cmd));
        Serial.print(',');
        Serial.print(req.name);
        Serial.print(',');
        Serial.print(req.value);
        Serial.println();
        return 1;
    });

#ifdef GH_ESP_BUILD
    // Действия при перезагрузке
    hub.onReboot([](gh::Reboot r) {
        Serial.println(gh::readReboot(r));
    });

    hub.onFetch([](gh::Fetcher& f) {
        // if (f.path == "/fetch_file.txt") f.fetchFile("/fetch_file.txt");
        if (f.path == "/fetch_bytes.txt") f.fetchBytes((uint8_t*)fetch_bytes, strlen(fetch_bytes));
        if (f.path == "/fetch_pgm.txt") f.fetchBytes_P((uint8_t*)fetch_pgm, strlen_P(fetch_pgm));
    });

    hub.onUpload([](String& path) {
        Serial.print("Uploaded: ");
        Serial.println(path);
        if (path == "/data.dat") data.begin();  // refresh from file
    });
#endif

    hub.begin();

    bot.onUpdate(update);
    Serial.println("Ready");

    data.begin();
    hub.setBufferSize(2000);

    // Проверка наличия токена в файле, если есть - запускает бота
    if (data.has("token")) {
        Text raw_token = data.get("token");
        bot.setToken(raw_token);
    }
}

void loop() {
    data.tick();
    hub.tick();
    bot.tick();
}

/* TODO:
Разобраться с MQTT, почему не работает топик set
Добавить поле настройки для поключения к MQTT брокеру
Добавить таймеры для реле
Продублировать таймеры в тг боте
*/