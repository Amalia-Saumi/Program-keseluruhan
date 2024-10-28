#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Auth Token untuk masing-masing NodeMCU
char auth1[] = "MsU3-o-oSF8-BB8m5i1KaSImE9zcFmBw";  // Token NodeMCU Pertama
char auth2[] = "unUI6LbkExTDugMbk6eJFCuz94Mh7N_N";  // Token NodeMCU Kedua
char ssid[] = "Ndk Modal";  // Nama WiFi
char pass[] = "12345678";  // Password WiFi

// Pin untuk Sensor
#define SENSOR_1_PIN D5  // Sensor 1 di Cabang 1
#define SENSOR_2_PIN D6  // Sensor 2 di Cabang 1
#define SENSOR_3_PIN D5  // Sensor 3 di Cabang 2
#define SENSOR_4_PIN D6  // Sensor 4 di Cabang 2

float Q1 = 0;  // Debit Sensor 1 (m³/jam)
float Q2 = 0;  // Debit Sensor 2 (m³/jam)
float Q3 = 0;  // Debit Sensor 3 (m³/jam)
float Q4 = 0;  // Debit Sensor 4 (m³/jam)
float volumeCabang1 = 0; // Volume air cabang 1 (m³)
float volumeCabang2 = 0; // Volume air cabang 2 (m³)
float selisihCabang1, selisihCabang2;

BlynkTimer timer;

// Fungsi untuk menghitung biaya berdasarkan volume
float hitungBiaya(float volume) {
    float biaya = 0;
    if (volume <= 10) {
        biaya = volume * 210;  // Tarif Rp 210 per m³ untuk 0-10 m³
    } else if (volume <= 20) {
        biaya = (10 * 210) + ((volume - 10) * 310);  // Tarif Rp 310 per m³ untuk 11-20 m³
    } else if (volume <= 30) {
        biaya = (10 * 210) + (10 * 310) + ((volume - 20) * 450);  // Tarif Rp 450 per m³ untuk 21-30 m³
    } else {
        biaya = (10 * 210) + (10 * 310) + (10 * 450) + ((volume - 30) * 630);  // Tarif Rp 630 per m³ untuk >30 m³
    }
    return biaya;
}

// Fungsi untuk membaca debit air dari masing-masing sensor
void bacaSensor1() {
    Q1 = analogRead(SENSOR_1_PIN);  // Baca nilai dari sensor
    Q1 = (Q1 / 1000.0) * 60 * 7.5;   // Konversi ke m³/jam
    Blynk.virtualWrite(V0, Q1);
    volumeCabang1 += Q1 / 3600.0;    // Menghitung volume berdasarkan debit (m³)
}

void bacaSensor2() {
    Q2 = analogRead(SENSOR_2_PIN);  
    Q2 = (Q2 / 1000.0) * 60 * 7.5;   // Konversi ke m³/jam
    Blynk.virtualWrite(V1, Q2);
    volumeCabang1 += Q2 / 3600.0;    // Menambahkan volume air cabang 1
}

void bacaSensor3() {
    Q3 = analogRead(SENSOR_3_PIN);  
    Q3 = (Q3 / 1000.0) * 60 * 7.5;   // Konversi ke m³/jam
    Blynk.virtualWrite(V2, Q3);
    volumeCabang2 += Q3 / 3600.0;    // Menghitung volume berdasarkan debit (m³)
}

void bacaSensor4() {
    Q4 = analogRead(SENSOR_4_PIN);  
    Q4 = (Q4 / 1000.0) * 60 * 7.5;   // Konversi ke m³/jam
    Blynk.virtualWrite(V3, Q4);
    volumeCabang2 += Q4 / 3600.0;    // Menambahkan volume air cabang 2
}

// Fungsi untuk mengklasifikasikan tingkat kebocoran cabang 1
String klasifikasiKebocoranCabang1(float selisih) {
    if (selisih >= 0.000 && selisih <= 0.002) return "Aman";
    else if (selisih > 0.002 && selisih <= 0.004) return "Kecil";
    else if (selisih > 0.004 && selisih <= 0.008) return "Sedang";
    else if (selisih > 0.008) return "Besar";
    return "Tidak Terdeteksi";
}

// Fungsi untuk mengklasifikasikan tingkat kebocoran cabang 2
String klasifikasiKebocoranCabang2(float selisih) {
    if (selisih >= 0.000 && selisih <= 0.003) return "Aman";
    else if (selisih > 0.003 && selisih <= 0.005) return "Kecil";
    else if (selisih > 0.005 && selisih <= 0.009) return "Sedang";
    else if (selisih > 0.009) return "Besar";
    return "Tidak Terdeteksi";
}

// Fungsi untuk mengklasifikasikan kebocoran saat kedua cabang hidup
String klasifikasiKebocoranGabungan(float selisih1, float selisih2) {
    if (selisih1 >= 0.000 && selisih1 <= 0.136) return "Aman cabang 1";
    if (selisih1 > 0.136 && selisih1 <= 0.145) return "Kecil cabang 1";
    if (selisih1 > 0.145 && selisih1 <= 0.160) return "Sedang cabang 1";
    if (selisih1 > 0.160) return "Besar cabang 1";

    if (selisih2 >= 0.000 && selisih2 <= 0.008) return "Aman cabang 2";
    if (selisih2 > 0.008 && selisih2 <= 0.015) return "Kecil cabang 2";

    return "Tidak Terdeteksi";
}

// Fungsi deteksi kebocoran berdasarkan perbedaan debit
void deteksiKebocoran() {
    selisihCabang1 = abs(Q1 - Q2);
    selisihCabang2 = abs(Q3 - Q4);

    String statusCabang1 = klasifikasiKebocoranCabang1(selisihCabang1);
    String statusCabang2 = klasifikasiKebocoranCabang2(selisihCabang2);
    String statusGabungan = klasifikasiKebocoranGabungan(selisihCabang1, selisihCabang2);

    // Kirim status ke Blynk untuk ditampilkan
    Blynk.virtualWrite(V4, statusCabang1); // Status kebocoran cabang 1
    Blynk.virtualWrite(V5, statusCabang2); // Status kebocoran cabang 2
    Blynk.virtualWrite(V6, statusGabungan); // Status kebocoran gabungan

    // Menghitung biaya
    float biayaCabang1 = hitungBiaya(volumeCabang1);
    float biayaCabang2 = hitungBiaya(volumeCabang2);

    // Kirim biaya ke Blynk untuk ditampilkan
    Blynk.virtualWrite(V7, biayaCabang1);  // Biaya cabang 1
    Blynk.virtualWrite(V8, biayaCabang2);  // Biaya cabang 2
}

void setup() {
    Serial.begin(9600);
    Blynk.begin(auth1, ssid, pass);

    // Set timer untuk pembacaan data dan deteksi kebocoran
    timer.setInterval(1000L, bacaSensor1);
    timer.setInterval(1000L, bacaSensor2);
    timer.setInterval(1000L, bacaSensor3);
    timer.setInterval(1000L, bacaSensor4);
    timer.setInterval(2000L, deteksiKebocoran); // Deteksi kebocoran setiap 2 detik
}

void loop() {
    Blynk.run();
    timer.run();
}
