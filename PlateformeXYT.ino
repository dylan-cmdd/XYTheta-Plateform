#include "PlateformeXYT.h"

U8G2_SSD1327_WS_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

PlateformeXYT plateforme(7, 6, 14, 15, 3, 2, A1, A2, A0, A3, A4, A5, 11, &u8g2);

void setup() {
  plateforme.begin();
}

void loop() {
  plateforme.telecommandeIR();
  plateforme.gererSerial();
}




