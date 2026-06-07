# Echo Swarm: ESP-NOW ile Sürü Ağı Teknolojileri 🐝📡

Bu proje, KTO Karatay Üniversitesi Haberleşme Mühendisliği dersi kapsamında geliştirilmiş, **ESP-NOW** protokolü kullanan üç düğümlü bir kablosuz haberleşme ve sürü ağı MVP (Minimum Viable Product) prototipidir. 

Projenin temel amacı, dış bir ağ altyapısına (router/modem) ihtiyaç duymadan ESP32 düğümlerinin birbirleriyle düşük gecikmeli olarak haberleşmesini sağlamak ve "Echo Swarm" (enkaz altı arama-kurtarma sürüsü) konsepti için temel bir akustik algılama altyapısı oluşturmaktır.

## 🚀 Proje Mimarisi

Sistem 3 adet **ESP32-WROOM-32D** geliştirme kartından oluşmaktadır:
* **Master / Node 1:** Hem ortamı dinler (ses algılar) hem de diğer Node'lardan gelen olay (Event) paketlerini toplayarak merkezi kararı verir.
* **Node 2 & Node 3:** Ortamı bağımsız olarak dinler. Ses (eşik değeri) algıladıklarında Master'a ESP-NOW üzerinden veri gönderirler.
* **Karar Mekanizması:** Master, ilk gelen veri paketini "sese en yakın/ilk algılayan" düğüm olarak kabul eder ve sonucu tüm ağa *broadcast* olarak yayınlar. Kazanan düğüm **Kırmızı LED**, diğerleri **Yeşil LED** yakar.

## 🛠️ Donanım ve Kullanılan Malzemeler

| Bileşen | Adet | Görev |
| :--- | :---: | :--- |
| **ESP32-WROOM-32D** | 3 | Master ve node kontrolcüleri |
| **Mikrofon Sensörü** | 3 | Akustik ses olayının algılanması (Aktif LOW) |
| **Trafik Lambası / LED Modülü** | 3 | Kazanan/Bekleyen node durumlarını göstermek |
| **HC-SR04 Sensörü** | 1 | İlk MVP aşamasında engel algılama testi için |

## 🔌 Pin Bağlantıları

**Mikrofon Modülü Bağlantısı:**
| Mikrofon Modülü | ESP32-WROOM-32D |
| :--- | :--- |
| VCC | 3.3V |
| GND | GND |
| OUT | GPIO32 |

**Durum Gösterge LED Bağlantıları:**
| LED Rengi | ESP32-WROOM-32D |
| :--- | :--- |
| Kırmızı LED (Kazanan) | GPIO26 |
| Yeşil LED (Bekleme) | GPIO27 |
| GND | GND |

## 📂 Klasör Yapısı

* `/docs`: Projenin detaylı final raporu (PDF).
* `/src/1_Final_Akustik_MVP`: Mikrofon modüllü nihai sistemin (Master ve Node) Arduino kodları.
* `/src/2_Ilk_Mesafe_MVP_HCSR04`: Projenin ilk aşamasında ESP-NOW broadcast testleri için yazılmış mesafe sensörlü kodlar.
* `/src/3_Test_Kodlari`: Mikrofon modülünün dijital eşik çıkışlarını test etmek için kullanılan basit script.

## ⚙️ Kurulum ve Çalıştırma

1. Projeyi bilgisayarınıza klonlayın: `git clone https://github.com/Fatma-Celik/Echo-Swarm-ESP-NOW.git`
2. Arduino IDE üzerinden `esp32` kütüphanesinin kurulu olduğundan emin olun.
3. `src/1_Final_Akustik_MVP/Master_Node1` kodunu birinci ESP32'ye yükleyin.
4. `src/1_Final_Akustik_MVP/Node2_Node3` kodunu açın, `#define NODE_ID 2` yaparak ikinci karta, `#define NODE_ID 3` yaparak üçüncü karta yükleyin.
5. Kartlara güç verdiğinizde sistem otomatik olarak ESP-NOW ağı kuracak ve ses dinlemeye başlayacaktır.

---
*Geliştiren: Fatma Çelik | KTO Karatay Üniversitesi - 2026*
