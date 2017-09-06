//#define USE_SDFAT
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#ifdef USE_SDFAT
#include <SdFat.h>
#endif
#include <AsyncJson.h>
#include <Adafruit_PN532Ex.h>
#include <amiibo.h>
#include <amiitool.h>
#include <spiffs/spiffs_config.h> // For SPIFFS_OBJ_NAME_LEN
#include "index_htm_gz.h"

#define DBG_OUTPUT_PORT Serial

const char* sdPrefix = "SD:";
const char* spiffsPrefix = "SPIFFS:";

#ifdef USE_SDFAT
SdFat sd;
bool sdStarted = false;
const int SDChipSelect = D8;
#endif

const char* keyfilename = "/key_retail.bin";

Adafruit_PN532Ex pn532(D4);

volatile bool triggerReadNFC = false;
volatile bool triggerWriteNFC = false;
volatile bool triggerDummyWriteNFC = false;
amiitool atool((char*)keyfilename);

char newWifiSSID[32] = "";
char newWifiPasskey[32] = "";
volatile bool reconnectWifi = false;

volatile bool scanWifi = false;
unsigned long lastWifiScanMillis = 0;
String wifiScanJSON;

const char * hostName = "wifiibo";
const char* http_username = "admin";
const char* http_password = "admin";

volatile bool shouldReboot = false;

const char* versionStr = "1.0";

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket websocket("/ws");
AsyncEventSource events("/events");

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    DBG_OUTPUT_PORT.printf("ws[%s][%u] connect\n", server->url(), client->id());
    //client->printf("Hello Client %u :)", client->id());
    //client->ping();
    char ip[24];
    WiFi.localIP().toString().toCharArray(ip, 24);
    client->printf("{\"status\":\"Connected\",\"serverip\":\"%s\",\"version\":\"%s\"}", ip, versionStr);
    checkRetailKey();
  } else if(type == WS_EVT_DISCONNECT){
    DBG_OUTPUT_PORT.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    DBG_OUTPUT_PORT.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    DBG_OUTPUT_PORT.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      DBG_OUTPUT_PORT.printf("%s\n",msg.c_str());

      //if(info->opcode == WS_TEXT)
      //  client->text("I got your text message");
      //else
      //  client->binary("I got your binary message");
      parseClientJSON(server, client, info, msg);
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      DBG_OUTPUT_PORT.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        DBG_OUTPUT_PORT.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          DBG_OUTPUT_PORT.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          //if(info->message_opcode == WS_TEXT)
          //  client->text("I got your text message");
          //else
          //  client->binary("I got your binary message");
          parseClientJSON(server, client, info, msg);
        }
      }
    }
  }
}

// This function modifies the "path" string!
// Returns true if file path is SPIFFS, false if SD
bool isSPIFFSFile(String path)
{
  if (path.substring(0, strlen(sdPrefix)-1).equalsIgnoreCase(sdPrefix))
  {
    path.remove(0, strlen(sdPrefix));
    return false;
  }
  else if (path.substring(0, strlen(spiffsPrefix)-1).equalsIgnoreCase(spiffsPrefix))
  {
    path.remove(0, strlen(spiffsPrefix));
    return true;
  }

  return true;
}

bool dummyWriteTag() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID
  int8_t tries;
    
  // Wait for an NTAG215 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  sendStatusCharArray("Place blank tag on reader...");
  success = pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    //DBG_OUTPUT_PORT.println();
    DBG_OUTPUT_PORT.print(F("Tag UID: "));
    amiitool::printData(uid, uidLength, 16, false, false);
    //PrintHexShort(uid, uidLength);
    //DBG_OUTPUT_PORT.println();
    
    if (uidLength == 7)
    {
      if (atool.encryptLoadedFile(uid) >= 0)
      {
        sendStatusCharArray("Encrypted file, (dummy) write complete.");
        DBG_OUTPUT_PORT.println("Dummy wrote tag.");
        DBG_OUTPUT_PORT.println("Decrypted tag data:");
        amiitool::printData(atool.modified, NFC3D_AMIIBO_SIZE, 16, true, false);
        DBG_OUTPUT_PORT.println("Encrypted tag data:");
        amiitool::printData(atool.original, NTAG215_SIZE, 4, true, false);
      }
    }
  }

  //DBG_OUTPUT_PORT.println();
  //DBG_OUTPUT_PORT.println(F("Please remove your Amiibo from the reader."));
  //countdown("New attempt in", "Ready to read.");
  return true;
}

