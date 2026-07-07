#include <PDM.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

short sampleBuffer[256];
volatile int samplesRead = 0;

const int MIC_THRESHOLD = 200;
const int LIGHT_THRESHOLD = 50;
const float MOTION_THRESHOLD = 0.7;
const int PROX_THRESHOLD = 150;

int micLevel = 0;
int clearValue = 0;
int proxValue = 0;
float motionValue = 0;

void onPDMdata() {
    int bytesAvailable = PDM.available();
    PDM.read(sampleBuffer, bytesAvailable);
    samplesRead = bytesAvailable / 2;
}

void setup() {
    Serial.begin(115200);
    delay(1500);

    PDM.onReceive(onPDMdata);

    if (!PDM.begin(1, 16000)) {
        Serial.println("Failed to start PDM microphone.");
        while (1);
    }

    if (!IMU.begin()) {
        Serial.println("Failed to initialize IMU.");
        while (1);
    }

    if (!APDS.begin()) {
        Serial.println("Failed to initialize APDS9960 sensor.");
        while (1);
    }

    Serial.println("Workspace awareness started");
}

void loop() {
    // Microphone activity
    if (samplesRead) {
        long sum = 0;
        for (int i = 0; i < samplesRead; i++) {
            sum += abs(sampleBuffer[i]);
        }
        micLevel = sum / samplesRead;
        samplesRead = 0;
    }

    // Ambient light / clear channel
    int r, g, b, c;
    if (APDS.colorAvailable()) {
        APDS.readColor(r, g, b, c);
        clearValue = c;
    }

    // Proximity
    if (APDS.proximityAvailable()) {
        proxValue = APDS.readProximity();
    }

    // Motion from accelerometer magnitude
    float x, y, z;
    if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(x, y, z);
        motionValue = sqrt(x * x + y * y + z * z);
    }

    bool sound = micLevel > MIC_THRESHOLD;
    bool dark = clearValue < LIGHT_THRESHOLD;
    bool moving = abs(motionValue - 1.0) > MOTION_THRESHOLD;
    bool near = proxValue < PROX_THRESHOLD;
    String label;

    if (!sound && !dark && !moving && !near) {
        label = "QUIET_BRIGHT_STEADY_FAR";
    } else if (sound && !dark && !moving && !near) {
        label = "NOISY_BRIGHT_STEADY_FAR";
    } else if (!sound && dark && !moving && near) {
        label = "QUIET_DARK_STEADY_NEAR";
    } else if (sound && !dark && moving && near) {
        label = "NOISY_BRIGHT_MOVING_NEAR";
    } else {
        label = "UNCLASSIFIED";
    }

    Serial.print("raw,mic=");
    Serial.print(micLevel);
    Serial.print(",clear=");
    Serial.print(clearValue);
    Serial.print(",motion=");
    Serial.print(motionValue, 3);
    Serial.print(",prox=");
    Serial.println(proxValue);

    Serial.print("flags,sound=");
    Serial.print(sound);
    Serial.print(",dark=");
    Serial.print(dark);
    Serial.print(",moving=");
    Serial.print(moving);
    Serial.print(",near=");
    Serial.println(near);

    Serial.print("state,");
    Serial.println(label);

    delay(500);
}