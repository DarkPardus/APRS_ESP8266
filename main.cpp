#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

static const uint32_t CFG_MAGIC = 0x5441314B; // TA1K
static const uint16_t DNS_PORT = 53;
static const uint32_t BEACON_INTERVAL_DEFAULT = 15UL * 60UL * 1000UL;

struct Config {
  uint32_t magic;
  char ssid[33];
  char wifiPass[65];
  char callsign[16];
  char aprsPass[8];
  char latitude[10];   // DDMM.mmN
  char longitude[11];  // DDDMM.mmE
  uint16_t intervalMinutes;
};

Config cfg;
ESP8266WebServer server(80);
DNSServer dnsServer;
bool portalMode = false;
unsigned long lastBeacon = 0;

String htmlEscape(const String &s) {
  String o = s;
  o.replace("&", "&amp;"); o.replace("<", "&lt;"); o.replace(">", "&gt;");
  o.replace("\"", "&quot;"); o.replace("'", "&#39;");
  return o;
}

bool validPosition(const char *lat, const char *lon) {
  if (strlen(lat) != 8 || strlen(lon) != 9) return false;
  if (lat[7] != 'N' && lat[7] != 'S') return false;
  if (lon[8] != 'E' && lon[8] != 'W') return false;
  for (int i=0;i<7;i++) if (i != 4 && (lat[i] < '0' || lat[i] > '9')) return false;
  for (int i=0;i<8;i++) if (i != 5 && (lon[i] < '0' || lon[i] > '9')) return false;
  if (lat[4] != '.') return false;
  if (lon[5] != '.') return false;
  return true;
}

bool configValid() {
  return cfg.magic == CFG_MAGIC && strlen(cfg.ssid) > 0 && strlen(cfg.callsign) > 0 &&
         strlen(cfg.aprsPass) > 0 && validPosition(cfg.latitude, cfg.longitude);
}

void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, cfg);
  if (!configValid()) memset(&cfg, 0, sizeof(cfg));
}

void saveConfig() {
  cfg.magic = CFG_MAGIC;
  EEPROM.put(0, cfg);
  EEPROM.commit();
}

String configPage(const String &message = "") {
  String p = F("<!doctype html><html lang='tr'><meta name='viewport' content='width=device-width,initial-scale=1'><meta charset='utf-8'><title>TA1KD APRS Ayarları</title><style>body{font:16px Arial;max-width:520px;margin:24px auto;padding:0 14px;background:#101820;color:#eef}input{box-sizing:border-box;width:100%;padding:11px;margin:5px 0 14px;border:1px solid #567;border-radius:6px}button{padding:12px 20px;background:#00bfa5;border:0;border-radius:6px;font-weight:bold}small{color:#bbc}</style><h2>TA1KD APRS - ESP8266</h2><p>Wi-Fi ve APRS bilgilerini girin.</p>");
  if (message.length()) p += "<p><b>" + htmlEscape(message) + "</b></p>";
  p += F("<form method='POST' action='/save'>");
  p += F("Wi-Fi adı (SSID)<input name='ssid' required maxlength='32' value='"); p += htmlEscape(cfg.ssid); p += F("'>");
  p += F("Wi-Fi şifresi<input name='wpass' type='password' maxlength='64' value='"); p += htmlEscape(cfg.wifiPass); p += F("'>");
  p += F("Çağrı işareti<input name='call' required maxlength='15' value='"); p += htmlEscape(cfg.callsign[0] ? cfg.callsign : "TA1KD"); p += F("'>");
  p += F("APRS-IS passcode<input name='apass' required maxlength='7' inputmode='numeric' value='"); p += htmlEscape(cfg.aprsPass); p += F("'><small>Bu, APRS-IS için çağrı işaretinize ait sayısal doğrulama kodudur.</small>");
  p += F("<br>Enlem (DDMM.mmN / DDMM.mmS)<input name='lat' required maxlength='8' placeholder='4045.12N' value='"); p += htmlEscape(cfg.latitude); p += F("'>");
  p += F("Boylam (DDDMM.mmE / DDDMM.mmW)<input name='lon' required maxlength='9' placeholder='02903.45E' value='"); p += htmlEscape(cfg.longitude); p += F("'>");
  p += F("Gönderim aralığı (dakika)<input name='interval' type='number' min='5' max='60' value='"); p += String(cfg.intervalMinutes ? cfg.intervalMinutes : 15); p += F("'>");
  p += F("<button type='submit'>Kaydet ve yeniden başlat</button></form><p><small>APRS-IS internet üzerinden çalışır; bu cihaz RF/VHF yayını yapmaz. Konum sabittir, GPS kullanılmaz.</small></p></html>");
  return p;
}

