const int MIC_PIN = 32;

void setup() {
  Serial.begin(115200);
  pinMode(MIC_PIN, INPUT);
  Serial.println("Ses sensoru baslatildi. Dinleniyor...");
}

void loop() {
  int sesDurumu = digitalRead(MIC_PIN);
  
  // Serial.print("OUT durumu: ");
  // Serial.println(sesDurumu);
  
  if (sesDurumu == LOW) {
    Serial.println("Ses Algilandi! (El cirpmasi / Gurultu)");
    delay(200);
  }
  delay(50);
}
