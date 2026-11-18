TA1KD APRS - NodeMCU ESP8266 (ekransiz)

Bu proje ESP8266 ile APRS-IS'e internet uzerinden sabit konum beacon'i gonderir. RF/VHF yayini yapmaz; GPS yoktur.

FIRMWARE DERLEME
1) Bu klasoru bir GitHub deposuna yukleyin.
2) GitHub'da Actions sekmesine girin.
3) Build ESP8266 firmware is akisinin tamamlanmasini bekleyin.
4) Tamamlanan calismanin Artifacts bolumunden TA1KD-ESP8266-firmware dosyasini indirin.
5) ZIP icindeki firmware.bin dosyasini https://esp.huhn.me/ adresinden ESP8266'ya yukleyin.

ILK AYAR
1) NodeMCU'yu USB ile guclendirin.
2) Telefonda Wi-Fi aglarindan TA1KD-APRS agina baglanin. Sifre: TA1KDsetup
3) Tarayicida http://192.168.4.1 adresini acin.
4) Wi-Fi, cagri isareti, APRS-IS passcode ve koordinatlari girin.
5) Kaydet ve yeniden baslat secin.

SONRADAN AYAR DEGISTIRME
NodeMCU'yu yeniden baslatirken FLASH dugmesini basili tutun. Ayar agi tekrar acilir.

KOORDINAT
Enlem: DDMM.mmN veya DDMM.mmS (ornek: 4045.12N)
Boylam: DDDMM.mmE veya DDDMM.mmW (ornek: 02903.45E)
Ornekler yalnizca bicimi gostermek icindir; kendi koordinatinizi girin.

NOTLAR
- Beacon varsayilan 15 dakikada bir gonderilir; web formundan 5-60 dakika secilebilir.
- APRS-IS passcode, cagrı isaretinize ait dogru kod olmalidir.
- Ayar erisim noktasi WPA2 sifrelidir. Kurulumdan sonra portal kapatilir.
- Bu yazilim yalnizca internet/APRS-IS gonderimi icindir; telsizden RF gonderimi yapmaz.