void startPortal() {
  portalMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("TA1KD-APRS", "TA1KDsetup");
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.on("/", HTTP_GET, [](){ server.send(200, "text/html; charset=utf-8", configPage()); });
  server.on("/save", HTTP_POST, [](){
    String ssid = server.arg("ssid"), wp = server.arg("wpass"), call = server.arg("call"), ap = server.arg("apass");
    String lat = server.arg("lat"), lon = server.arg("lon");
    int interval = server.arg("interval").toInt();
    call.trim(); call.toUpperCase(); ap.trim(); lat.trim(); lat.toUpperCase(); lon.trim(); lon.toUpperCase();
    if (!ssid.length() || !call.length() || !ap.length() || !validPosition(lat.c_str(), lon.c_str()) || interval < 5 || interval > 60) {
      server.send(400, "text/html; charset=utf-8", configPage("Hatalı bilgi. Koordinat biçimini ve 5-60 dakika aralığını kontrol edin.")); return;
    }
    memset(&cfg, 0, sizeof(cfg));
    strlcpy(cfg.ssid, ssid.c_str(), sizeof(cfg.ssid));
    strlcpy(cfg.wifiPass, wp.c_str(), sizeof(cfg.wifiPass));
    strlcpy(cfg.callsign, call.c_str(), sizeof(cfg.callsign));
    strlcpy(cfg.aprsPass, ap.c_str(), sizeof(cfg.aprsPass));
    strlcpy(cfg.latitude, lat.c_str(), sizeof(cfg.latitude));
    strlcpy(cfg.longitude, lon.c_str(), sizeof(cfg.longitude));
    cfg.intervalMinutes = interval;
    saveConfig();
    server.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><h2>Ayarlar kaydedildi.</h2><p>Cihaz yeniden başlatılıyor. Telefonunuzu normal Wi-Fi ağınıza geri bağlayın.</p>");
    delay(1200); ESP.restart();
  });
  server.onNotFound([](){ server.send(200, "text/html; charset=utf-8", configPage()); });
  server.begin();
  Serial.println("Ayar agi: TA1KD-APRS / sifre: TA1KDsetup");
  Serial.print("Tarayicida acin: http://"); Serial.println(WiFi.softAPIP());
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid, cfg.wifiPass);
  Serial.print("Wi-Fi baglantisi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 25000) { delay(500); Serial.print('.'); }
  if (WiFi.status() == WL_CONNECTED) { Serial.print("\nWi-Fi IP: "); Serial.println(WiFi.localIP()); return true; }
  Serial.println("\nWi-Fi baglanamadi; ayar portalina geciliyor.");
  return false;
}

bool sendBeacon() {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClient client;
  client.setTimeout(5000);
  Serial.println("APRS-IS baglantisi...");
  if (!client.connect("euro.aprs2.net", 14580)) { Serial.println("APRS-IS baglanti hatasi."); return false; }
  client.print("user "); client.print(cfg.callsign);
  client.print(" pass "); client.print(cfg.aprsPass);
  client.println(" vers TA1KD-ESP8266 1.0");
  unsigned long t = millis();
  String response;
  while (millis() - t < 2500) {
    while (client.available()) { char c = client.read(); if (c == '\n') { Serial.print("Sunucu: "); Serial.println(response); response = ""; } else if (c != '\r') response += c; }
    delay(10);
  }
  String packet = String(cfg.callsign) + ">APRS,TCPIP*:= " + cfg.latitude + "/" + cfg.longitude + ">TA1KD ESP8266 internet APRS";
  // APRS position format has no space after :=
  packet.replace(":= ", ":=");
  client.println(packet);
  Serial.print("Gonderilen paket: "); Serial.println(packet);
  delay(500);
  while (client.available()) Serial.write(client.read());
  client.stop();
  lastBeacon = millis();
  return true;
}

void setup() {
  Serial.begin(115200); delay(300);
  loadConfig();
  pinMode(0, INPUT_PULLUP); // FLASH dugmesine basili acilis: ayar portalini ac
  if (!configValid() || digitalRead(0) == LOW) { startPortal(); return; }
  if (connectWiFi()) sendBeacon(); else startPortal();
}

void loop() {
  if (portalMode) { dnsServer.processNextRequest(); server.handleClient(); return; }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect(); delay(1000);
    if (WiFi.status() != WL_CONNECTED) return;
  }
  uint32_t interval = (uint32_t)(cfg.intervalMinutes ? cfg.intervalMinutes : 15) * 60000UL;
  if (millis() - lastBeacon >= interval) sendBeacon();
  delay(100);
}