void parseClientJSON(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsFrameInfo * info, String msg)
{
  //DBG_OUTPUT_PORT.printf("[%u] get Text: %s\n", num, payload);
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(msg);
  
  // Test if parsing succeeds.
  if (!root.success()) {
    DBG_OUTPUT_PORT.println("parseObject() failed");
    return;
  }
  else
  {
    if (root.containsKey("func"))
    {
      if (!strncmp("readnfc", root["func"], 7))
      {
        DBG_OUTPUT_PORT.println("Triggering NFC read");
        triggerReadNFC = true;
      }
      else if (!strncmp("cancelread", root["func"], 10))
      {
        DBG_OUTPUT_PORT.println("Canceling NFC read");
        atool.cancelNFCRead();
      }
      else if (!strncmp("writenfc", root["func"], 8))
      {
        DBG_OUTPUT_PORT.println("Triggering NFC write");
        triggerWriteNFC = true;
      }
      else if (!strncmp("cancelwrite", root["func"], 11))
      {
        DBG_OUTPUT_PORT.println("Canceling NFC write");
        atool.cancelNFCWrite();
      }
      else if (!strncmp("dummywritenfc", root["func"], 13))
      {
        DBG_OUTPUT_PORT.println("Triggering dummy NFC write");
        triggerDummyWriteNFC = true;
      }
      else if (!strncmp("getfileinfo", root["func"], 11))
      {
        DBG_OUTPUT_PORT.println("Get file info");
        getAmiiboInfo(root["filename"]);
      }
      else if (!strncmp("deleteamiibo", root["func"], 12))
      {
        DBG_OUTPUT_PORT.println("Delete amiibo");
        deleteFile(root["filename"]);
      }
      else if (!strncmp("saveamiibo", root["func"], 10))
      {
        DBG_OUTPUT_PORT.println("Save amiibo");
        saveAmiibo(root["filename"]);
      }
      else if (!strncmp("listamiibo", root["func"], 10))
      {
        DBG_OUTPUT_PORT.println("List amiibo");
        String outStr;
        getAmiiboList(&outStr);
        client->text(outStr);
      }
      else if (!strncmp("configurewifi", root["func"], 13))
      {
        DBG_OUTPUT_PORT.println("Configure WiFi");
        if (root.containsKey("ssid")) {
          if (root.containsKey("passkey")) {
            strcpy(newWifiPasskey, root["passkey"]);
          }
          strcpy(newWifiSSID, root["ssid"]);
          reconnectWifi = true;
        }
      }
      else if (!strncmp("triggerScanWifi", root["func"], 13))
      {
        DBG_OUTPUT_PORT.println("Scan WiFi");
        scanWifi = true;
      }
    }
    else
    {
      DBG_OUTPUT_PORT.println("no func found");
    }
  }
}

void checkRetailKey()
{
  if (!atool.tryLoadKey())
  {
    String json = "{";
      json += "\"";
      json += "invalidkey";
      json += "\":\"";
      json += keyfilename;
      json += "\"";
      json += "}";
    //webSocket.broadcastTXT(json);
    websocket.textAll(json);
  }
}

void sendTagInfo()
{  
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& taginfo = root.createNestedObject("taginfo");
  taginfo["name"] = String(atool.amiiboInfo.amiiboName);
  taginfo["miiName"] = String(atool.amiiboInfo.amiiboOwnerMiiName);
  taginfo["id"] = String(atool.amiiboInfo.amiiboHeadChar) + String(atool.amiiboInfo.amiiboTailChar);
  sendJSON(&root);
}

void sendJSON(JsonObject *root)
{
  String json;
  root->printTo(json);
  //DBG_OUTPUT_PORT.println(json);
  websocket.textAll(json);
}

void sendFunctionStatusCode(char *funcName, int code)
{
  DBG_OUTPUT_PORT.print("Status: ");
  DBG_OUTPUT_PORT.print(funcName);
  DBG_OUTPUT_PORT.print(": ");
  DBG_OUTPUT_PORT.print(code);
  String json = "{";
    json += "\"";
    json += funcName;
    json += "\":\"";
    json += code;
    json += "\"";
    json += "}";
  //webSocket.broadcastTXT(json);
  websocket.textAll(json);
}

