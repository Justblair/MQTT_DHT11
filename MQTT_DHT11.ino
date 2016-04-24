

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define wifi_ssid "..............."
#define wifi_password "................"

byte mqtt_server[] = { 192, 168 ,0 , 23 };
#define mqtt_user ""
#define mqtt_password ""

#define humidity_topic "btsensor/humidity"
#define temperature_topic "btsensor/temperature"
#define heatIndex_topic	"btsensor/heatindex"

#define DHTPIN D4     // what pin we're connected to

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


// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 0.2;

unsigned long oldTime;

void setup() {
	Serial.begin(115200);
	Serial.println("DHTxx test!");
	setup_wifi();
	client.setServer(mqtt_server, 1883);
	client.setCallback(callback);
	dht.begin();
	reconnect();
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	// Switch on the LED if an 1 was received as first character
	if ((char)payload[0] == '1') {
		digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
		// but actually the LED is on; this is because
		// it is acive low on the ESP-01)
	}
	else {
		digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
	}

}



void reconnect() {
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Attempt to connect
		// If you do not want to use a username and password, change next line to
		// if (client.connect("ESP8266Client")) {
		if (client.connect("ESP8266Client")) {
			Serial.println("connected");
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
	return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

void loop() {

	if (!client.connected()) {
		reconnect();
	}
	client.loop();

	if (oldTime + 60000 < millis()) {
		// Reading temperature or humidity takes about 250 milliseconds!
	  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
		float h = dht.readHumidity();
		// Read temperature as Celsius (the default)
		float t = dht.readTemperature();
		// Read temperature as Fahrenheit (isFahrenheit = true)
		float f = dht.readTemperature(true);

		// Check if any reads failed and exit early (to try again).
		if (isnan(h) || isnan(t) || isnan(f)) {
			Serial.println("Failed to read from DHT sensor!");
			return;
		}

		// Compute heat index in Fahrenheit (the default)
		float hif = dht.computeHeatIndex(f, h);
		// Compute heat index in Celsius (isFahreheit = false)
		float hic = dht.computeHeatIndex(t, h, false);
		client.publish(heatIndex_topic, String(hic).c_str(), true);

		//if (checkBound(t, temp, diff)) {
		temp = t;
		Serial.print("New temperature:");
		Serial.println(String(temp).c_str());
		client.publish(temperature_topic, String(temp).c_str(), true);
		//}

//		if (checkBound(h, hum, diff)) {
		hum = h;
		Serial.print("New humidity:");
		Serial.println(String(hum).c_str());
		client.publish(humidity_topic, String(hum).c_str(), true);

		//	}
			//client.publish(humidity_topic, "Hello");
		Serial.print("Humidity: ");
		Serial.print(h);
		Serial.print(" %\t");
		Serial.print("Temperature: ");
		Serial.print(t);
		Serial.print(" *C ");
		Serial.print(f);
		Serial.print(" *F\t");
		Serial.print("Heat index: ");
		Serial.print(hic);
		Serial.print(" *C ");
		Serial.print(hif);
		Serial.println(" *F");

		// Wait a few seconds between measurements.
//		delay(2000);
		oldTime = millis();
	}
}


