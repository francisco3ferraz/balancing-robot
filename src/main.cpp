#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 21
#define SDL_PIN 22

void setup() {
    Serial.begin(115200);
    delay(1000);
    Wire.begin(SDA_PIN, SDL_PIN);
}

void loop() {
    Wire.beginTransmission(0x68);
    Serial.printf("%d\n", Wire.endTransmission());
    delay(1000);
}