void sendStatusCharArray(char *statusmsg)
{
  DBG_OUTPUT_PORT.println(statusmsg);
  String json = "{";
    json += "\"status\":\"";
    json += statusmsg;
    json += "\"";
    json += "}";
  //webSocket.broadcastTXT(json);
  websocket.textAll(json);
}

void sendStatus(String statusmsg)
{
  DBG_OUTPUT_PORT.println(statusmsg);
  String json = "{";
    json += "\"status\":\"";
    json += statusmsg;
    json += "\"";
    //json += ", \"analog\":"+String(analogRead(A0));
    //json += ", \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
  //webSocket.broadcastTXT(json);
  websocket.textAll(json);
}

void updateProgress(int percent)
{
  DBG_OUTPUT_PORT.print(percent);
  DBG_OUTPUT_PORT.println("%");
  String json = "{";
    json += "\"progress\":\"";
    json += String(percent);
    json += "\"";
    json += "}";
  //webSocket.broadcastTXT(json);
  websocket.textAll(json);
}

void handleNFC()
{
  if (triggerReadNFC) {
    triggerReadNFC = false;
    DBG_OUTPUT_PORT.printf("Reading NFC tag...");
    bool result = atool.readTag(sendStatusCharArray, updateProgress);
    if (result)
    {
      sendTagInfo();
    }
  }
  else if (triggerWriteNFC) {
    triggerReadNFC = false;
    DBG_OUTPUT_PORT.printf("Writing NFC tag...");
    bool result = atool.writeTag(sendStatusCharArray, updateProgress);
    //bool result = dummyWriteTag();
    if (result)
    {
      sendStatusCharArray("Tag successfully written.");
      //sendTagInfo();
    }
  }
  else if (triggerDummyWriteNFC)
  {
    triggerDummyWriteNFC = false;
    DBG_OUTPUT_PORT.printf("(Dummy) Writing NFC tag...");
    //bool result = atool.writeTag(sendStatusCharArray, updateProgress);
    bool result = dummyWriteTag();
    if (result)
    {
      sendStatusCharArray("Tag successfully (dummy) written.");
      //sendTagInfo();
    }
  }

  triggerReadNFC = false;
  triggerWriteNFC = false;
  triggerDummyWriteNFC = false;
}

bool getAmiiboInfo(String filename)
{
  int loadResult = 0;
  fs::File f = SPIFFS.open(filename, "r");
  
  if (f)
  {
    loadResult = atool.loadFileSPIFFS(&f, true);
    f.close();
  }
  
  if (loadResult >= 0)
  {
    sendTagInfo();
  }

  return loadResult >= 0;
}

void getAmiiboList(String *outStr)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& tagInfo = root.createNestedArray("tagInfoList");
  uint8_t bin[AMIIBO_HEAD_LEN+AMIIBO_TAIL_LEN];
  char id[(AMIIBO_HEAD_LEN+AMIIBO_TAIL_LEN)*2+1];

  fs::Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    if (dir.fileName().endsWith(".bin"))
    {
      //DBG_OUTPUT_PORT.print("Opening file ");
      //DBG_OUTPUT_PORT.println(dir.fileName());
      fs::File f = dir.openFile("r");
      if (f && (f.size() == NTAG215_SIZE)) {
        //DBG_OUTPUT_PORT.print("Reading file ");
        //DBG_OUTPUT_PORT.println(dir.fileName());

        JsonObject& tag = tagInfo.createNestedObject();
        
        tag["filename"] = dir.fileName();
        
        f.seek(AMIIBO_ENC_CHARDATA_OFFSET, fs::SeekSet);
        f.readBytes((char*)bin, AMIIBO_HEAD_LEN+AMIIBO_TAIL_LEN);

        for (int i = 0; i < AMIIBO_HEAD_LEN+AMIIBO_TAIL_LEN; i++)
        {
          sprintf(id+(i*2), "%02x", bin[i]);
        }

        tag["id"] = String(id);
      }
      f.close();
    }
  }

  //String json;
  //root.prettyPrintTo(json);
  //DBG_OUTPUT_PORT.println(json);

  if (outStr != NULL)
    root.printTo(*outStr);
}

