/*
 * IMU_Reader.ino  —  Air Writing Edition
 * ----------------------------------------
 * Streams both accelerometer AND gyroscope data over USB Serial.
 * Used for Madgwick filter + double integration on the PC side.
 *
 * Library required: Arduino_LSM9DS1
 *
 * Serial output format (one line per sample):
 *   "ax,ay,az,gx,gy,gz\n"
 *   ax/ay/az in g (gravity units)
 *   gx/gy/gz in degrees/second
 *
 * Baud rate: 115200 (Mac compatible)
 * Sample rate: 119 Hz
 */

#include <Arduino_LSM9DS1.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  while (!Serial);

  if (!IMU.begin()) {
    // Flash to signal failure
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH); delay(100);
      digitalWrite(LED_BUILTIN, LOW);  delay(100);
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);

  // Print metadata so PC knows sample rates
  Serial.print("# accel_sample_rate=");
  Serial.print(IMU.accelerometerSampleRate());
  Serial.println("Hz");

  Serial.print("# gyro_sample_rate=");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println("Hz");
}

void loop() {
  float ax, ay, az;
  float gx, gy, gz;

  // Wait until BOTH sensors have a new sample ready
  if (IMU.accelerometerAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAccelerometer(ax, ay, az);
    IMU.readGyroscope(gx, gy, gz);

    // Send all 6 values as CSV
    Serial.print(ax, 4); Serial.print(',');
    Serial.print(ay, 4); Serial.print(',');
    Serial.print(az, 4); Serial.print(',');
    Serial.print(gx, 2); Serial.print(',');
    Serial.print(gy, 2); Serial.print(',');
    Serial.println(gz, 2);
  }
}
