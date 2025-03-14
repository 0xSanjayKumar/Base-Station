#include <Arduino.h>
#include <WiFi.h> // for the WiFi Access Point
#include <WebServer.h> 
#include <SPIFFS.h> // for the web server
#include <SoftwareSerial.h> // for RF module
#include <HardwareSerial.h> // for GPS device
#include <SparkFun_u-blox_GNSS_v3.h> // GNSS library
#include "serial_readnonblocking.h" // to send the command cmd to the GPS device
#include <U8g2lib.h> // for the OLED display

// U8g2 Contructor
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// U8g2 display object
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Hardware I2C with HW Slave Address


// Set up the ESP32 as a WiFi Access Point
const char* ssid = "Base_Station";
const char* password = "123456789";

HardwareSerial RF_Beitian(1);
HardwareSerial GPS_device(2);
WebServer server(80);

SFE_UBLOX_GNSS_SERIAL GNSS_device; // Create an instance of the GNSS object
bool response;

String button ="";
byte gpsByte, serialByte;
char blue_buf;
long lastTime = 0;
unsigned long previousMillis = 0;
const unsigned long interval = 10;
int gps_state = 0;
uint32_t observationTime;
float requiredAccuracy;
int p1 = 0; // parity bit 1
int p2 = 0; // parity bit 2

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open index.html");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  String contentType = "text/plain";
  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".png")) contentType = "image/png";
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }
  server.streamFile(file, contentType);
  file.close();
}

void handleControl() {
  button = server.arg("command");

  String latitude = server.arg("latitude");
  String longitude = server.arg("longitude");
  String altitude = server.arg("altitude");
  String accuracy = server.arg("accuracy");
  String time = server.arg("time");

  if (button == "Update") {
    Serial.println("Update command received");
    Serial.println("Latitude: " + latitude);
    Serial.println("Longitude: " + longitude);
    Serial.println("Altitude: " + altitude);
    Serial.println("Accuracy: " + accuracy);
    Serial.println("Time: " + time);
  } else if (button == "Stop") {
  } else if (button == "Auto-Survey") {
    observationTime = time.length() > 0 ? atoi(time.c_str()) : 200;
    requiredAccuracy = accuracy.length() > 0 ? atof(accuracy.c_str()) : 2.0;
  } else if (button == "Auto-Fix") {
  } else if (button == "Reset") {
  } else if (button == "Port") {
  } else if (button == "MSG-Enable") {
  } else if (button == "Survey_Stat") {
  } else if (button == "Surveying") {
    observationTime = time.length() > 0 ? atoi(time.c_str()) : 200;
    requiredAccuracy = accuracy.length() > 0 ? atof(accuracy.c_str()) : 2.0;

  } else if (button == "Broadcast") {
  } else if (button == "Status") {
  } else if (button == "Fix-Mode"){
  }

  server.send(200, "text/plain", "Command received");
}


void newRTCM1005(RTCM_1005_data_t *rtcmStruct)
{
  double x = rtcmStruct->AntennaReferencePointECEFX;
  x /= 10000.0; // Convert to m
  double y = rtcmStruct->AntennaReferencePointECEFY;
  y /= 10000.0; // Convert to m
  double z = rtcmStruct->AntennaReferencePointECEFZ;
  z /= 10000.0; // Convert to m

}

void resetGPS(){
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print(F("Reset, Requested"));
  GNSS_device.setRTCM1005callbackPtr(nullptr);
  GNSS_device.factoryDefault();
  delay(5000);
  u8g2.setCursor(0, 30);
  u8g2.print(F("GPS Reseted"));
  u8g2.sendBuffer();
  gps_state = 1;
}

void portConfig(){
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print(F("Port, Configuring"));
  GNSS_device.setUART1Output(COM_TYPE_UBX | COM_TYPE_NMEA | COM_TYPE_RTCM3);  // Ensure RTCM3 is enabled
  GNSS_device.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);                     //Save the communications port settings to flash and BBR
  u8g2.setCursor(0, 30);
  u8g2.print(F("GPS Port Configured"));
  u8g2.sendBuffer();
  gps_state = 2;
}

