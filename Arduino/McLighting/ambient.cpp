#include "ambient.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_AM2320.h"
#include <BH1750.h>

BH1750 lightMeter;
Adafruit_AM2320 am2320 = Adafruit_AM2320();
float temperatureBuffer[5];
float humidityBuffer[5];
float illuminanceBuffer[5];
int tempIdx = 0;
int humIdx = 0;
int illuIdx = 0;
float tTemperature, tHumidity, tIlluminance;
float temperature, humidity, illuminance;
unsigned long prevMillis = 0;

void Ambient_Initialize() {
    am2320.begin();
    lightMeter.begin();
    temperature = am2320.readTemperature();
    humidity = am2320.readHumidity();
    illuminance = lightMeter.readLightLevel();
    for(int i = 0; i < 5; i++) {
        temperatureBuffer[i] = temperature;
        humidityBuffer[i] = humidity;
        illuminanceBuffer[i] = illuminance;
    }
}

float Ambient_GetIlluminance() {
    return illuminance;
}

float Ambient_GetTemperature() {
    return temperature;
}

float Ambient_GetHumidity() {
    return humidity;
}

float addValue(float *buffer, int *index, float value) {
    buffer[*index] = value;
    float sum = 0.0f;
    for(int i = 0; i < 5; i++) {
        sum += buffer[i] / 5.0f;
    }
    *index = ((*index) + 1) % 5;
    return sum;
}

int Ambient_Process() {
    if (millis() - prevMillis >= 1000) {
        prevMillis = millis();
        temperature = addValue(temperatureBuffer, &tempIdx, am2320.readTemperature());
        humidity = addValue(humidityBuffer, &humIdx, am2320.readHumidity());
        illuminance = addValue(illuminanceBuffer, &illuIdx, lightMeter.readLightLevel());
        int change = 0;
        if (fabs(tTemperature - temperature) > 0.5
            || fabs(tHumidity - humidity) > 5.0
            || fabs(tIlluminance - illuminance) > 10.0) {
            tTemperature = temperature;
            tHumidity = humidity;
            tIlluminance = illuminance;
            change = 1;
            Serial.printf("Temperature: %f\n", temperature);
            Serial.printf("Humidity: %f\n", humidity);
            Serial.printf("Illuminance: %f\n", illuminance);
        }
        return change;
    }
    return 0;
}