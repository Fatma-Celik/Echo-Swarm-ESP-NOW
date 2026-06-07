#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

#define NODE_ID 1
#define ESPNOW_CHANNEL 1
#define MIC_OUT_PIN 32
#define RED_LED_PIN 26
#define GREEN_LED_PIN 27
#define MSG_EVENT 1
#define MSG_RESULT 2

#define SOUND_ACTIVE_STATE LOW
#define DECISION_WINDOW_MS 400
#define RESULT_HOLD_MS 3000
#define LOCAL_COOLDOWN_MS 1200

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

bool roundActive = false;
bool resultSent = false;
uint32_t roundStartMs = 0;
uint32_t resultStartMs = 0;
uint32_t lastLocalEventMs = 0;
uint32_t localEventId = 0;
uint32_t decisionId = 0;
uint8_t winnerId = 0;
bool seen[4] = {false, false, false, false};

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

void registerEvent(uint8_t id) {
  if (id < 1 || id > 3) return;
  if (resultSent) return;

  if (!roundActive) {
    roundActive = true;
    roundStartMs = millis();
  }

  winnerId = id;
  seen[id] = true;
  Serial.print("EVENT KAYDEDILDI | Node: ");
  Serial.println(id);
}

void sendResult(uint8_t winner) {
  ResultPacket result;
  result.msgType = MSG_RESULT;
  result.winnerId = winner;
  result.decisionId = ++decisionId;

  for (int i = 0; i < 5; i++) {
    esp_now_send(broadcastAddress, (uint8_t *)&result, sizeof(result));
    delay(30);
  }

  Serial.println();
  Serial.println("========== KARAR ==========");
  Serial.print("Sese en yakin / ilk algilayan Node: ");
  Serial.println(winner);
  for (int i = 1; i <= 3; i++) {
    Serial.print("Node ");
    Serial.print(i);
    Serial.print(" algiladi mi: ");
    Serial.println(seen[i] ? "EVET" : "HAYIR");
  }
  Serial.println("===========================");
  Serial.println();

  if (winner == NODE_ID) {
    setRed();
  } else {
    setGreen();
  }

  resultSent = true;
  resultStartMs = millis();
}

void handleEventPacket(const uint8_t *incomingData, int len) {
  if (len != sizeof(EventPacket)) return;
  EventPacket pkt;
  memcpy(&pkt, incomingData, sizeof(pkt));
  if (pkt.msgType != MSG_EVENT) return;
  if (pkt.nodeId == NODE_ID) return;
  registerEvent(pkt.nodeId);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
  if (len <= 0) return;
  uint8_t msgType = incomingData[0];
  if (msgType == MSG_EVENT) {
    handleEventPacket(incomingData, len);
  }
}

void resetRound() {
  roundActive = false;
  resultSent = false;
  winnerId = 0;
  for (int i = 0; i < 4; i++) {
    seen[i] = false;
  }
  setGreen();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("MASTER + NODE 1 DIJITAL MIKROFON BASLIYOR...");

  pinMode(MIC_OUT_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  setGreen();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.print("MASTER / NODE 1 MAC: ");
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
  resetRound();
  Serial.println("MASTER HAZIR. Ses bekleniyor...");
}

void loop() {
  if (resultSent) {
    if (millis() - resultStartMs >= RESULT_HOLD_MS) {
      resetRound();
      Serial.println("Yeni ses olayi bekleniyor...");
    }
    return;
  }

  int micState = digitalRead(MIC_OUT_PIN);
  static uint32_t lastPrintMs = 0;

  if (millis() - lastPrintMs > 300) {
    lastPrintMs = millis();
  }

  if (micState == SOUND_ACTIVE_STATE && millis() - lastLocalEventMs > LOCAL_COOLDOWN_MS) {
    lastLocalEventMs = millis();
    localEventId++;
    Serial.println("NODE 1 SES ALGILADI!");
    registerEvent(NODE_ID);
  }

  if (roundActive && !resultSent) {
    if (millis() - roundStartMs >= DECISION_WINDOW_MS) {
      if (winnerId != 0) {
        sendResult(winnerId);
      } else {
        resetRound();
      }
    }
  }
}
