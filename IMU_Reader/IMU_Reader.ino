/*
 * IMU_Reader.ino
 * --------------
 * Streams gyroscope X/Y data over USB Serial as fast as possible.
 * Designed for Arduino Nano 33 BLE Sense.
 *
 * Library required: Arduino_LSM9DS1
 *   Install via: Arduino IDE → Sketch → Include Library → Manage Libraries
 *   Search: "Arduino_LSM9DS1" by Arduino
 *
 * Serial output format (one line per sample):
 *   "X,Y\n"
 *   e.g. "45.23,-12.87\n"
 *   Units: degrees per second (dps)
 *
 * Baud rate: 500000
 *   Set your serial monitor / C++ reader to 500000 to match.
 *
 * Gyroscope range: ±2000 dps (default from library)
 * Sample rate:     119 Hz (default from library)
 */

#include <Arduino_LSM9DS1.h>

// Built-in LED used to signal ready state
#define READY_LED LED_BUILTIN

void setup() {
  pinMode(READY_LED, OUTPUT);
  digitalWrite(READY_LED, LOW);

  // High baud rate — minimize serial transmission time
  Serial.begin(500000);

  // Wait for serial port to connect (important for native USB boards)
  while (!Serial);

  // Initialize IMU
  if (!IMU.begin()) {
    // Flash rapidly to signal IMU init failure
    while (true) {
      digitalWrite(READY_LED, HIGH); delay(100);
      digitalWrite(READY_LED, LOW);  delay(100);
    }
  }

  // Signal ready — LED on solid
  digitalWrite(READY_LED, HIGH);

  // Optional: print sample rate once so the reader knows what to expect
  // Comment out if you want pure data lines only
  Serial.print("# gyro_sample_rate=");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println("Hz");
}

void loop() {
  float x, y, z;

  // Only read when a new sample is ready — avoids blocking and duplicate reads
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x, y, z);

    // Send X,Y only — Z (twist) not needed for 2D canvas
    // Use print + println to avoid String object overhead
    Serial.print(x, 2);   // 2 decimal places is plenty
    Serial.print(',');
    Serial.println(y, 2);
  }

  // No delay — let the IMU's own sample rate gate the output
  // The LSM9DS1 runs at 119 Hz by default, so loop naturally throttles
}
