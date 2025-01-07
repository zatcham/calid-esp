#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Preferences.h>
#include <time.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Create instances
WiFiManager wifiManager;
Preferences preferences;
WebServer webServer(80);

// Default settings - TODO: move to separate config.h file
const char* DEFAULT_API_ENDPOINT = "http://calid.uk/api/sensor/data";
const int DEFAULT_SENSOR_PIN = 26;
const char* DEFAULT_SENSOR_ID = "";
const char* DEFAULT_API_KEY = "";

// Settings variables
String apiEndpoint;
int sensorPin;
String sensorID;
String apiKey;

// DHT sensor settings
#define DHTTYPE DHT11
DHT* dht = nullptr;

// NTP server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Time between sensor readings (ms)
const unsigned long READING_INTERVAL = 10000;
unsigned long lastReadingTime = 0;

void loadPreferences() {
    preferences.begin("config", false);
    apiEndpoint = preferences.getString("apiEndpoint", DEFAULT_API_ENDPOINT);
    sensorPin = preferences.getInt("sensorPin", DEFAULT_SENSOR_PIN);
    sensorID = preferences.getString("sensorID", DEFAULT_SENSOR_ID);
    apiKey = preferences.getString("apiKey", DEFAULT_API_KEY);
    preferences.end();
}

void savePreferences() {
    preferences.begin("config", false);
    preferences.putString("apiEndpoint", apiEndpoint);
    preferences.putInt("sensorPin", sensorPin);
    preferences.putString("sensorID", sensorID);
    preferences.putString("apiKey", apiKey);
    preferences.end();
}

void handleRoot() {
    String page = R"(
        <html>
        <head>
            <title>Calid Sensor Config</title>
            <style>
                body { font-family: Arial; margin: 20px; }
                .form-group { margin: 10px 0; }
                input[type='text'], input[type='number'] { width: 300px; padding: 5px; }
                input[type='submit'] { padding: 10px 20px; }
            </style>
        </head>
        <body>
            <h1>Calid - ESP32 Sensor Configuration <small>(v0.1)</small></h1>
            <h5>Config Upload</h5>
            <form method="POST" action="/upload" enctype="multipart/form-data">
                <input type="file" name="upload">
                <input type="submit" value="Upload">
            </form>
            <h5>Manual Config</h5>
            <form action='/save' method='POST'>
                <div class='form-group'>
                    <label>API Endpoint:</label><br>
                    <input type='text' name='apiEndpoint' value=')" + apiEndpoint + R"('> 
                </div>
                <div class='form-group'>
                    <label>Sensor Pin:</label><br>
                    <input type='number' name='sensorPin' value=')" + String(sensorPin) + R"('> 
                </div>
                <div class='form-group'>
                    <label>Sensor ID:</label><br>
                    <input type='text' name='sensorID' value=')" + sensorID + R"('> 
                </div>
                <div class='form-group'>
                    <label>API Key:</label><br>
                    <input type='text' name='apiKey' value=')" + apiKey + R"('> 
                </div>
                <input type='submit' value='Save'>
            </form>
        </body>
        </html>
    )";
    webServer.send(200, "text/html", page);
}

void handleSave() {
    if (webServer.method() != HTTP_POST) {
        webServer.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    apiEndpoint = webServer.arg("apiEndpoint");
    sensorPin = webServer.arg("sensorPin").toInt();
    sensorID = webServer.arg("sensorID");
    apiKey = webServer.arg("apiKey");

    // Reinitialize DHT sensor if pin changed
    if (dht != nullptr) {
        delete dht;
    }
    dht = new DHT(sensorPin, DHTTYPE);
    dht->begin();

    savePreferences();

    webServer.send(200, "text/html", "<html><body><h1>Settings Saved</h1><a href='/'>Back to configuration</a></body></html>");
}

void handleFileUpload() {
    HTTPUpload& upload = webServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Upload: START, filename: %s\n", upload.filename.c_str());
        File file = SPIFFS.open("/config.json", "w");
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }
        file.close();
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        Serial.printf("Upload: WRITE, bytes: %d\n", upload.currentSize);
        File file = SPIFFS.open("/config.json", "a");
        if (file) {
            file.write(upload.buf, upload.currentSize);
            file.close();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.printf("Upload: END, Size: %d\n", upload.totalSize);
        webServer.send(200, "text/plain", "Upload complete");

        // Load and process the uploaded config
        loadJsonConfig();
    }
}


void sendSensorData(float value, const char* dataType, const char* unit) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Disconnected");
        return;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // Create JSON payload
    String payload = "[{\"time\":\"" + String(timeStr) + 
                    "\",\"sensor_id\":\"" + sensorID + 
                    "\",\"data_type\":\"" + String(dataType) + 
                    "\",\"value\":\"" + String(value) + 
                    "\",\"unit\":\"" + String(unit) + "\"}]";

    Serial.println(payload);

    HTTPClient http;
    http.begin(apiEndpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Sensor-ID", sensorID);
    http.addHeader("X-Sensor-Api-Key", apiKey);

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
        Serial.printf("%s: HTTP Response code: %d\n", dataType, httpResponseCode);
    } else {
        Serial.printf("Error sending %s: %d\n", dataType, httpResponseCode);
    }

    http.end();
}

void loadJsonConfig() {
    File file = SPIFFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return;
    }

    // Allocate a buffer to store the file contents
    StaticJsonDocument<1024> doc;

    // Parse the JSON file
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Failed to parse config file: ");
        Serial.println(error.c_str());
        file.close();
        return;
    }

    file.close();

    // Access JSON values
    const char* sensorId = doc["sensor_id"];
    const char* apiKey = doc["api_key"];
    const char* apiEndpoint = doc["api_endpoint"];
    int updateInterval = doc["update_interval"];
    bool temperatureEnabled = doc["sensors"]["temperature"];
    bool humidityEnabled = doc["sensors"]["humidity"];

    // Save the configuration to Preferences
    preferences.begin("config", false);
    preferences.putString("sensor_id", sensorId);
    preferences.putString("api_key", apiKey);
    preferences.putString("api_endpoint", apiEndpoint);
    preferences.putInt("update_interval", updateInterval);
    preferences.putBool("temperature", temperatureEnabled);
    preferences.putBool("humidity", humidityEnabled);
    preferences.end();
}


void setupWebServer() {
  webServer.on("/", HTTP_GET, []() {
        webServer.send(200, "text/html", R"(
            
        )");
    });

    webServer.on("/save", handleSave);

    webServer.on("/upload", HTTP_POST, []() {}, handleFileUpload);

    webServer.begin();
}

void setup() {
    Serial.begin(115200);
    if (!SPIFFS.begin(true)) {
      Serial.println("Failed to mount SPIFFS");
      return;
    }
    
    // Load settings
    loadPreferences();

    // Initialise DHT sensor
    dht = new DHT(sensorPin, DHTTYPE);
    dht->begin();

    // Configure WiFiManager
    wifiManager.setConfigPortalTimeout(180);
    if (!wifiManager.autoConnect("ESP32_Sensor_AP")) {
        Serial.println("Failed to connect. Restarting...");
        delay(3000);
        ESP.restart();
    }

    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Setup web server routes
    setupWebServer();

    // Initialize NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
    webServer.handleClient();

    unsigned long currentTime = millis();
    if (currentTime - lastReadingTime >= READING_INTERVAL) {
        float humidity = dht->readHumidity();
        float temperature = dht->readTemperature();

        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }

        sendSensorData(temperature, "temperature", "C");
        sendSensorData(humidity, "humidity", "%");

        lastReadingTime = currentTime;
    }
}
