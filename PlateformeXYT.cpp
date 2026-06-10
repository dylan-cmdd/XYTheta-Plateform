#include "PlateformeXYT.h"
#include <IRremote.hpp>

// ============================================================
//  Constructeur
// ============================================================

PlateformeXYT::PlateformeXYT(int stepX, int dirX, int stepY, int dirY, int stepTheta, int dirTheta,
                              int xlim_m, int xlim_p, int ylim_m, int ylim_p, int tlim_m, int tlim_p,
                              int recvPin, U8G2_SSD1327_WS_128X128_F_HW_I2C* u8g2) {
  
  this->stepX = stepX;          this->dirX = dirX;
  this->stepY = stepY;          this->dirY = dirY;
  this->stepTheta = stepTheta;  this->dirTheta = dirTheta;

  this->xlim_m = xlim_m;        this->xlim_p = xlim_p;
  this->ylim_m = ylim_m;        this->ylim_p = ylim_p;
  this->tlim_m = tlim_m;        this->tlim_p = tlim_p;

  this->recvPin = recvPin;
  this->u8g2 = u8g2;

  posX = 0;
  posY = 0;
  posTheta = 0;
  longueurDep = 500;
}

// ============================================================
//  Initialisation
// ============================================================

void PlateformeXYT::begin() {
  Serial.begin(115200);
  
  // Moteurs
  pinMode(stepX, OUTPUT);
  pinMode(dirX, OUTPUT);
  pinMode(stepY, OUTPUT);
  pinMode(dirY, OUTPUT);
  pinMode(stepTheta, OUTPUT);
  pinMode(dirTheta, OUTPUT);

  // Fins de course
  pinMode(xlim_m, INPUT_PULLUP);
  pinMode(xlim_p, INPUT_PULLUP);
  pinMode(ylim_m, INPUT_PULLUP);
  pinMode(ylim_p, INPUT_PULLUP);
  pinMode(tlim_m, INPUT_PULLUP);
  pinMode(tlim_p, INPUT_PULLUP);

  // Écran OLED
  u8g2->setI2CAddress(0x3D * 2);
  u8g2->begin();
  ecranUpdate();
  
  // Télécommande IR
  IrReceiver.begin(recvPin, ENABLE_LED_FEEDBACK);
}

// ============================================================
//  timeSmoother — profil en cloche avec plateau ET sécurité
// ============================================================

int PlateformeXYT::timeSmoother(int maxDelay, int minDelay, int seuilPlateau, int totalSteps, int currentStep) {
  //Securite pour faibles valeurs
  if (totalSteps <= 100) {
    return maxDelay; 
  }

  //Vitesse amortie
  float deltat = PI / (totalSteps + 1);
  float t = deltat * currentStep;
  int delayCalcule = maxDelay - (maxDelay - minDelay) * sin(t);

  //Plateau pour evider la non fluidite du mvt
  if (delayCalcule < seuilPlateau) {
    return seuilPlateau;
  }
  return delayCalcule;
}

// ============================================================
//  ecranUpdate — rafraîchit l'affichage OLED
// ============================================================

void PlateformeXYT::ecranUpdate() {
  u8g2->clearBuffer();
  u8g2->setFont(u8g2_font_6x12_tr);

  u8g2->drawStr(0, 20, "Distance X :");
  u8g2->setCursor(80, 20);
  u8g2->print(posX);

  u8g2->drawStr(0, 45, "Distance Y :");
  u8g2->setCursor(80, 45);
  u8g2->print(posY);

  u8g2->drawStr(0, 70, "Angle Theta:");
  u8g2->setCursor(80, 70);
  u8g2->print(posTheta);

  u8g2->drawStr(0, 95, "Pas/cmd :");
  u8g2->setCursor(80, 95);
  u8g2->print(longueurDep);

  u8g2->sendBuffer();
}

// ============================================================
//  moveMotor — déplace un moteur avec profil de vitesse amorti
//    stepPin   : broche STEP du driver
//    dirPin    : broche DIR du driver
//    limitPin  : fin de course à surveiller
//    direction : true = sens positif, false = sens négatif
// ============================================================

