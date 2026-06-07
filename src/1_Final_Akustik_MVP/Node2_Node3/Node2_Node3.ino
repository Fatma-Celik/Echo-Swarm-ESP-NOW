#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

// NODE 2 için 2 yap, NODE 3 için 3 yap.
#define NODE_ID 2
#define ESPNOW_CHANNEL 1
#define MIC_OUT_PIN 32
#define RED_LED_PIN 26
#define GREEN_LED_PIN 27
#define MSG_EVENT 1
#define MSG_RESULT 2
#define SOUND_ACTIVE_STATE LOW
#define SEND_REPEAT 3
#define SEND_COOLDOWN_MS 1200
#define RESULT_HOLD_MS 3000

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct __attribute__((packed)) {
  uint8_t msgType;
  uint8_t nodeId;
  uint32_t eventId;
  uint32_t eventTime;
} EventPacket;

typedef struct __attribute__((packed)) {
  uint8_t msgType;
  uint8_t winnerId;
  uint32_t decisionId;
} ResultPacket;

bool resultMode = false;
uint32_t resultStartMs = 0;
uint32_t lastSendMs = 0;
uint32_t eventId = 0;

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

void sendEvent() {
  EventPacket pkt;
  pkt.msgType = MSG_EVENT;
  pkt.nodeId = NODE_ID;
  pkt.eventId = ++eventId;
  pkt.eventTime = millis();

  for (int i = 0; i < SEND_REPEAT; i++) {
    esp_now_send(broadcastAddress, (uint8_t *)&pkt, sizeof(pkt));
    delay(20);
  }
  Serial.print("EVENT GONDERILDI | Node: ");
  Serial.println(NODE_ID);
}

void handleResultPacket(const uint8_t *incomingData, int len) {
  if (len != sizeof(ResultPacket)) return;
  ResultPacket result;
  memcpy(&result, incomingData, sizeof(result));
  
  if (result.msgType != MSG_RESULT) return;
  if (result.winnerId < 1 || result.winnerId > 3) return;

  Serial.print("RESULT ALINDI | Kazanan Node: ");
  Serial.println(result.winnerId);
  
  resultMode = true;
  resultStartMs = millis();

  if (result.winnerId == NODE_ID) {
    setRed();
  } else {
    setGreen();
  }
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
  if (len <= 0) return;
  uint8_t msgType = incomingData[0];
  if (msgType == MSG_RESULT) {
    handleResultPacket(incomingData, len);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.print("NODE ");
  Serial.print(NODE_ID);
  Serial.println(" BASLIYOR...");

  pinMode(MIC_OUT_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  setGreen();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.print("NODE MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW BASLATILAMADI!");
    setRed();
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Broadcast peer eklenemedi!");
      setRed();
      return;
    }
  }
  Serial.print("NODE ");
  Serial.print(NODE_ID);
  Serial.println(" HAZIR. Ses bekleniyor...");
}

void loop() {
  if (resultMode) {
    if (millis() - resultStartMs >= RESULT_HOLD_MS) {
      resultMode = false;
      setGreen();
      Serial.print("NODE ");
      Serial.print(NODE_ID);
      Serial.println(" tekrar dinleme modunda.");
    }
    return;
  }

  int micState = digitalRead(MIC_OUT_PIN);
  static uint32_t lastPrintMs = 0;

  if (millis() - lastPrintMs > 300) {
    lastPrintMs = millis();
  }

  if (micState == SOUND_ACTIVE_STATE && millis() - lastSendMs > SEND_COOLDOWN_MS) {
    lastSendMs = millis();
    Serial.print("NODE ");
    Serial.print(NODE_ID);
    Serial.println(" SES ALGILADI!");
    sendEvent();
  }
}
