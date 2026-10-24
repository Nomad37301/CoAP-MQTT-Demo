# Demo CoAP: tombol dan bel LED dengan tiga ESP

Tekan tombol pada ESP32 DevKit, ESP32-S3 membaca penghitung tombol lewat CoAP GET, lalu mengirim CoAP PUT ke ESP32-WROOM. Penerima menyalakan LED selama satu detik. Semua board terhubung ke Wi-Fi 2,4 GHz yang sama, tanpa perlu internet saat demo.

## Peralatan dan peran

| Board | Sketch | Komponen |
| --- | --- | --- |
| ESP32 DevKit biasa | `01_button_server/01_button_server.ino` | Push button |
| ESP32-S3 | `02_controller_s3/02_controller_s3.ino` | USB untuk Serial Monitor |
| ESP32-WROOM pada development board | `03_led_receiver/03_led_receiver.ino` | LED dan resistor 220-330 ohm |

Nama DevKit dan WROOM dapat merujuk pada board sejenis. Contoh mengasumsikan dua board pertama/terakhir memakai ESP32 klasik dan menyediakan GPIO18. GPIO18 berarti pin bertuliskan IO18/18, bukan kaki ke-18. Jika perangkatnya modul WROOM tanpa development board, rangkaian catu daya dan upload perlu disesuaikan dahulu.

OLED belum diperlukan untuk tahap ini. Sebelum menambahkannya, pastikan model pengontrol (misalnya SSD1306 atau SH1106), ukuran layar, antarmuka, dan tegangan modul.

## Rangkaian

**ESP tombol:**

```text
GPIO18 ---- [ PUSH BUTTON ] ---- GND
```

Program menggunakan `INPUT_PULLUP`: dilepas = HIGH, ditekan = LOW. Tidak perlu resistor pull-up eksternal untuk contoh ini. Tombol tidak disambungkan ke 5 V. Gunakan tombol eksternal, bukan tombol EN/RESET.

Untuk tombol empat kaki, gunakan dua kontak yang baru saling terhubung saat ditekan. Dua kaki dalam pasangan yang sama sudah tersambung secara internal. Cek menggunakan mode continuity pada multimeter jika ragu.

**ESP penerima:**

```text
GPIO18 ---- [ resistor 220-330 ohm ] ---- anoda LED
                                             |
                                         katoda LED ---- GND
```

Katoda biasanya kaki pendek/sisi badan LED yang pipih. Resistor harus seri dengan LED. Gunakan GND milik board tempat komponen terpasang. Antarboard tidak memerlukan kabel data karena komunikasi menggunakan Wi-Fi. Beri daya masing-masing development board lewat USB.

## Persiapan Arduino IDE

1. Pasang paket board **esp32 by Espressif Systems** lewat Boards Manager jika belum ada.
2. Library yang digunakan adalah **CoAP simple library** oleh Hirotaka Niisato/hirotakaster, dengan header `coap-simple.h`. Cari lewat Library Manager. Alternatif: unduh ZIP dari https://github.com/hirotakaster/CoAP-simple-library dan gunakan Sketch > Include Library > Add .ZIP Library.
3. Buka satu `.ino` untuk board yang akan diprogram. Setiap board memakai sketch terpisah; jangan gabungkan ketiganya dalam satu folder sketch.
4. Setelah clone, jalankan `Setup-LocalConfig.ps1` dari root repository untuk membuat tab `NetworkConfig.h` di setiap sketch. Isi `WIFI_SSID` dan `WIFI_PASSWORD` pada tab tersebut. Konfigurasi lokal ini diabaikan Git. Hindari guest Wi-Fi/client isolation yang memblokir komunikasi antarperangkat.
5. Untuk ESP32 klasik, pilih profil board yang cocok (umumnya ESP32 Dev Module). Untuk S3, pilih profil S3 yang cocok (umumnya ESP32S3 Dev Module). Pilih port USB sesuai board yang sedang tersambung.
6. Serial Monitor memakai **115200 baud**. Pada S3 dengan USB native, pengaturan USB CDC On Boot mungkin perlu diaktifkan sesuai konektor dan board. Program tetap berjalan tanpa menunggu Serial Monitor.

Paket ESP32 3.3.11 terdeteksi tersedia di komputer saat persiapan contoh ini. Contoh memakai API CoAP simple library 1.4.0. Library CoAP disalin ke `.build/libraries` untuk pemeriksaan kompilasi lokal; salinan ini tidak otomatis memasang library ke Library Manager Arduino IDE.

## Urutan upload dan demo

1. Upload `01_button_server` ke DevKit. Buka Serial Monitor, tekan RESET bila perlu, lalu catat `BUTTON IP`.
2. Upload `03_led_receiver` ke ESP32-WROOM. Catat `RECEIVER IP`.
3. Pada tab `NetworkConfig.h` milik `02_controller_s3`, ganti `BUTTON_IP` dan `RECEIVER_IP` dengan IP sebenarnya. Angka pada template `192.168.1.101` dan `.102` hanya contoh.
4. Upload `02_controller_s3` ke S3. Tunggu pesan **Sinkronisasi tombol**. Pengontrol mengabaikan tekanan sebelum sinkronisasi awal.
5. Tekan dan lepas tombol. LED di penerima menyala satu detik, kemudian mati sendiri. Untuk melihat tiap kedipan secara jelas, beri jeda sekitar tiga detik antarpenekanan.
6. Serial S3 menampilkan kejadian tombol, `PUT /bell`, dan `ACK 2.04`. Serial penerima menampilkan `BEL MASUK`, `LED ON`, dan `LED OFF`.

