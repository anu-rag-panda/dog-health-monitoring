// Dog Health Monitoring System
 // written by- Anurag Panda
 //17:00
 //25/11/2024
 //Durgapur
 // Components:
 //- Arduino Nano
 //- ESP8266 WiFi Module
 //- MAX30102 Heart Rate Sensor
 //- MPU6050 Accelerometer/Gyroscope
 //- NEO-6M GPS Module
 //- SIM800L GSM Module
 #include <Wire.h>
 #include <SoftwareSerial.h>
 #include <TinyGPS++.h>
 #include <MPU6050.h>
 #include <MAX30105.h>
 #include <heartRate.h>
 #include <ESP8266WiFi.h>
 #include <ThingSpeak.h>
 // WiFi credentials
 const char* WIFI_SSID = "YOUR_WIFI_SSID";
 const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
 // ThingSpeak credentials
 unsigned long CHANNEL_ID = YOUR_CHANNEL_ID;
 const char* API_KEY = "YOUR_API_KEY";
 // Pin definitions
 #define GPS_RX 4
 #define GPS_TX 3
 #define GSM_RX 6
 #define GSM_TX 5
 // Create objects for sensors and modules
 SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
 SoftwareSerial gsmSerial(GSM_RX, GSM_TX);
 TinyGPSPlus gps;
MPU6050 mpu;
 MAX30105 heartSensor;
 WiFiClient client;
 // Global variables for sensor readings
 float heartRate = 0;
 float latitude = 0;
 float longitude = 0;
 float temperature = 0;
 int16_t ax, ay, az;
 int16_t gx, gy, gz;
 bool isMoving = false;
 const int movementThreshold = 8000;
 void setup() {
 // Initialize serial communications
 Serial.begin(9600);
 gpsSerial.begin(9600);
 gsmSerial.begin(9600);
 // Initialize I2C devices
 Wire.begin();
 mpu.initialize();
 // Initialize heart rate sensor
 if (!heartSensor.begin()) {
 Serial.println("Heart rate sensor not found!");
 while (1);
 }
 heartSensor.setup();
 heartSensor.setPulseAmplitudeRed(0x0A);
 // Initialize WiFi connection
 WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 while (WiFi.status() != WL_CONNECTED) {
 delay(1000);
 Serial.println("Connecting to WiFi...");
 }
 Serial.println("Connected to WiFi");
 // Initialize ThingSpeak
ThingSpeak.begin(client);
 // Initialize GSM module
 initGSM();
 }
 void loop() {
 // Read sensors
 readHeartRate();
 readGPS();
 readMPU();
 checkMovement();
 // Send data to ThingSpeak
 sendToThingSpeak();
 // If unusual readings detected, send SMS alert
 checkAlerts();
 delay(15000); // Wait 15 seconds before next reading
 }
 void readHeartRate() {
 long irValue = heartSensor.getIR();
 if (irValue > 50000) {
 heartRate = getHeartRate(irValue);
 } else {
 heartRate = 0; // No finger detected
 }
 }
 void readGPS() {
 while (gpsSerial.available() > 0) {
 if (gps.encode(gpsSerial.read())) {
 if (gps.location.isValid()) {
 latitude = gps.location.lat();
 longitude = gps.location.lng();
 }
 }
 }
}
 void readMPU() {
 mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
 temperature = mpu.getTemperature() / 340.0 + 36.53;
 }
 void checkMovement() {
 // Calculate total acceleration
 long totalAccel = sqrt(ax*ax + ay*ay + az*az);
 isMoving = (totalAccel > movementThreshold);
 }
 void sendToThingSpeak() {
 ThingSpeak.setField(1, heartRate);
 ThingSpeak.setField(2, latitude);
 ThingSpeak.setField(3, longitude);
 ThingSpeak.setField(4, temperature);
 ThingSpeak.setField(5, isMoving ? 1 : 0);
 int response = ThingSpeak.writeFields(CHANNEL_ID, API_KEY);
 if (response == 200) {
 Serial.println("Data sent to ThingSpeak successfully");
 } else {
 Serial.println("Error sending data to ThingSpeak");
 }
 }
 void initGSM() {
 gsmSerial.println("AT");
 delay(1000);
 gsmSerial.println("AT+CMGF=1"); // Set SMS text mode
 delay(1000);
 }
 void sendSMS(const char* message, const char* phoneNumber) {
 gsmSerial.println("AT+CMGS=\"" + String(phoneNumber) + "\"");
 delay(1000);
 gsmSerial.print(message);
 delay(100);
gsmSerial.write(26); // Ctrl+Z to send SMS
 delay(1000);
 }
 void checkAlerts() {
 if (heartRate > 120 || heartRate < 60) {
 sendSMS("Alert: Abnormal heart rate detected!",
 "YOUR_PHONE_NUMBER");
 }
 if (temperature > 39.5) { // High fever
 sendSMS("Alert: High temperature detected!",
 "YOUR_PHONE_NUMBER");
 }
 }
 float getHeartRate(long irValue) {
 // Calculate heart rate using the heartRate library
 byte rateValue;
 static byte rates[4];
 static byte rateSpot = 0;
 static long lastBeat = 0;
 if (checkForBeat(irValue)) {
 long delta = millis()- lastBeat;
 lastBeat = millis();
 float beatsPerMinute = 60 / (delta / 1000.0);
 if (beatsPerMinute < 255 && beatsPerMinute > 20) {
 rates[rateSpot++] = (byte)beatsPerMinute;
 rateSpot %= 4;
 // Take average of readings
 float avgBPM = 0;
 for (byte x = 0; x < 4; x++)
 avgBPM += rates[x];
 avgBPM /= 4;
 return avgBPM;
 }
 }
return 0;
}





