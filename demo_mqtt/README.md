# Demo MQTT: tombol, LED, dan OLED SSD1306

Tiga ESP menggunakan broker MQTT lokal di laptop. ESP tombol publish kejadian; ESP LED dan ESP OLED subscribe melalui broker. Tidak diperlukan polling antar-ESP atau internet saat demo. Laptop dan ketiga ESP harus menggunakan hotspot **A51** yang sama.

## Rangkaian dan program

| Perangkat | Sketch | Sambungan |
| --- | --- | --- |
| ESP32 DevKit | `01_button_publisher/01_button_publisher.ino` | GPIO18 -> tombol -> GND, INPUT_PULLUP |
| ESP32-WROOM development board | `02_led_subscriber/02_led_subscriber.ino` | GPIO18 -> resistor 330 ohm -> anoda LED; katoda -> GND |
| ESP32-S3 | `03_oled_monitor_s3/03_oled_monitor_s3.ino` | OLED SSD1306 128x64 I2C, tabel di bawah |

| Pin OLED | ESP32-S3 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

Ikuti tulisan pin pada OLED; urutan fisik VCC/GND bisa berbeda antarproduk. GPIO8/9 harus tersedia dan bebas pada board S3 yang digunakan; jika tidak, ganti `OLED_SDA`/`OLED_SCL` sesuai board. Jangan gunakan GPIO21/22 dari contoh ESP32 klasik sebagai pin default S3. Program memeriksa alamat 0x3C dan 0x3D. Pengaturan I2C dibuat eksplisit dengan `Wire.begin(OLED_SDA, OLED_SCL)`.

Tombol empat kaki: pilih dua kontak yang baru saling terhubung saat ditekan. Resistor dan LED harus seri, bukan kedua kakinya dalam satu jalur breadboard. Penerima menjalankan tes dua kedipan saat reset seperti versi CoAP.

## 1. Sambungkan jaringan

Sambungkan laptop ke hotspot A51. Ketiga ESP juga menggunakan hotspot tersebut. Dalam demo ini S3 menjadi monitor OLED dan memakai mode Wi-Fi station. Wi-Fi access point dan broker MQTT adalah peran berbeda; broker berjalan di laptop.

Setelah clone, jalankan `Setup-LocalConfig.ps1` dari root repository untuk membuat tab `NetworkConfig.h` pada setiap sketch berdasarkan template. Isi SSID/password sesuai jaringan. Konfigurasi lokal yang sudah ada dipertahankan dan diabaikan Git. Jika hotspot berubah, sesuaikan konfigurasi pada ketiga sketch. Password Wi-Fi tidak sama dengan password MQTT; contoh broker lokal ini tidak memakai login MQTT.

## 2. Pasang library di Arduino IDE

- **MQTT** oleh **Joel Gaehwiler (256dpi)**, versi 2.5.3; header `MQTT.h`. Cari "MQTT" atau "lwmqtt" di Library Manager. Bukan PubSubClient.
- **Adafruit SSD1306**, **Adafruit GFX Library**, dan **Adafruit BusIO**, untuk sketch OLED.
- Paket board **esp32 by Espressif Systems**. ESP32 klasik memakai profil yang sesuai, umumnya ESP32 Dev Module; S3 memakai profil S3 yang sesuai, umumnya ESP32S3 Dev Module.

Library MQTT 2.5.3 sudah dipasang di Arduino IDE saat persiapan. Library Adafruit juga sudah terdeteksi terpasang. Semua Serial Monitor memakai 115200 baud. USB CDC On Boot pada S3 disesuaikan dengan konektor/board.

## 3. Siapkan broker Mosquitto di laptop

Pada komputer pengujian, Mosquitto 2.1.2 portabel disiapkan dari installer resmi di `.build/mosquitto`. Folder `.build` diabaikan Git dan tidak ikut clone. Siapkan Mosquitto dari https://mosquitto.org/download/ atau tentukan lokasi salinan portabel melalui parameter `-MosquittoPath`. Script otomatis mendukung salinan di `.build/mosquitto` atau instalasi standar Windows, dan tidak mengubah service atau firewall.

Buka PowerShell di folder `demo_mqtt`, setelah laptop tersambung ke A51:

```powershell
.\Start-Broker.ps1
```

Script mencari IP Wi-Fi laptop, menuliskannya ke `MQTT_HOST` pada ketiga `NetworkConfig.h`, membuat `mosquitto.runtime.conf`, lalu menjalankan broker di IP tersebut pada TCP port 1883. Biarkan terminal terbuka. Jika IP tidak dapat dipilih otomatis, lihat bagian Wireless LAN adapter Wi-Fi pada `ipconfig` dan jalankan:

```powershell
.\Start-Broker.ps1 -BrokerIp 192.168.43.100
```

IP di atas hanya contoh. Jangan gunakan IP Tailscale/VPN, 127.0.0.1, atau IP ESP sebagai alamat broker. Bila perlu, jalankan script menggunakan lokasi Mosquitto yang berbeda:

```powershell
.\Start-Broker.ps1 -MosquittoPath 'D:\Mosquitto\mosquitto.exe'
```

Jika PowerShell memblokir script lokal, jalankan satu kali dari terminal saat ini:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Start-Broker.ps1
```

Perintah tersebut hanya memilih execution policy untuk proses itu; script tidak mengubah pengaturan permanen komputer. Jika Windows Firewall menanyakan akses Mosquitto, izinkan pada jaringan demo yang dipakai. Jika broker sudah berjalan sebagai service dan memakai port 1883, jangan jalankan instance kedua pada alamat/port yang sama; gunakan konfigurasi service yang sesuai atau hentikan instance milik demo sebelumnya.

Konfigurasi demo membatasi listener ke IP Wi-Fi laptop, menggunakan akses tanpa login dan tanpa TLS, serta tidak menyimpan retained message ke disk saat broker dimatikan. Gunakan untuk jaringan demo lokal.

Untuk hanya mengatur IP tanpa menjalankan broker:

```powershell
.\Set-BrokerIp.ps1
```

## 4. Upload dan coba

1. Jalankan broker setelah laptop bergabung ke A51. Pastikan `MQTT_HOST` pada ketiga tab `NetworkConfig.h` berisi IP laptop yang benar, bukan placeholder.
2. Upload `02_led_subscriber` ke WROOM. LED eksternal harus berkedip dua kali setelah reset. Tunggu `MQTT terhubung` dan `SUB QoS1 demo/button/press`.
3. Upload `03_oled_monitor_s3` ke S3. OLED menampilkan judul, WiFi/MQTT, hitungan, status LED, dan status dua board.
4. Upload `01_button_publisher` ke DevKit. Tunggu koneksi MQTT.
5. Tekan tombol. LED penerima menyala sekitar satu detik, OLED menampilkan hitungan baru dan LED ON/OFF. Beri jeda tiga detik antarpenekanan untuk demo awal.
6. Reset S3: hitungan dan status terakhir diperoleh lagi dari retained message, tanpa memicu bel lama.
7. Cabut daya salah satu board: broker mengirim Last Will `offline` setelah koneksi dinyatakan putus (biasanya belasan detik, bukan langsung).

Jika alamat IP laptop berubah, jalankan script lagi dan upload ulang ketiga sketch. Script tidak memprogram board secara otomatis.

## Topik MQTT

| Topik | Payload | Publish/subscribe | Retain | QoS |
| --- | --- | --- | --- | --- |
| `demo/button/press` | contoh `a1b2c3d4:5` (ID boot:hitungan) | Tombol -> LED, OLED | Tidak | 1 |
| `demo/button/count` | contoh `5` | Tombol -> OLED | Ya | 1 |
| `demo/led/state` | `ON` / `OFF` | LED -> OLED | Ya | 1 |
| `demo/button/status` | `online` / `offline` | Tombol / broker Last Will -> OLED | Ya | 1 |
| `demo/led/status` | `online` / `offline` | LED / broker Last Will -> OLED | Ya | 1 |
| `demo/oled/status` | `online` / `offline` | OLED / broker Last Will -> pemantau laptop | Ya | 1 |

MQTT memakai TCP. Masing-masing ESP mengetahui alamat broker, bukan alamat dua ESP lainnya. Client ID dibuat dari peran dan ID chip agar tidak saling memutus koneksi.

QoS1 memberi konfirmasi pada tiap hubungan MQTT dan dapat menghasilkan duplikat. Receiver mengingat ID kejadian terakhir agar pengiriman ulang kejadian yang sama tidak mengulang kedipan. Status GPIO yang dilaporkan adalah keadaan output menurut program, bukan hasil sensor yang membuktikan LED menyala secara fisik. PUBACK pada tombol berarti broker mengakui pesan, bukan bukti penerima telah menyalakan LED.

## Melihat pesan dari laptop

Buka terminal kedua; ganti IP contoh dengan IP broker yang dicetak script:

```powershell
& '..\.build\mosquitto\mosquitto_sub.exe' -h 192.168.43.100 -t 'demo/#' -q 1 -v
```

Uji penerima tanpa tombol (jangan gunakan `-r` pada kejadian tombol):

```powershell
& '..\.build\mosquitto\mosquitto_pub.exe' -h 192.168.43.100 -t 'demo/button/press' -m 'deadbeef:1' -q 1
```

Untuk kejadian berikutnya ubah menjadi `deadbeef:2`. Mengulang ID terakhir seharusnya tidak menyalakan LED lagi. Penghitung OLED berasal dari `demo/button/count` milik board tombol, jadi pengujian event manual ini tidak menaikkan penghitung tersebut.

## Batasan dan pemeriksaan masalah

- Contoh memakai clean session. Subscriber yang offline tidak mendapat seluruh riwayat kejadian tombol. Retained count/state hanya memberikan nilai terakhir.
- Tombol menyimpan satu kejadian terbaru di RAM saat koneksi gagal; beberapa tekanan saat offline digabung menjadi satu kejadian terbaru. Hitungan tetap bertambah selama board hidup dan kembali nol ketika board tombol reset. Ini bukan antrean persisten.
- Dua kedipan boot tidak mengirim kejadian tombol atau menaikkan hitungan. LED hanya sementara diuji sebelum koneksi jaringan.
- OLED gelap: cek urutan pin, catu 3,3 V, SDA8/SCL9, dan keluaran Serial. Program tetap memantau lewat Serial walaupun layar tidak terdeteksi. Jika alamat terdeteksi tetapi layar kosong, pastikan pengontrol benar SSD1306, bukan SH1106.
- MQTT gagal: pastikan broker tetap menyala, laptop satu hotspot dengan ESP, IP sesuai, TCP 1883 dapat diakses, dan hotspot tidak mengisolasi klien. Perangkat yang berhasil terhubung Wi-Fi belum tentu bisa mencapai broker.
- LED tidak merespons: cek dua kedipan saat reset dahulu, lalu lihat `BEL MASUK`. Jika event diterima tetapi LED mati, periksa rangkaian GPIO18-resistor-LED-GND.
- Tampilan LED ON dapat terlambat/hilang jika jaringan lambat; durasi output diproses di loop dan operasi koneksi/publish dapat menunda pembaruan.
- Broker dimatikan: OLED menunjukkan MQTT `--`; status remote terakhir bisa usang. Setelah broker kembali, board menyambung ulang dan publish status terkini.

## Validasi

Ketiga sketch berhasil dikompilasi pada 24 Oktober 2026 dengan paket esp32 3.3.11 dan MQTT 2.5.3. Sketch OLED memakai Adafruit SSD1306 2.5.17, Adafruit GFX 1.12.6, dan Adafruit BusIO 1.17.4 yang sudah terpasang.

| Sketch | Target | Flash | RAM global |
| --- | --- | --- | --- |
| Tombol | `esp32:esp32:esp32` | 903292 byte (68%) | 47360 byte (14%) |
| LED | `esp32:esp32:esp32` | 904580 byte (69%) | 47360 byte (14%) |
| OLED | `esp32:esp32:esp32s3` | 926453 byte (70%) | 47928 byte (14%) |

Mosquitto 2.1.2 portabel diuji sementara di localhost TCP 18883 dengan klien mosquitto_pub/sub. Semua pemeriksaan berikut lulus: satu kejadian QoS1 diterima dua subscriber, subscriber baru menerima retained count, subscriber baru tidak menerima kejadian tombol lama, dan putusnya klien menghasilkan retained Last Will `offline`. Semua proses pengujian sudah dihentikan. Kedua script PowerShell juga lolos pemeriksaan sintaks.

Pemeriksaan otomatis tidak melakukan upload atau menguji fisik ESP, tombol, LED, dan OLED. Pemilik proyek kemudian melaporkan demo berhasil dijalankan pada perangkat. Pada clone baru, IP broker masih berupa placeholder sampai laptop bergabung ke jaringan demo dan script pengaturan IP dijalankan; izin firewall disesuaikan pada komputer masing-masing.

## Referensi

- MQTT library/API: https://github.com/256dpi/arduino-mqtt
- Mosquitto Windows: https://mosquitto.org/download/
- Mosquitto listener/authentication: https://mosquitto.org/documentation/authentication-methods/
- SSD1306: https://github.com/adafruit/Adafruit_SSD1306
- I2C ESP32: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html
