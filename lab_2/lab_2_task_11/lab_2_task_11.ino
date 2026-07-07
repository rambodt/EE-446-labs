#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

const float HUMID_JUMP_THRESHOLD = 10;
const float TEMP_RISE_THRESHOLD = 1.5;
const float MAG_SHIFT_THRESHOLD = 20.0;
const int LIGHT_CHANGE_THRESHOLD = 40;
const unsigned long COOLDOWN_MS = 1500;

float baseRh = 0;
float baseTemp = 0;
float baseMag = 0;
int baseR = 0, baseG = 0, baseB = 0, baseClear = 0;

unsigned long lastEventTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1500);

    if (!HS300x.begin()) {
        Serial.println("Failed to initialize humidity/temperature sensor.");
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

    delay(1000);

    baseRh = HS300x.readHumidity();
    baseTemp = HS300x.readTemperature();

    float mx, my, mz;
    IMU.readMagneticField(mx, my, mz);
    baseMag = sqrt(mx * mx + my * my + mz * mz);

    if (APDS.colorAvailable()) {
        APDS.readColor(baseR, baseG, baseB, baseClear);
    }

    Serial.println("Event monitor started");
}

void loop() {
    float rh = HS300x.readHumidity();
    float temp = HS300x.readTemperature();

    float mx, my, mz;
    float mag = baseMag;

    if (IMU.magneticFieldAvailable()) {
        IMU.readMagneticField(mx, my, mz);
        mag = sqrt(mx * mx + my * my + mz * mz);
    }

    int r = baseR, g = baseG, b = baseB, clear = baseClear;

    if (APDS.colorAvailable()) {
        APDS.readColor(r, g, b, clear);
    }

    bool humid_jump = abs(rh - baseRh) > HUMID_JUMP_THRESHOLD;
    bool temp_rise = (temp - baseTemp) > TEMP_RISE_THRESHOLD;
    bool mag_shift = abs(mag - baseMag) > MAG_SHIFT_THRESHOLD;

    bool light_or_color_change =
        abs(r - baseR) > LIGHT_CHANGE_THRESHOLD ||
        abs(g - baseG) > LIGHT_CHANGE_THRESHOLD ||
        abs(b - baseB) > LIGHT_CHANGE_THRESHOLD ||
        abs(clear - baseClear) > LIGHT_CHANGE_THRESHOLD;

    String label = "BASELINE_NORMAL";

    bool cooldownOver = millis() - lastEventTime > COOLDOWN_MS;

    if (cooldownOver) {
        if (mag_shift) {
            label = "MAGNETIC_DISTURBANCE_EVENT";
            lastEventTime = millis();
        } else if (light_or_color_change) {
            label = "LIGHT_OR_COLOR_CHANGE_EVENT";
            lastEventTime = millis();
        } else if (humid_jump || temp_rise) {
            label = "BREATH_OR_WARM_AIR_EVENT";
            lastEventTime = millis();
        }
    }

    Serial.print("raw,rh=");
    Serial.print(rh, 2);
    Serial.print(",temp=");
    Serial.print(temp, 2);
    Serial.print(",mag=");
    Serial.print(mag, 2);
    Serial.print(",r=");
    Serial.print(r);
    Serial.print(",g=");
    Serial.print(g);
    Serial.print(",b=");
    Serial.print(b);
    Serial.print(",clear=");
    Serial.println(clear);

    Serial.print("flags,humid_jump=");
    Serial.print(humid_jump);
    Serial.print(",temp_rise=");
    Serial.print(temp_rise);
    Serial.print(",mag_shift=");
    Serial.print(mag_shift);
    Serial.print(",light_or_color_change=");
    Serial.println(light_or_color_change);

    Serial.print("event,");
    Serial.println(label);

    delay(500);
}