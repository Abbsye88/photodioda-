const int PIN_PHOTODIODE = 34; // Pin OUT sensor photodiode terhubung ke GPIO 34

// Parameter kalibrasi deteksi tetesan
const unsigned long DEBOUNCE_MS  = 200;   // Jeda minimal antar tetes (200 ms = maks 300 tetes/menit)
const unsigned long TIMEOUT_MS   = 10000; // Batas waktu jika aliran berhenti (10 detik)
const float FAKTOR_TETES         = 20.0;  // 20 tetes/mL (set makro standar) atau 60 tetes/mL (mikro)

volatile unsigned long totalTetes = 0;
volatile unsigned long waktuTetesTerakhir = 0;
volatile unsigned long intervalTetes = 0;
volatile bool adaTetesBaru = false;

// Interrupt Service Routine (ISR): otomatis berjalan saat tetesan memotong cahaya
void IRAM_ATTR deteksiTetesISR() {
  unsigned long sekarang = millis();
  unsigned long selisih = sekarang - waktuTetesTerakhir;

  // Filter debounce mencegah 1 tetesan terhitung ganda akibat pantulan cahaya air
  if (selisih >= DEBOUNCE_MS) {
    if (waktuTetesTerakhir > 0) {
      intervalTetes = selisih;
    }
    waktuTetesTerakhir = sekarang;
    totalTetes++;
    adaTetesBaru = true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PHOTODIODE, INPUT);

  // Deteksi perubahan sinyal saat tetesan melewati sensor
  attachInterrupt(digitalPinToInterrupt(PIN_PHOTODIODE), deteksiTetesISR, FALLING);

  Serial.println("=== MONITOR LAJU TETES AIR (ESP32 + PHOTODIODE) ===");
  Serial.println("Menunggu tetesan pertama...");
}

void loop() {
  unsigned long salinanTotal;
  unsigned long salinanInterval;
  unsigned long salinanWaktuTerakhir;
  bool tetesBaru;

  // Salin variabel interrupt secara aman agar tidak bertabrakan dengan ISR
  noInterrupts();
  salinanTotal = totalTetes;
  salinanInterval = intervalTetes;
  salinanWaktuTerakhir = waktuTetesTerakhir;
  tetesBaru = adaTetesBaru;
  adaTetesBaru = false;
  interrupts();

  // Tampilkan hasil perhitungan setiap kali ada tetesan baru
  if (tetesBaru) {
    float tpm = 0.0;      // Tetes Per Menit (TPM)
    float mlPerJam = 0.0; // Estimasi debit cairan (mL/jam)

    if (salinanInterval > 0) {
      tpm = 60000.0 / (float)salinanInterval;
      mlPerJam = (tpm * 60.0) / FAKTOR_TETES;
    }

    Serial.print("Tetes ke-");
    Serial.print(salinanTotal);
    Serial.print(" | Interval: ");
    Serial.print(salinanInterval);
    Serial.print(" ms | Laju: ");
    Serial.print(tpm, 1);
    Serial.print(" tetes/menit | Debit: ");
    Serial.print(mlPerJam, 1);
    Serial.println(" mL/jam");
  }

  // Deteksi jika aliran berhenti (tidak ada tetes selama > 10 detik)
  static bool statusBerhentiTampil = false;
  if (salinanTotal > 0 && (millis() - salinanWaktuTerakhir > TIMEOUT_MS)) {
    if (!statusBerhentiTampil) {
      Serial.println("[PERINGATAN] Aliran berhenti: 0.0 tetes/menit (0.0 mL/jam)");
      statusBerhentiTampil = true;
    }
  } else if (tetesBaru) {
    statusBerhentiTampil = false;
  }
}

