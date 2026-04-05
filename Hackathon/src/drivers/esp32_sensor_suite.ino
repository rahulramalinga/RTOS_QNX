/*
 * Project "Event Horizon" - ESP32 Sensor Suite
 * Outputs CSV sensor data to Serial at 115200 baud.
 * Format: "energy,imu_x,imu_y,imu_z\n"
 */

#define POT_PIN 34  // Analog pin for Energy Slider
#define UPDATE_RATE_HZ 10

void setup() {
  Serial.begin(115200);
  pinMode(POT_PIN, INPUT);
  // Initialize IMU if available
}

void loop() {
  // Read Potentiometer (Energy Slider)
  int potVal = analogRead(POT_PIN);
  float energy = (potVal / 4095.0) * 100.0;

  // Mock IMU Data (Replace with real MPU6050/similar reading)
  float imu_x = (float)random(-100, 100) / 10.0;
  float imu_y = (float)random(-100, 100) / 10.0;
  float imu_z = (float)random(-100, 100) / 10.0;

  // Output CSV to Serial
  Serial.print(energy);
  Serial.print(",");
  Serial.print(imu_x);
  Serial.print(",");
  Serial.print(imu_y);
  Serial.print(",");
  Serial.println(imu_z);

  delay(1000 / UPDATE_RATE_HZ);
}
