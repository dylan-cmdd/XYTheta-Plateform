#ifndef PlateformeXYT_h
#define PlateformeXYT_h

#include <Arduino.h>
#include <U8g2lib.h>
#include <IRremote.h>

class PlateformeXYT {
  private:
    int stepX, dirX;
    int stepY, dirY;
    int stepTheta, dirTheta;

    int xlim_m, xlim_p;
    int ylim_m, ylim_p;
    int tlim_m, tlim_p;

    int recvPin;

    long posX;
    long posY;
    long posTheta;
    int longueurDep;

    U8G2_SSD1327_WS_128X128_F_HW_I2C* u8g2;

    int timeSmoother(int maxDelay, int minDelay, int seuilPlateau, int totalSteps, int currentStep);

  public:
    PlateformeXYT(int stepX, int dirX, int stepY, int dirY, int stepTheta, int dirTheta,
                  int xlim_m, int xlim_p, int ylim_m, int ylim_p, int tlim_m, int tlim_p,
                  int recvPin, U8G2_SSD1327_WS_128X128_F_HW_I2C* u8g2);

    void begin();
    void ecranUpdate();
    void moveMotor(int stepPin, int dirPin, int limitPin, bool direction = true);
    void resetMotor();
    void setLongueurDep(int val);
    void telecommandeIR();
};

#endif