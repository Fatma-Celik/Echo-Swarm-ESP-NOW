#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#define NODE_ID 2
#define ESPNOW_CHANNEL 1
#define GREEN_LED_PIN 27
#define RED_LED_PIN 26
#define MSG_COMMAND 1

typedef struct __attribute__((packed)) {
  uint8_t msgType;
  uint8_t obstacle;
  float distanceCm;
  uint32_t commandId;
} CommandPacket;

void setGreen() {
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
}

void setRed() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
}

void allOff() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
  if (len != sizeof(CommandPacket)) return;
  
  CommandPacket cmd;
  memcpy(&cmd, incomingData, sizeof(cmd));
  
  if (cmd.msgType != MSG_COMMAND) return;
  
  if (cmd.obstacle == 1) {
    setRed();
  } else {
    setGreen();
  }
  
  Serial.print("NODE ");
  Serial.print(NODE_ID);
  Serial.print(" KOMUT ALDI | Mesafe: ");
  Serial.print(cmd.distanceCm);
  Serial.print(" cm | Durum: ");
  if (cmd.obstacle == 1) {
    Serial.println("ENGEL VAR -> KIRMIZI");
  } else {
    Serial.println("ENGEL YOK -> YESIL");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.print("NODE ");
  Serial.print(NODE_ID);
  Serial.println(" BASLIYOR...");
  
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  allOff();
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  
  Serial.print("NODE MAC: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW BASLATILAMADI!");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("NODE HAZIR. Master komutu bekleniyor...");
}

void loop() {
  // Node sadece master'dan gelen ESP-NOW komutunu bekler.
}
