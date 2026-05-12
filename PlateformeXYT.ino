#include "PlateformeXYT.h"

U8G2_SSD1327_WS_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

PlateformeXYT plateforme(2, 5, 3, 6, 4, 9, A0, A1, A3, A2, 7, 8, 11, &u8g2);

void setup() {
  plateforme.begin();
}

void loop() {
  plateforme.telecommandeIR();
}