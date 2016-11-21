#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

#define wifi_ssid "________WIFI_SSID________"
#define wifi_password "________WIFI_PASS________"

#define mqtt_server "________MQTT_SERVER________"
#define mqtt_user "________MQTT_USER________"
#define mqtt_password "________MQTT_PASS________"

#define motion_topic "sensor/motion1"
#define button_topic "sensor/button1"
#define humidity_topic "sensor/humidity1"
#define temp_topic "sensor/temp1"

#define motion_pin 12
#define button_pin 2

#define dht_type DHT11
#define dht_pin 16

int motionState = 0;
int buttonState = 2;
int buttonCount = 0;

DHT dht(dht_pin, dht_type, 11); // 11 works fine for ESP8266
 
float humidity, temp_f;  // Values read from sensor
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 10000;              // interval at which to read sensor in seconds

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();           // initialize temperature sensor
  setup_wifi();          // connect to wifi
  client.setServer(mqtt_server, 1883);

  //pinMode(0, OUTPUT);
  pinMode(motion_pin, INPUT);
  pinMode(button_pin, INPUT);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Read the sensor
  int currentRead = digitalRead(motion_pin);
  // If motion is detected we don't want to 'spam' the service
  if(currentRead != motionState) {
    motionState = currentRead;
    String message = motionState ? "controlOn" : "controlOff";
    Serial.print("New Motion:");
    Serial.println(String(message).c_str());
    client.publish(motion_topic, String(message).c_str(), true);
  }
  int currentRead2 = digitalRead(button_pin);   
  if(currentRead2 == buttonState && buttonState == 0 && buttonCount < 1) {
    buttonCount = buttonCount + 1;
    String message = "controlOn";
    Serial.print("New Button:");
    Serial.println(String(message + " : " + buttonCount).c_str());
    client.publish(button_topic, String(message).c_str(), true);
  }
  if(currentRead2 != buttonState) {
    buttonState = currentRead2;
    if (buttonCount > 0) {
      buttonCount = 0;
    }
  }

  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;
    gettemperature();
    String message = String(humidity);
    Serial.print("New Humidity:");
    Serial.println(String(message).c_str());
    client.publish(humidity_topic, String(message).c_str(), true);

    message = String(temp_f);
    Serial.print("New Temp F:");
    Serial.println(String(message).c_str());
    client.publish(temp_topic, String(message).c_str(), true);
  }
}

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  
 
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  humidity = dht.readHumidity();          // Read humidity (percent)
  temp_f = dht.readTemperature(true);     // Read temperature as Fahrenheit
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temp_f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
}
