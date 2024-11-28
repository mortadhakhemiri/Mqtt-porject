#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <FirebaseESP8266.h>

// WiFi credentials
const char* ssid = "Rmiki";
const char* password = "Hovcu3378#@firas";

// MQTT broker credentials
const char* MQTT_username = ""; 
const char* MQTT_password = ""; 
const char* mqtt_server = "192.168.17.6";  // MQTT broker address

// Firebase configuration
#define FIREBASE_HOST "https://sensor-a9e85-default-rtdb.firebaseio.com"  // Firebase URL
#define FIREBASE_AUTH "Q5nLp6MwBYhKd2WwiKanvpzJBxwFrWhXTsgYvo08"  // Firebase Auth Key

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Pin assignments
const int potPin = A0;   // Potentiometer on A0
const int lamp = 4;       // Lamp (LED) on GPIO 4 (D4 )

// Timers
long now = millis();
long lastMeasure = 0;

// WiFi and MQTT Client setup
WiFiClient espClient;
PubSubClient client(espClient);

// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT callback function to handle incoming messages
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

 if (topic == "home/lamp") {
    Serial.print("Changing home lamp to ");
    if (messageTemp == "on") {
      digitalWrite(lamp, HIGH);
      Serial.println("On");
    } else if (messageTemp == "off") {
      digitalWrite(lamp, LOW);
      Serial.println("Off");
    }
  }

}

// Reconnect to the MQTT broker if connection is lost
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", MQTT_username, MQTT_password)) {
      Serial.println("connected");
      client.subscribe("home/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 second");
      delay(1000);
    }
  }
}

// Setup function to initialize WiFi, MQTT, and Firebase
void setup() {
  pinMode(lamp, OUTPUT);

  // Serial monitor setup
  Serial.begin(115200);
  setup_wifi();

  // Initialize Firebase configuration
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&firebaseConfig, nullptr);

  if (!Firebase.ready()) {
    Serial.println("Firebase initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Firebase initialized.");

  // MQTT setup
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Main loop function
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if (!client.loop()) {
    client.connect("ESP8266Client", MQTT_username, MQTT_password);
  }

  now = millis();
  
  // Measure potentiometer value and voltage every second
  if (now - lastMeasure > 1000) {
    lastMeasure = now;

    int potValue = analogRead(potPin);
    float voltage = potValue * (3.3 / 1023.0); // Convert ADC value to voltage

    // Publish potentiometer value and voltage to MQTT
    client.publish("home/potentiometer", String(potValue).c_str());
    client.publish("home/voltage", String(voltage).c_str());

    // Send potentiometer value and voltage to Firebase
    String path = "/sensorData"; // Firebase path
    if (Firebase.pushFloat(firebaseData, path + "/potentiometer", potValue)) {
      Serial.println("Potentiometer data sent to Firebase.");
    } else {
      Serial.print("Failed to send potentiometer data to Firebase: ");
      Serial.println(firebaseData.errorReason());
    }

    if (Firebase.pushFloat(firebaseData, path + "/voltage", voltage)) {
      Serial.println("Voltage data sent to Firebase.");
    } else {
      Serial.print("Failed to send voltage data to Firebase: ");
      Serial.println(firebaseData.errorReason());
    }

    // Debugging output
    Serial.print("Potentiometer Value: ");
    Serial.println(potValue);
    Serial.print("Voltage: ");
    Serial.println(voltage);
  }
}