Alamat DHCP dapat berubah setelah board/router dinyalakan ulang. Catat lagi IP dan sesuaikan sketch pengontrol, atau gunakan DHCP reservation di router.

## Alur protokol

```text
S3 -- CON GET /button --> ESP tombol
S3 <-- ACK 2.05 Content, contoh a1b2c3d4:1 -- ESP tombol
S3 -- CON PUT /bell, isi a1b2c3d4:1 --> ESP penerima
S3 <-- ACK 2.04 Changed, isi a1b2c3d4:1 -- ESP penerima
```

| Resource | Metode | Arti |
| --- | --- | --- |
| `coap://IP_TOMBOL:5683/button` | GET | ID boot dan jumlah tekanan tombol |
| `coap://IP_PENERIMA:5683/bell` | PUT | Mengirim ID kejadian untuk menyalakan indikator |
| `coap://IP_PENERIMA:5683/bell` | GET | Status LED: `1` menyala, `0` mati |

Debounce 35 ms mengurangi tekanan ganda akibat kontak mekanis. Penghitung membuat tekanan singkat tidak hilang hanya karena tombol dilepas di antara polling. ID boot membedakan penghitung setelah board tombol di-reset. Polling dilakukan paling cepat setiap 250 ms jika tidak ada pengiriman bel tertunda.

Penerima mengingat ID kejadian terakhir selama masih menyala, sehingga permintaan ulang kejadian yang sama tidak memperpanjang kedipan. Pengontrol mencoba PUT baru setelah timeout dua detik dengan ID kejadian yang sama. Ini adalah percobaan ulang di tingkat aplikasi, bukan implementasi lengkap jadwal retransmisi CoAP standar. Contoh difokuskan pada GET, PUT, CON, ACK, dan kode respons; belum menggunakan Observe atau enkripsi.

Beberapa tekanan yang terkumpul di antara pembacaan diperlakukan sebagai satu notifikasi terbaru; contoh ini bukan antrean satu kedipan untuk setiap tekanan saat jaringan terputus. Jika penerima reboot, ingatan kejadian terakhir hilang. Jika S3 reboot, pengontrol mengambil kondisi tombol saat ini sebagai titik awal. ACK menunjukkan handler menerima perintah, bukan bukti LED secara fisik menyala.

## Jika belum berhasil

- Tombol selalu terbaca ditekan/tidak bereaksi: periksa pasangan kaki, GPIO18, dan GND. Lihat apakah hitungan pada Serial board tombol bertambah satu setiap tekanan.
- `GET /button timeout`: periksa IP board tombol dan apakah ketiga board satu jaringan tanpa client isolation.
- `PUT /bell timeout`: periksa IP board penerima dan daya board.
- Ada `BEL MASUK` tetapi LED mati: periksa polaritas, resistor, dan pin IO18. Jangan menganggap LED power board adalah LED yang bisa dikendalikan program.
- Sketch penerima kini memiliki `LED_BOOT_TEST = true`: setelah reset, output LED berkedip dua kali sebelum koneksi Wi-Fi. Jika LED eksternal tidak ikut berkedip, periksa apakah `LED_PIN` sesuai label GPIO tempat kabel terpasang, polaritas LED, sambungan GND, dan resistor. Pada rangkaian GPIO -> resistor -> anoda LED -> katoda -> GND, gunakan `LED_ACTIVE_HIGH = true`. Setelah pemeriksaan selesai, tes boot dapat dimatikan dengan mengubah `LED_BOOT_TEST` menjadi `false`.
- Untuk mencoba tanpa LED, ubah `LED_PIN` menjadi `-1`; status masih tampil di Serial Monitor.
- Untuk melihat paket di Wireshark, gunakan filter `coap` atau `udp.port == 5683`. Capture harus berada pada jalur lalu lintas (misalnya pada access point atau adaptor monitor yang mendukung); capture biasa pada laptop lain belum tentu melihat unicast antarketiga ESP.

## Referensi

- CoAP simple library dan API: https://github.com/hirotakaster/CoAP-simple-library
- GPIO dan INPUT_PULLUP ESP32: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html
- CoAP RFC 7252: https://www.rfc-editor.org/rfc/rfc7252.html

## Hasil pemeriksaan

Ketiga sketch berhasil dikompilasi pada 24 Oktober 2026 menggunakan Arduino CLI, paket esp32 3.3.11, dan CoAP simple library 1.4.0:

| Sketch | Target kompilasi | Flash | RAM global |
| --- | --- | --- | --- |
| Tombol | `esp32:esp32:esp32` | 894428 byte (68%) | 48064 byte (14%) |
| Pengontrol | `esp32:esp32:esp32s3` | 856784 byte (65%) | 46920 byte (14%) |
| Penerima | `esp32:esp32:esp32` | 895424 byte (68%) | 48056 byte (14%) |

Angka di atas berasal dari pemeriksaan ulang setelah penambahan tes boot LED dan pemisahan SSID/password ke `NetworkConfig.h`. Ketiga sketch lolos kompilasi. Konfigurasi jaringan yang sudah digunakan pada komputer pengujian dipertahankan dalam file lokal yang diabaikan Git.

Pemeriksaan otomatis tidak melakukan upload ke board. Pemilik proyek telah melaporkan demo tombol dan LED berhasil dijalankan pada perangkat; pemisahan konfigurasi tidak mengubah alur CoAP.
