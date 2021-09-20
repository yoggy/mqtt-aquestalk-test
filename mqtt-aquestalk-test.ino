#include <M5Core2.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "AquesTalkTTS.h"

#include "config.h"

void mqtt_sub_callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_host, mqtt_port, mqtt_sub_callback, wifi_client);

static bool speak_ready_flag = false;
static char speak_buf[256];

void setup()
{
  Serial.begin(115200);

  M5.begin(true, true, true, true);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(4);

  M5.Lcd.println("connecting...");

  M5.Axp.SetLcdVoltage(2600);
  M5.Axp.SetLed(false); 
  M5.Axp.SetSpkEnable(true);

  // print MAC Address
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  Serial.printf("MAC Address = %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println();

  // Wifi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("INFO: WiFi connected!");
  delay(1000);

  // MQTT
  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  }
  else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    Serial.println("ERROR: mqtt connecting failed...");
    reboot();
  }
  Serial.println("INFO: MQTT connected!");
  mqtt_client.subscribe(mqtt_subscribe_topic);
  delay(1000);

  int irv = TTS.createK(aquestalk_licencekey);
  if (irv) {
    Serial.print("ERROR: TTS.createK() failed...");
    reboot();
  }
  Serial.print("INFO: TTS.createK() success");

  //
  M5.Lcd.clear(TFT_BLACK);

  M5.Lcd.fillEllipse(80, 80, 20, 20, TFT_WHITE);
  M5.Lcd.fillEllipse(240, 80, 20, 20, TFT_WHITE);

  M5.Lcd.fillEllipse(160, 180, 80, 10, TFT_WHITE);
}

static int old_level = 0;
void loop()
{
  if (!mqtt_client.connected()) {
    Serial.println("ERROR: MQTT disconnected...");
    reboot();
  }
  mqtt_client.loop();

  if (M5.BtnA.wasPressed()) {
    TTS.playK("チェック、ワン、ツー、ワン、ツー", 100);
  }

  if (speak_ready_flag) {
      TTS.playK(speak_buf, 100);
      speak_ready_flag = false;
  }

  int level = TTS.getLevel();
  if (old_level != level) {
    if (level == 0) {
        M5.Lcd.fillEllipse(160, 180, 80, 10, TFT_WHITE);
    }
    else {
      int v = (millis() / 200) % 2;
      if (v) {
        M5.Lcd.fillEllipse(160, 180, 80, 10, TFT_BLACK);
      }
      else {
        M5.Lcd.fillEllipse(160, 180, 80, 10, TFT_WHITE);
      }
    }
    old_level = level;
  }

  M5.update();
}

void reboot() {
  M5.Lcd.fillScreen(RED);
  Serial.println("REBOOT!!!!!");
  delay(3000);
  ESP.restart();
}

void mqtt_sub_callback(char* topic, byte* payload, unsigned int length) {
  memset(speak_buf, 0, 256);
  int len = length >= 255 ? 255 : length;
  memcpy(speak_buf, payload, len);

  Serial.print("topic=");
  Serial.print(topic);
  Serial.print(", payload=");
  String str(speak_buf);
  Serial.print(str);
  Serial.println();

  speak_ready_flag = true;

  delay(2000);
}