void msg_enable(){
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print(F("MSG-Enable, Requested"));
  response = GNSS_device.newCfgValset();                                         // Create a new Configuration Item VALSET message
  response &= GNSS_device.addCfgValset8(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART1, 1);  //Enable message 1005 to output through I2C port, message every second
  response &= GNSS_device.addCfgValset8(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_UART1, 1);
  response &= GNSS_device.addCfgValset8(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_UART1, 1);
  response &= GNSS_device.addCfgValset8(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_UART1, 1);
  response &= GNSS_device.addCfgValset8(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_UART1, 1);
  response &= GNSS_device.addCfgValset8(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART1, 10);  // Enable message 1230 every 10 seconds
  response &= GNSS_device.sendCfgValset();  
  u8g2.setCursor(0, 30);

  if (response == true) {
    u8g2.print(F("Message, Enabled"));
    gps_state = 3;
  } else {
    u8g2.print(F("Message, Failed"));

  }
  u8g2.sendBuffer();
}

void survey_stat(){
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print(F("Survey Status, Requested"));
  response = GNSS_device.getSurveyStatus(2000);  //Query module for SVIN status with 2000ms timeout (request can take a long time)
  Serial.println(response);
  u8g2.setCursor(0, 30);
  if (response == false) {  // Check if fresh data was received
    u8g2.print(F("Survey Status,"));
    u8g2.setCursor(0, 45);
    u8g2.print(F("Fail"));
  } else {
    u8g2.print(F("Survey Status,"));
    u8g2.setCursor(0, 45);
    u8g2.print(F("Success"));
    gps_state = 4;
    p1 = 0;
  }
  u8g2.sendBuffer();
}

void surveying() { 
  if (GNSS_device.getSurveyInActive() == true) {  // Use the helper function
    u8g2.clearBuffer();
    u8g2.setCursor(0, 15);
    u8g2.print(F("Surveying, In Progress"));
    u8g2.sendBuffer();
  } else {
    response = GNSS_device.enableSurveyModeFull(observationTime, requiredAccuracy, VAL_LAYER_RAM);  //Enable Survey in. Save setting in RAM layer only (not BBR)

    if (response == false) {
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      Serial.println("Survey, failed");
      u8g2.print(F("Survey, failed"));
      u8g2.sendBuffer();
      return;
    } else {
      if (p1 == 0) {
        u8g2.clearBuffer();
        u8g2.setCursor(0, 15);
        u8g2.print(F("Survey Started"));
        u8g2.setCursor(0, 30);
        u8g2.print(F("T: "));
        u8g2.print(observationTime);
        u8g2.print(F("s, Acc: "));
        u8g2.print(requiredAccuracy, 2);
        u8g2.print(F("m"));
        u8g2.setCursor(0, 45);
        u8g2.print(F("Satellite, Surveying: "));
        u8g2.print(GNSS_device.getSIV());
        u8g2.sendBuffer();
        delay(2000);
        u8g2.clearBuffer();
        p1 = 1;
      }
    }
  }
  // Check survey status
  while (GNSS_device.getSurveyInValid() == false) {  // Call the helper function
    server.handleClient();
    response = GNSS_device.getSurveyStatus(2000);  
    
    if ((button == "Stop") || (GNSS_device.getSurveyInMeanAccuracy() < requiredAccuracy)) {  
      // Stop survey mode
      gps_state = 5;
      int i = 0;
      response= GNSS_device.disableSurveyMode();  // Fully disable survey mode
      Serial.println(response);
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      u8g2.print(F("Survey, Stopped"));
      u8g2.sendBuffer();
      
      // Reset button state
      button = "";
      Serial.println("GPS Survey Stopped");
      delay(1000);
      
      GNSS_device.enableSurveyModeFull(0, 0, VAL_LAYER_RAM);
    
      return;  // Exit the function after stopping the survey
    }
    
    if (response == true) {  // Check if fresh data was received
      u8g2.setDrawColor(0); // Set draw color to black to overwrite previous text
      u8g2.drawBox(0, 15, 128, 60); // Clear the area where text and progress bar are drawn
      u8g2.setDrawColor(1); // Set draw color back to white

      u8g2.setCursor(0, 15);
      u8g2.print(F("Surveying, In Progress"));
      u8g2.setCursor(0, 30);
      u8g2.print(F("Press stop to end"));
      u8g2.setCursor(0, 45);
      u8g2.print(F("T: "));
      u8g2.print(GNSS_device.getSurveyInObservationTimeFull());
      u8g2.print(F(" s, Acc: "));
      u8g2.print(GNSS_device.getSurveyInMeanAccuracy());
      u8g2.print(F("m"));

      // Calculate the progress
      unsigned long startTime = millis() -  GNSS_device.getSurveyInObservationTimeFull()*1000;
      unsigned long elapsedTime = millis() - startTime;
      float progress = (float)elapsedTime / (observationTime * 1000) * 100;
      if (progress > 100) progress = 100;

      // Show the progress bar
      u8g2.drawFrame(0, 50, 128, 10); 
      u8g2.drawBox(0, 50, (int)(progress * 1.28), 10); 
      u8g2.sendBuffer();
    } else{
      Serial.println("Reply,SVIN_request_failed,end");
      return;
    }
  }
  return;
}
void broadcast(){
  u8g2.clearBuffer();
  u8g2.setCursor(0, 15);
  u8g2.print(F("Broadcasting, Requested"));
  GNSS_device.setUART1Output(COM_TYPE_UBX | COM_TYPE_RTCM3);
  GNSS_device.setRTCM1005callbackPtr(&newRTCM1005); //Set up the callback
  u8g2.setCursor(0, 30);
  u8g2.print(F("Broadcasting......"));
  u8g2.sendBuffer();
  //gps_state = 6;
}

