#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define NODE_ID 1
#define ESPNOW_CHANNEL 1
#define TRIG_PIN 26
#define ECHO_PIN 27
#define OBSTACLE_ON_CM 20.0
#define OBSTACLE_OFF_CM 25.0
#define MIN_VALID_CM 2.0
#define MAX_VALID_CM 400.0
#define MSG_COMMAND 1
#define SEND_INTERVAL_MS 300

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct __attribute__((packed)) {
  uint8_t msgType;
  uint8_t obstacle;
  float distanceCm;
  uint32_t commandId;
} CommandPacket;

uint32_t lastSendMs = 0;
uint32_t commandId = 0;
// 0 = engel yok, 1 = engel var
bool obstacleState = false;

float readRawDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return -1.0;
  }
  return duration * 0.0343 / 2.0;
}

float readFilteredDistanceCm() {
  const int sampleCount = 5;
  float validSum = 0;
  int validCount = 0;
  for (int i = 0; i < sampleCount; i++) {
    float d = readRawDistanceCm();
    if (d >= MIN_VALID_CM && d <= MAX_VALID_CM) {
      validSum += d;
      validCount++;
    }
    delay(30);
  }
  if (validCount == 0) {
    return -1.0;
  }
  return validSum / validCount;
}

void updateObstacleState(float distanceCm) {
  if (distanceCm < 0) return;
  if (distanceCm <= OBSTACLE_ON_CM) {
    obstacleState = true;
  } else if (distanceCm >= OBSTACLE_OFF_CM) {
    obstacleState = false;
  }
}

void sendCommand(bool obstacle, float distanceCm) {
  CommandPacket cmd;
  cmd.msgType = MSG_COMMAND;
  cmd.obstacle = obstacle ? 1 : 0;
  cmd.distanceCm = distanceCm;
  cmd.commandId = ++commandId;
  esp_now_send(broadcastAddress, (uint8_t *)&cmd, sizeof(cmd));
  
  Serial.print("MESAFE: ");
  Serial.print(distanceCm);
  Serial.print(" cm | DURUM: ");
  if (obstacle) {
    Serial.println("ENGEL VAR -> KIRMIZI");
  } else {
    Serial.println("ENGEL YOK -> YESIL");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("MASTER + HC-SR04 BASLIYOR...");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  
  Serial.print("MASTER MAC: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW BASLATILAMADI!");
    return;
  }
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  
  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Broadcast peer eklenemedi!");
      return;
    }
  }
  Serial.println("MASTER HAZIR.");
}

void loop() {
  if (millis() - lastSendMs >= SEND_INTERVAL_MS) {
    lastSendMs = millis();
    float distance = readFilteredDistanceCm();
    if (distance < 0) {
      Serial.println("Gecerli mesafe okunamadi. Onceki durum korunuyor.");
    } else {
      updateObstacleState(distance);
    }
    sendCommand(obstacleState, distance);
  }
}
