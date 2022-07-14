#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>             // Khai báo thư viện sử dụng cho động cơ
#include <LiquidCrystal_I2C.h> // Khai báo thư viện LCD sử dụng I2C
#include <Keypad.h>            // Khai báo thư viện Keypad
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 27 // Digital pin connected to the DHT sensor
#define servosPins 26
#define DHTTYPE DHT11 // DHT 11
#define MOTIONSENSOR_PIN 1
#define BUTTON_PIN 25
// #define DHTTYPE DHT22 // DHT 22 (AM2302)
const char *ssid = "Fuvitech";
const char *password = "fuvitech.vn";
#define MQTT_SERVER "103.130.214.230"
#define MQTT_PORT 1883
#define MQTT_TEMPERATURE_TOPIC "nodeWiFi32/dht11/temperature"
#define MQTT_HUMIDITY_TOPIC "nodeWiFi32/dht11/humidity"
#define MQTT_LED_TOPIC "nodeWiFi32/led"

TaskHandle_t Task1;
TaskHandle_t Task2;

LiquidCrystal_I2C lcd(0x27, 16, 2); // 0x27 địa chỉ LCD, 16 cột và 2 hàng
const byte ROWS = 4;                // Bốn hàng
const byte COLS = 4;                // Ba cột
byte rowPins[ROWS] = {19, 18, 5, 17};
byte colPins[COLS] = {16, 4, 0, 2};
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'}, {'4', '5', '6', 'B'}, {'7', '8', '9','C'}, {'*', '0', '#','D'}};

char STR[4] = {'2', '0', '1', '8'}; // Cài đặt mật khẩu tùy ý
char str[4] = {' ', ' ', ' ', ' '};
int i, j, count = 0;
unsigned long previousMillis = 0;
const long interval = 2000;
#define DOOR_STATE_OPEN 1
#define DOOR_STATE_CLOSE 0
int doorState = DOOR_STATE_OPEN;
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
Servo myServo;
WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHTPIN, DHTTYPE);

void Task1code(void *parameter);
void Task2code(void *parameter);
void callback(char *topic, byte *payload, unsigned int length);
void setup_wifi();
void connect_to_broker();
void openDoor();
void closeDoor();

void setup()
{
  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  connect_to_broker();

  pinMode(BUTTON_PIN, INPUT);
  pinMode(MOTIONSENSOR_PIN, OUTPUT);

  myServo.attach(servosPins); // Khai báo chân điều khiển động cơ
  dht.begin();
  lcd.init(); // Khai báo sử dụng LCD
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.print(" Enter Password");

  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 0);
  delay(500);

  xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, &Task2, 1);
  delay(500);
}
void Task1code(void *parameter)
{
  Serial.print("Task1 is running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    if (!client.connected())
    {
      connect_to_broker();
    }
    unsigned long currentMillis = millis();
    /* This is the event */
    if (currentMillis - previousMillis >= interval)
    {
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t))
      {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
      else
      {
        client.publish(MQTT_TEMPERATURE_TOPIC, String(t).c_str());
        client.publish(MQTT_HUMIDITY_TOPIC, String(h).c_str());
        previousMillis = currentMillis;
      }
    }
  }
}

void Task2code(void *parameter)
{
  Serial.print("Task2 is running on core ");
  Serial.println(xPortGetCoreID());

  for (;;)
  {
    if (digitalRead(MOTIONSENSOR_PIN) == HIGH)
    {
      if (doorState == DOOR_STATE_OPEN)
      {
        closeDoor();
      }
      else
      {
        openDoor();
      }
    }
    char key = keypad.getKey(); // Ký tự nhập vào sẽ gán cho biến Key
    if (key)                    // Nhập mật khẩu
    {
      if (i == 0)
      {
        str[0] = key;
        lcd.setCursor(6, 1);
        lcd.print(str[0]);
        delay(1000); // Ký tự hiển thị trên màn hình LCD trong 1s
        lcd.setCursor(6, 1);
        lcd.print("*"); // Ký tự được che bởi dấu *
      }
      if (i == 1)
      {
        str[1] = key;
        lcd.setCursor(7, 1);
        lcd.print(str[1]);
        delay(1000);
        lcd.setCursor(7, 1);
        lcd.print("*");
      }
      if (i == 2)
      {
        str[2] = key;
        lcd.setCursor(8, 1);
        lcd.print(str[2]);
        delay(1000);
        lcd.setCursor(8, 1);
        lcd.print("*");
      }
      if (i == 3)
      {
        str[3] = key;
        lcd.setCursor(9, 1);
        lcd.print(str[3]);
        delay(1000);
        lcd.setCursor(9, 1);
        lcd.print("*");
        count = 1;
      }
      i = i + 1;
    }

    if (count == 1)
    {
      if (str[0] == STR[0] && str[1] == STR[1] && str[2] == STR[2] &&
          str[3] == STR[3])
      {
        lcd.clear();
        lcd.print("    Correct!");
        delay(3000);
        myServo.write(180); // Mở cửa
        lcd.clear();
        lcd.print("      Opened!");
        i = 0;
        count = 0;
      }
      else
      {
        lcd.clear();
        lcd.print("   Incorrect!");
        delay(3000);
        lcd.clear();
        lcd.print("   Try Again!");
        delay(3000);
        lcd.clear();
        lcd.print(" Enter Password");
        i = 0;
        count = 0;
      }
    }

    switch (key)
    {
    case '#':
      lcd.clear();
      myServo.write(90); // close door
      lcd.print("     Closed!");
      delay(10000);
      lcd.clear();
      lcd.print(" Enter Password");
      i = 0;
      break;
    }
  }
}
void loop()
{
}

void setup_wifi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_to_broker()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "fuvitech";
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}
void openDoor()
{
  doorState = DOOR_STATE_OPEN;
  myServo.write(180); // close door
};
void closeDoor()
{
  doorState = DOOR_STATE_CLOSE;
  myServo.write(90); // close door
};
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("-------new message from broker-----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("message: ");
  Serial.write(payload, length);
  Serial.println();
}