void status(){
  if (millis() - lastTime > 1000) {
    lastTime = millis();  //Update the timer
    long latitude = GNSS_device.getLatitude();
    u8g2.clearBuffer();
    u8g2.setCursor(0, 15);
    u8g2.print(F("Latitude: "));
    u8g2.print(latitude);
    long longitude = GNSS_device.getLongitude();
    u8g2.setCursor(0, 30);
    u8g2.print(F("Longitude: "));
    u8g2.print(longitude);
    long altitude = GNSS_device.getAltitude();
    u8g2.setCursor(0, 45);
    u8g2.print(F("Altitude: "));
    u8g2.print(altitude);
    byte fixType = GNSS_device.getFixType();
    u8g2.setCursor(0, 60);
    u8g2.print(F("Fix Type: "));
    if (fixType == 0) u8g2.print(F("No fix"));
    else if (fixType == 1) u8g2.print(F("Dead reckoning"));
    else if (fixType == 2) u8g2.print(F("2D"));
    else if (fixType == 3) u8g2.print(F("3D"));
    else if (fixType == 4) u8g2.print(F("GNSS + Dead reckoning"));
    else if (fixType == 5) u8g2.print(F("Time only"));
    u8g2.sendBuffer();

    byte RTK = GNSS_device.getCarrierSolutionType();
    Serial.print("RTK:");
    Serial.println(RTK);
    if (RTK == 0) Serial.println(("Reply,Status,Fix,0"));
    else if (RTK == 1) Serial.println(("Reply,Status,Fix,1"));
    else if (RTK == 2) Serial.println(("Reply,Status,Fix,2"));
  }
}

void fixMode(){
  int32_t set_lat = atoi(command[2]);
  int8_t set_latHP = atoi(command[3]);
  int32_t set_lon = atoi(command[4]);
  int8_t set_lonHP = atoi(command[5]);
  int32_t set_alt = atoi(command[6]);
  int8_t set_altHP = atoi(command[7]);
  int8_t set_Accuracy = atoi(command[8]);
  GNSS_device.setVal32(0x20030001, 2, VAL_LAYER_RAM);//2 for Tmode3
  delay(200);
  GNSS_device.setVal32(0x20030002, 1, VAL_LAYER_RAM);//change config to LLH
  delay(200);
  GNSS_device.setVal32(0x40030009,  set_lat,  VAL_LAYER_RAM);//latitude
  GNSS_device.setVal32(0x2003000c,  set_latHP, VAL_LAYER_RAM);//latitude HP (-99 to 99) deg
  GNSS_device.setVal32(0x4003000a,  set_lon,  VAL_LAYER_RAM);//longitude
  GNSS_device.setVal32(0x2003000d,  set_lonHP,  VAL_LAYER_RAM);//longitude HP (-99 to 99) deg
  GNSS_device.setVal32(0x4003000b,  set_alt,  VAL_LAYER_RAM);//altitude
  GNSS_device.setVal32(0x2003000e,  set_altHP,  VAL_LAYER_RAM);//altitude HP (-99 to 99)mm
  GNSS_device.setVal32(0x4003000f,  set_Accuracy, VAL_LAYER_RAM);//accuracy
  GNSS_device.saveConfiguration(); //Save the current settings to flash and BBR

}

