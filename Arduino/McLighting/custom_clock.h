
#include "font.h"
#include <time.h>
#include "ambient.h"

char monthNames[12][4] = {"JAN", "FEB", "MAR", "APR", "MAJ", "JUN", "JUL", "AVG", "SEP", "OKT", "NOV", "DEC"};
extern LEDState main_color;
int dst, prevSec, prevBrightness;
LEDState drawColor;

// Given H,S,L in range of 0-1
// Returns a Color (RGB struct) in range of 0-255
LEDState HSL2RGB(double h, double sl, double l)
{
    double v;
    double r,g,b;

    r = l;   // default to gray
    g = l;
    b = l;
    v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
    if (v > 0)
    {
        double m;
        double sv;
        int sextant;
        double fract, vsf, mid1, mid2;

        m = l + l - v;
        sv = (v - m ) / v;
        h *= 6.0;
        sextant = (int)h;
        fract = h - sextant;
        vsf = v * sv * fract;
        mid1 = m + vsf;
        mid2 = v - vsf;
        switch (sextant)
        {
            case 0:
                r = v;
                g = mid1;
                b = m;
                break;
            case 1:
                r = mid2;
                g = v;
                b = m;
                break;
            case 2:
                r = m;
                g = v;
                b = mid1;
                break;
            case 3:
                r = m;
                g = mid2;
                b = v;
                break;
            case 4:
                r = mid1;
                g = m;
                b = v;
                break;
            case 5:
                r = v;
                g = m;
                b = mid2;
                break;
        }

    }

    return {(uint8_t)(r * 255.0), (uint8_t)(g * 255.0), (uint8_t)(b * 255.0)};
}

// Given a Color (RGB Struct) in range of 0-255
// Return H,S,L in range of 0-1
void RGB2HSL(LEDState rgb, double *h, double *s, double *l)
{
    double r = rgb.red/255.0;
    double g = rgb.green/255.0;
    double b = rgb.blue/255.0;
    double v;
    double m;
    double vm;
    double r2, g2, b2;

    *h = 0; // default to black
    *s = 0;
    *l = 0;
    v = max(r,g);
    v = max(v,b);
    m = min(r,g);
    m = min(m,b);
    *l = (m + v) / 2.0;

    if (*l <= 0.0) return;

    vm = v - m;
    *s = vm;
    if (*s > 0.0)
    {
        *s /= (*l <= 0.5) ? (v + m ) : (2.0 - v - m) ;
    }
    else return;
    r2 = (v - r) / vm;
    g2 = (v - g) / vm;
    b2 = (v - b) / vm;
    if (r == v)
    {
        *h = (g == m ? 5.0 + b2 : 1.0 - g2);
    }
    else if (g == v)
    {
        *h = (b == m ? 1.0 + r2 : 3.0 - b2);
    }
    else
    {
        *h = (r == m ? 3.0 + g2 : 5.0 - r2);
    }
    *h /= 6.0;
}

LEDState getComplement(LEDState color) {
    double h, s, l;
    RGB2HSL(color, &h, &s, &l);
    h += 0.5;
    if (h > 1.0) h -= 1.0;
    return HSL2RGB(h, s, l);
}

int getTimeOffset(struct tm *tm){
  if((tm->tm_mon > 2 && tm->tm_mon < 9) || 
          (tm->tm_mon == 2 && tm->tm_mday >= (31-((int)(((5*tm->tm_year)/4)+4)%7))) || 
          (tm->tm_mon == 9 && tm->tm_mday < (31-((int)(((5*tm->tm_year)/4)+1)%7)))) return 3600;
  else return 0;
}

void clockSwitchMode(){
    if(strip) strip->setMode(56);
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");
}

int ledIndex(int x, int y) {
    x = 31 - x;
    if (x % 2 == 0) return (x * 8) + 7 - y;
    else return (x * 8) + y;
}

void drawText(int startX, char *text) {
    for (int idx = 0; idx < strlen(text); idx++) {
        char c = text[idx];
        int start = c * 8;
        for (int row = 0; row < 8; row++) {
            char rowData = console_font_6x8[start + row];
            for (int col = 0; col < 6; col++) {
                int x = (idx * 6) + col + startX;
                int y = row;
                if (x >= 32) continue;
                LEDState color = {0, 0, 0};
                if (rowData & (128 >> col)) {
                    color = drawColor;
                }
                strip->setPixelColor(ledIndex(x, y), color.red, color.green, color.blue);
            }
        }
    }
}

void drawClock()
{
    time_t now = time(nullptr);
    struct tm *currentTime = localtime(&now);

    int newDst = getTimeOffset(currentTime);
    if (dst != newDst) {
        dst = newDst;
        configTime(3600, dst, "pool.ntp.org", "time.nist.gov");
    }
    float ill = Ambient_GetIlluminance();
    int seconds = currentTime->tm_sec;
    if (seconds == prevSec) {
        int brightness = (int)sqrt(ill);
        if (brightness < 1) brightness = 1;
        if (brightness > 100) brightness = 100;
        if (brightness != prevBrightness) {
            prevBrightness = brightness;
            strip->setBrightness(brightness);
            Serial.printf("Setting brightness to: %d (based on illuminance %f)\n", brightness, ill);
        }
        return;
    }
    prevSec = seconds;

    if (ill > 1.0f) {
        drawColor = main_color;
    }
    else drawColor = {255, 0, 0};

    strip->setColor(0, 0, 0);
    strip->mode_static();
    LEDState complement = getComplement(main_color);

    char *text = "      ";
    if ((seconds > 10 && seconds < 16) || (seconds > 30 && seconds < 36)) {
        sprintf(text, "%2d", currentTime->tm_mday);
        drawText(0, text);
        drawText(14, monthNames[currentTime->tm_mon]);
        strip->setPixelColor(ledIndex(13, 6), drawColor.red, drawColor.green, drawColor.blue);
    }
    else if ((seconds > 20 && seconds < 26) || (seconds > 40 && seconds < 46)) {
        float temperature = Ambient_GetTemperature();
        if (!isnan(temperature)) {
            sprintf(text, "%2d °C", (int)(temperature + 0.5));
            drawText(1, text);
        }
    }
    else {
        char separation = (currentTime->tm_sec % 2 == 0) ? ':' : ' ';
        sprintf(text, "%2d%c%02d", currentTime->tm_hour, separation, currentTime->tm_min);
        
        drawText(1, text);
    }
    if (ill > 1.0f) {
        for (int i = 0; i < 30; i++) {
            if (i * 2 <= seconds) {
                strip->setPixelColor(ledIndex(i + 1, 7), complement.red, complement.green, complement.blue);
            }
        }
    }
}

uint16_t clockEffect() {
  drawClock();
  return (strip->getSpeed() / WS2812FXStripSettings.stripSize);
}