void getAmiiboList2(String *outStr)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& tagInfo = root.createNestedArray("tagInfoList");
  int loadResult = 0;

  fs::Dir dir = SPIFFS.openDir("/");
  while (dir.next())
  {
    if (dir.fileName().endsWith(".bin"))
    {
      DBG_OUTPUT_PORT.print("Opening file ");
      DBG_OUTPUT_PORT.println(dir.fileName());
      fs::File f = dir.openFile("r");

      if (f)
      {
        loadResult = atool.loadFileSPIFFS(&f, true);
        f.close();
      }
      
      if (loadResult >= 0)
      {
        JsonObject& tag = tagInfo.createNestedObject();
      
        tag["filename"] = dir.fileName();
        tag["name"] = String(atool.amiiboInfo.amiiboName);
        tag["miiName"] = String(atool.amiiboInfo.amiiboOwnerMiiName);
        tag["id"] = String(atool.amiiboInfo.amiiboHeadChar) + String(atool.amiiboInfo.amiiboTailChar);
      }
    }
  }
  
  //String json;
  //root.prettyPrintTo(json);
  //DBG_OUTPUT_PORT.println(json);

  if (outStr != NULL)
    root.printTo(*outStr);
}

void deleteFile(String filename) {
  if (SPIFFS.remove(filename))
    sendStatusCharArray("amiibo deleted.");
  else
    sendStatusCharArray("Failed to delete amiibo.");
}

void getWifiStatusInfo(String *outStr, bool scan) {
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["wifistatus"] = WiFi.status();
  if (scan) {
    JsonArray& scanresults = root.createNestedArray("scanresults");
    int numNetworks = WiFi.scanNetworks(false, true);
    for (int i = 0; i < numNetworks; i++) {
      JsonObject& network = scanresults.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      //network["channel"] = WiFi.channel(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryptionType"] = WiFi.encryptionType(i);
    }
  }

  //String json;
  //root.prettyPrintTo(json);
  //DBG_OUTPUT_PORT.println(json);

  if (outStr != NULL)
    root.printTo(*outStr);
}

void saveAmiibo(String filename) {
  int bytesOut = 0;

  if (filename.length() > SPIFFS_OBJ_NAME_LEN - 1) {
    sendFunctionStatusCode("saveAmiibo", -1);
    sendStatusCharArray("Failed to save file: filename too long.");
  }
  else {
    if (SPIFFS.exists(filename)) {
      sendFunctionStatusCode("saveAmiibo", -2);
      sendStatusCharArray("Failed to save file: file exists already.");
    }
    else {
      fs::File f = SPIFFS.open(filename, "w");
      if (!f) {
        sendFunctionStatusCode("saveAmiibo", -3);
        sendStatusCharArray("Failed to save file: failed to open file for writing.");
      }
      else {
        if ((bytesOut = f.write(atool.original, NTAG215_SIZE)) != NTAG215_SIZE) {
          sendFunctionStatusCode("saveAmiibo", -4);
          sendStatusCharArray("Failed to save file: failed to write data.");
        }
        else {
          if (getAmiiboInfo(filename)) {
            sendFunctionStatusCode("saveAmiibo", 1);
            sendStatusCharArray("File saved successfully.");
          }
          else {
            // Delete file?
            sendFunctionStatusCode("saveAmiibo", -5);
            sendStatusCharArray("File error: unable to decrypt amiibo data.");
          }
        }
      }
    }
  }
}

void handleReconnectWifi(bool force) {
  if (reconnectWifi || force) {
    reconnectWifi = false;
    unsigned long wifiTrySeconds = 5;

    DBG_OUTPUT_PORT.printf("Switching to STA mode.");

    if (!force) {
      WiFi.disconnect(false);
      delay(500);
    }
    
    WiFi.mode(WIFI_STA);
    
    if (strlen(newWifiSSID)) {
      WiFi.begin(newWifiSSID, newWifiPasskey);   
    }
    else {
      WiFi.begin();
    }

    unsigned long wifiStartTime = millis();
    
    do
    {
      if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        DBG_OUTPUT_PORT.printf("STA: Failed!\n");
        WiFi.printDiag(Serial);
        WiFi.disconnect(false);
        delay(500);
        WiFi.begin();
      }
      else
      {
        break;
      }
    } while ((millis() - wifiStartTime) < (wifiTrySeconds * 1000));

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      DBG_OUTPUT_PORT.printf("Wifi connection failed, going back to AP mode.");
      WiFi.disconnect(false);
      delay(500);
      WiFi.mode(WIFI_STA);
      if (!WiFi.softAP(hostName)) {
        DBG_OUTPUT_PORT.printf("Error starting AP.");
      }
    }
  }
}