void setup() {
  u8g2.begin(); // initialize the OLED display
  Serial.begin(115200); // Start the serial communication
  GPS_device.begin(38400); // Start the serial communication with the GPS device
  RF_Beitian.begin(38400); // Start the serial communication with the RF module
  GNSS_device.connectedToUART2(); // Connect the GNSS object to the hardware serial port
  
  while(!GNSS_device.begin(GPS_device)) {
    Serial.println(F("Unable to communicate with u-blox GNSS module."));
    GNSS_device.setUART2Output(COM_TYPE_UBX); // Set the GNSS module to output UBX only
    Serial.println(F("Freezing..."));
    delay(1000);
  }
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  u8g2.clearBuffer(); 
  u8g2.setFont(u8g2_font_ncenB08_tr);	
  int x_center = (u8g2.getDisplayWidth() - u8g2.getStrWidth("MERAQUE")) / 2;
  int y_center = (u8g2.getDisplayHeight() + u8g2.getMaxCharHeight()) / 2;
  u8g2.drawStr(x_center, y_center, "MERAQUE");
  u8g2.sendBuffer();

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.serveStatic("/index.html", SPIFFS, "/index.htmll");
  server.serveStatic("/styles.css", SPIFFS, "/styles.css");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/logo.png", SPIFFS, "/logo.png");
  server.begin();
  Serial.println("HTTP server started");
}
void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();
  // Check if the interval has elapsed since the last execution.
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the current time as the new reference point.

    // Check for the arrival of new data and process it.
    GNSS_device.checkUblox();

    // Check if any callbacks are waiting to be processed.
    GNSS_device.checkCallbacks();

    // Your other code and logic can go here.
  }
  

  if (button == ""){
    return;
  }


  if (button == "Stop") { 

  }
  if (button == "Auto-Survey") { 
    //gps_state = 0;
    Serial.println("Auto-Survey Button Pressed");
    resetGPS();
    delay(1000);
    portConfig();
    delay(1000);
    msg_enable();
    delay(1000);
    survey_stat();
    delay(1000);
    surveying();
    delay(1000);
    broadcast();
    delay(1000);
    status();
  
    button = "";
  }

  if (button == "Auto-Fix") { 
    gps_state = 0;
    Serial.println("Auto-Fix Button Pressed");
    resetGPS();
    delay(1000);
    portConfig();
    delay(1000);
    msg_enable();
    delay(1000);
    fixMode();
    delay(1000);
    broadcast();
    delay(1000);
    status();
    button = "";
  }

  if (button == "Reset") { 
    resetGPS();
    button = "";
  }

  else if (button == "Port") { 
    if(gps_state == 1){
      Serial.println("Port Button Pressed");
      portConfig();
    }
    else{
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      u8g2.print(F("GPS not reseted"));
      u8g2.sendBuffer();
    }
    button = "";
  }

  else if (button == "MSG-Enable") { 
    if(gps_state == 2){
      Serial.println("MSG-Enable Button Pressed");
      msg_enable();
    }
    else{
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      u8g2.print(F("GPS not configured"));
      u8g2.sendBuffer();
    }
    button = "";
  }

  else if (button == "Survey_Stat") { 
    if(gps_state == 3){
      Serial.println("Survey Status Button Pressed");
      survey_stat();
    }
    else{
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      u8g2.print(F("GPS not configured"));
      u8g2.sendBuffer();
    }
    button = "";
  }

  else if (button == "Surveying") { 
    if(gps_state == 4){
      GNSS_device.disableSurveyMode();
      surveying();


    }

    else{
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      u8g2.print(F("GPS not configured"));
      u8g2.sendBuffer();
    }  
  }

  else if (button == "Broadcast") { 
    Serial.println("Broadcast Button Pressed");
    broadcast();
    // if(gps_state == 5){
    // }
    // else{
    //   u8g2.clearBuffer();
    //   u8g2.setCursor(0, 15);
    //   u8g2.print(F("GPS not Broadcasting"));
    //   u8g2.sendBuffer();
    // }
    button = "";

  }

  else if (button = "Status") { 
    Serial.println("Status Button Pressed");
    status();
    // if(gps_state == 6){
    // }
    // else{
    //   u8g2.clearBuffer();
    //   u8g2.setCursor(0, 15);
    //   u8g2.print(F("GPS not configured"));
    //   u8g2.sendBuffer();
    // }
    button = "";
  }
  
  else if (button = "Fix-Mode") { 
    if(gps_state == 3){
      Serial.println("Fix-Mode Button Pressed");

    }
    else{
      u8g2.clearBuffer();
      u8g2.setCursor(0, 15);
      u8g2.print(F("GPS not configured"));
      u8g2.sendBuffer();
    }
    button = "";
  }

}