void PlateformeXYT::moveMotor(int stepPin, int dirPin, int limitPin, bool direction) {
  digitalWrite(dirPin, direction ? HIGH : LOW);

  int maxD, minD, seuil;

  if (longueurDep <= 100) {
    maxD = 2000;  minD = 2000;  seuil = 2000; 
  } else if (longueurDep <= 300) {
    maxD = 2500;  minD = 1200;  seuil = 1000; 
  } else {
    maxD = 3000;  minD = 800;   seuil = 800; 
  }

  for (int x = 0; x < longueurDep; x++) {

     // ── Arrêt immédiat si la butée est touchée ──────────────
    if (digitalRead(limitPin) == LOW) {
      return;
    }
    // ── courbe cloche inverse de la vitesse ─────────────────
    // maxD = max courbe cloche inversee
    // seuil = fonction lineaire du plateau
    // minD = min courbe cloche inversee
    int wait = timeSmoother(maxD, minD, seuil, longueurDep, x);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(wait);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(wait);

    // ── Mise à jour de la position sur l'ecran ──────────────
    if (stepPin == stepX)          direction ? posX++ : posX--;
    else if (stepPin == stepY)     direction ? posY++ : posY--;
    else if (stepPin == stepTheta) direction ? posTheta++ : posTheta--;
  }
}

// ============================================================
//  resetMotor — ramène tous les axes à l'origine (0, 0, 0)
// ============================================================

void PlateformeXYT::resetMotor() {
  int sauvegarde = longueurDep;
  longueurDep = 10000;

  u8g2->clearBuffer();
  u8g2->setFont(u8g2_font_6x12_tr);
  u8g2->drawStr(20, 64, "Mise a zero...");
  u8g2->sendBuffer();

  moveMotor(stepY, dirY, ylim_m, false);
  moveMotor(stepX, dirX, xlim_m, false);

  posX = 0;
  posY = 0;
  posTheta = 0;

  longueurDep = sauvegarde;

  ecranUpdate();
}

// ============================================================
//  Setter pour longueurDep
// ============================================================

void PlateformeXYT::setLongueurDep(int val) {
  longueurDep = val;
}

// ============================================================
//  Gestion des commandes reçues par le PC (liaison Série)
// ============================================================

void PlateformeXYT::gererSerial() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "RESET") {
      resetMotor();
      Serial.println("RESET_OK");
    } 
    // ---- AJOUT DE LA COMMANDE POS ICI ----
    else if (cmd == "POS") {
      Serial.print("X : "); Serial.print(posX);
      Serial.print(" Y : "); Serial.print(posY);
      Serial.print(" T : "); Serial.println(posTheta);
    }
    // --------------------------------------
    else if (cmd.startsWith("PAS")) {
      int pas = cmd.substring(3).toInt();
      setLongueurDep(pas);
      Serial.println("PAS_OK");
    } 
    else if (cmd == "XP") {
      moveMotor(stepX, dirX, xlim_p, true);
      Serial.println("XP_OK");
    } 
    else if (cmd == "XM") {
      moveMotor(stepX, dirX, xlim_m, false);
      Serial.println("XM_OK");
    } 
    else if (cmd == "YP") {
      moveMotor(stepY, dirY, ylim_p, true);
      Serial.println("YP_OK");
    } 
    else if (cmd == "YM") {
      moveMotor(stepY, dirY, ylim_m, false);
      Serial.println("YM_OK");
    } 
    else if (cmd == "TP") {
      moveMotor(stepTheta, dirTheta, tlim_p, true);
      Serial.println("TP_OK");
    } 
    else if (cmd == "TM") {
      moveMotor(stepTheta, dirTheta, tlim_m, false);
      Serial.println("TM_OK");
    }

    ecranUpdate();
  }
}

// ============================================================
//  Gestion de la télécommande IR
// ============================================================

void PlateformeXYT::telecommandeIR() {
  if (IrReceiver.decode()) {

    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    switch (IrReceiver.decodedIRData.decodedRawData) {

      // ── Réglage du nombre de pas par commande ─────────────
      case 0xE916FF00: setLongueurDep(1);    break;
      case 0xE619FF00: setLongueurDep(50);   break;
      case 0xF20DFF00: setLongueurDep(150);  break;
      case 0xA15EFF00: setLongueurDep(500);  break;
      case 0xF708FF00: setLongueurDep(2000); break;

      // ── Axe Y ─────────────────────────────────────────────
      case 0xB946FF00: moveMotor(stepY, dirY, ylim_p);        break;
      case 0xEA15FF00: moveMotor(stepY, dirY, ylim_m, false); break;

      // ── Axe X ─────────────────────────────────────────────
      case 0xBC43FF00: moveMotor(stepX, dirX, xlim_p);        break;
      case 0xBB44FF00: moveMotor(stepX, dirX, xlim_m, false); break;

      // ── Axe Theta ─────────────────────────────────────────
      case 0xF30CFF00: moveMotor(stepTheta, dirTheta, tlim_p);        break;
      case 0xE718FF00: moveMotor(stepTheta, dirTheta, tlim_m, false); break;

      // ── Reset vers origine ────────────────────────────────
      case 0xBF40FF00: resetMotor(); break;
    }

    ecranUpdate(); // Rafraîchit l'écran après chaque action
    IrReceiver.resume(); // Prêt pour la prochaine commande IR
  }
}