void handleRootRequest(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm_gz, index_htm_gz_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void setup(){
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  
  WiFi.hostname(hostName);

  handleReconnectWifi(true);
  /*
  unsigned long wifiTrySeconds = 5;
  unsigned long wifiStartTime = millis();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hostName);
  //WiFi.begin(ssid, password);
  WiFi.begin();
  while ((millis() - wifiStartTime) > wifiTrySeconds * 1000)
  {
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      DBG_OUTPUT_PORT.printf("STA: Failed!\n");
      WiFi.disconnect(false);
      delay(500);
      WiFi.begin();
    }
    else
    {
      break;
    }
  }

  wifiStartTime = millis();
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    DBG_OUTPUT_PORT.printf("Switching to STA mode.");
    WiFi.disconnect(false);
    WiFi.mode(WIFI_STA);
    delay(500);
    WiFi.begin();
    
    while ((millis() - wifiStartTime) > wifiTrySeconds * 1000)
    {
      if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        DBG_OUTPUT_PORT.printf("STA: Failed!\n");
        WiFi.disconnect(false);
        delay(500);
        WiFi.begin();
      }
      else
      {
        break;
      }
    }
  }
  */

  //Send OTA events to the browser
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();

  MDNS.addService("http","tcp",80);

  SPIFFS.begin();

#ifdef USE_SDFAT
  sdStarted = sd.begin(SDChipSelect, SD_SCK_MHZ(50));
  if (!sdStarted) {
    DBG_OUTPUT_PORT.printf("No SD card found.\n");
    //sd.initErrorHalt();
  }
#endif

  websocket.onEvent(onWsEvent);
  server.addHandler(&websocket);

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(http_username,http_password));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/getAmiiboList", HTTP_GET, [](AsyncWebServerRequest *request){
    String outStr;
    getAmiiboList(&outStr);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", outStr);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/getWifiStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    String outStr;
    getWifiStatusInfo(&outStr, false);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", outStr);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/triggerScanWifi", HTTP_GET, [](AsyncWebServerRequest *request){
    scanWifi = true;
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/getWifiScanResults", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", wifiScanJSON);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  server.on("/uploadAmiibo", HTTP_POST, [](AsyncWebServerRequest *request){
    DBG_OUTPUT_PORT.println("/uploadAmiibo, POST");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  },
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static fs::File fsUploadFile;
    static size_t fileSize = 0;
    static bool fileOK = true;
    static bool isKey = false;

    if (!index) { // Start
        DBG_OUTPUT_PORT.printf("handleFileUpload Name: %s\r\n", filename.c_str());
        DBG_OUTPUT_PORT.printf("handleFileUpload len: %d\r\n", len);
        if (!filename.startsWith("/")) filename = "/" + filename;
        if (filename.length() > SPIFFS_OBJ_NAME_LEN - 1) {
          sendFunctionStatusCode("saveAmiibo", -1);
          sendStatusCharArray("Failed to save file: filename too long.");
          fileOK = false;
        }
        else if ((filename == keyfilename) && (len == 160)) {
          DBG_OUTPUT_PORT.println("Uploading retail key.");
          isKey = true;
          fileOK = true;
        }
        else if (SPIFFS.exists(filename)) {
          sendFunctionStatusCode("saveAmiibo", -2);
          sendStatusCharArray("Failed to save file: file exists already.");
          fileOK = false;
        }
        else {
          fileOK = true;
        }

        if (fileOK) {
          fsUploadFile = SPIFFS.open(filename, "w");
          DBG_OUTPUT_PORT.printf("First upload part.\r\n");
        }
        else {
          if (fsUploadFile) {
            fsUploadFile.close();
          }
        }
    }
    if (fsUploadFile) { // Continue
        DBG_OUTPUT_PORT.printf("Continue upload part. Size = %u\r\n", len);
        if (fsUploadFile.write(data, len) != len) {
            DBG_OUTPUT_PORT.println("Write error during upload");
        } else
            fileSize += len;
    }
    if (final) { // End
        bool finalOK = false;
        if (fsUploadFile) {
            fsUploadFile.close();
            finalOK = true;
        }
        DBG_OUTPUT_PORT.printf("handleFileUpload Size: %u\n", fileSize);
        fileSize = 0;

        if (isKey) {
          if (atool.tryLoadKey()) {
            sendStatusCharArray("Key upload complete.");
            String json = "{";
              json += "\"";
              json += "validkey";
              json += "\":\"";
              json += keyfilename;
              json += "\"";
              json += "}";
            websocket.textAll(json);
          }
          else
            sendStatusCharArray("Invalid key file.");
        }
        else if (finalOK) {
          sendStatusCharArray("amiibo upload complete.");
          getAmiiboInfo(filename);
        }
        else {
          //sendStatusCharArray("Unknown error during amiibo upload.");
        }
    }
  });
  

  server.on("/getAmiiboList2", HTTP_GET, [](AsyncWebServerRequest *request){
    String outStr;
    getAmiiboList2(&outStr);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", outStr);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });
  
  //server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");
  server.serveStatic("/", SPIFFS, "/");

  server.onNotFound([](AsyncWebServerRequest *request){
    DBG_OUTPUT_PORT.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      DBG_OUTPUT_PORT.printf("GET");
    else if(request->method() == HTTP_POST)
      DBG_OUTPUT_PORT.printf("POST");
    else if(request->method() == HTTP_DELETE)
      DBG_OUTPUT_PORT.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      DBG_OUTPUT_PORT.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      DBG_OUTPUT_PORT.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      DBG_OUTPUT_PORT.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      DBG_OUTPUT_PORT.printf("OPTIONS");
    else
      DBG_OUTPUT_PORT.printf("UNKNOWN");
    DBG_OUTPUT_PORT.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      DBG_OUTPUT_PORT.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      DBG_OUTPUT_PORT.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      DBG_OUTPUT_PORT.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        DBG_OUTPUT_PORT.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        DBG_OUTPUT_PORT.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        DBG_OUTPUT_PORT.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      DBG_OUTPUT_PORT.printf("UploadStart: %s\n", filename.c_str());
    DBG_OUTPUT_PORT.printf("%s", (const char*)data);
    if(final)
      DBG_OUTPUT_PORT.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      DBG_OUTPUT_PORT.printf("BodyStart: %u\n", total);
    DBG_OUTPUT_PORT.printf("%s", (const char*)data);
    if(index + len == total)
      DBG_OUTPUT_PORT.printf("BodyEnd: %u\n", total);
  });

  server.on("/", HTTP_GET, handleRootRequest);
  server.on("/index.htm", HTTP_GET, handleRootRequest);
  server.on("/index.html", HTTP_GET, handleRootRequest);

  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      DBG_OUTPUT_PORT.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        DBG_OUTPUT_PORT.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });
  
  server.begin();

  if (atool.initNFC(&pn532)) {
    DBG_OUTPUT_PORT.print("NFC chip: ");
    DBG_OUTPUT_PORT.print(atool.nfcChip);
    DBG_OUTPUT_PORT.print(" FW ver: ");
    DBG_OUTPUT_PORT.print(atool.nfcChipFWVer);
    DBG_OUTPUT_PORT.println("");
  }

  //printFilesJSON();
}

void loop(){
  if(shouldReboot){
    DBG_OUTPUT_PORT.println("Rebooting...");
    delay(100);
    ESP.restart();
  }
  ArduinoOTA.handle();
  handleNFC();
  handleReconnectWifi(false);

  if (scanWifi) {
    if ((millis() - lastWifiScanMillis) > 5000) {
      scanWifi = false;
      wifiScanJSON = "";
      getWifiStatusInfo(&wifiScanJSON, true);
      websocket.textAll(wifiScanJSON);
      lastWifiScanMillis = millis();
    }
  }
}
