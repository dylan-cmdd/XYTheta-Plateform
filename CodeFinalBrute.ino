#include <IRremote.h>
#include <U8g2lib.h>
#include <Wire.h>

// ── Broches moteurs ─────────────────────────────────────────
const int StepX = 2;
const int DirX = 5;
const int StepY = 3;
const int DirY = 6;
const int StepTheta = 4;
const int DirTheta = 9;

// ── Fins de course ──────────────────────────────────────────
//   m = butée côté négatif   p = butée côté positif
const int Xlim_m = A0;
const int Xlim_p = A1;
const int Ylim_m = A3;
const int Ylim_p = A2;
const int Tlim_m = 7;   
const int Tlim_p = 8;  

// ── Télécommande IR ─────────────────────────────────────────
const int RECV_PIN  = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

// ── Écran OLED (SPI) ────────────────────────────────────────
//U8G2_SSD1327_WS_128X128_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 7, /* reset=*/ 8); //Mode SPI (CS et RST coupés)
U8G2_SSD1327_WS_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //Mode I2C (SDA=A4, SCL=A5)

// ── Variables ──────────────────────────────────────
long posX = 0;
long posY = 0;
long posTheta = 0;
int  longueur_dep = 500;   // Nombre de pas par commande

// // ============================================================
// //  oldtimeSmoother — profil de vitesse en cloche (sinus)
// // ============================================================
// int timeSmoother(int maxDelay, int minDelay, int totalSteps, int currentStep) {
//   float deltat = PI / (totalSteps + 1);
//   float t = deltat * currentStep;
//   return maxDelay - (maxDelay - minDelay) * sin(t);
// }

// ============================================================
//  timeSmoother — profil en cloche avec plateau ET sécurité
// ============================================================
int timeSmoother(int maxDelay, int minDelay, int seuilPlateau, int totalSteps, int currentStep) {
  
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
void ecranUpdate() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  u8g2.drawStr(0, 20, "Distance X :");
  u8g2.setCursor(80, 20);
  u8g2.print(posX);

  u8g2.drawStr(0, 45, "Distance Y :");
  u8g2.setCursor(80, 45);
  u8g2.print(posY);

  u8g2.drawStr(0, 70, "Angle Theta:");
  u8g2.setCursor(80, 70);
  u8g2.print(posTheta);

  u8g2.drawStr(0, 95, "Pas/cmd :");
  u8g2.setCursor(80, 95);
  u8g2.print(longueur_dep);

  u8g2.sendBuffer();
}
// ============================================================
//  moveMotor — déplace un moteur avec profil de vitesse amorti
//    stepPin   : broche STEP du driver
//    dirPin    : broche DIR du driver
//    limitPin  : fin de course à surveiller
//    direction : true = sens positif, false = sens négatif
// ============================================================
void moveMotor(int stepPin, int dirPin, int limitPin, bool direction = true) {
  digitalWrite(dirPin, direction ? HIGH : LOW);

  int maxD, minD, seuil;

  if (longueur_dep <= 100) {
    maxD = 2000;  minD = 2000;  seuil = 2000;  // Vitesse constante, lente
  } else if (longueur_dep <= 300) {
    maxD = 2500;  minD = 1200;  seuil = 1000;  // Accélération douce
  } else {
    maxD = 3000;  minD = 800;   seuil = 800;   // Pleine accélération
  }

  for (int x = 0; x < longueur_dep; x++) {

    // ── Arrêt immédiat si la butée est touchée ──────────────
    if (digitalRead(limitPin) == LOW) {
      return;
    }
    // ── courbe cloche inverse de la vitesse ─────────────────
    // maxD = max courbe cloche inversee
    // seuil = fonction lineaire du plateau
    // minD = min courbe cloche inversee
    int wait = timeSmoother(maxD, minD, seuil, longueur_dep, x);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(wait);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(wait);

    // ── Mise à jour de la position sur l'ecran ──────────────
    if (stepPin == StepX) {
      direction ? posX++ : posX--;
    } else if (stepPin == StepY) {
      direction ? posY++ : posY--;
    } else if (stepPin == StepTheta) {
      direction ? posTheta++ : posTheta--;
    }
  }
}

// ============================================================
//  resetMotor — ramène tous les axes à l'origine (0, 0, 0)
// ============================================================

void resetMotor() {
  int sauvegarde = longueur_dep;
  longueur_dep = 2000;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(20, 64, "Mise a zero...");
  u8g2.sendBuffer();

  moveMotor(StepY, DirY, Ylim_m, false);
  moveMotor(StepX, DirX, Xlim_m, false);

  // ── On force les positions à zéro ──
  posX = 0;
  posY = 0;
  posTheta = 0;

  longueur_dep = sauvegarde;

  // ── On réutilise la fonction d'affichage au lieu de tout réécrire ──
  ecranUpdate();
}


// ============================================================
//  setup
// ============================================================
void setup() {
  Serial.begin(115200);

  // Moteurs
  pinMode(StepX, OUTPUT);
  pinMode(DirX, OUTPUT);
  pinMode(StepY, OUTPUT);
  pinMode(DirY, OUTPUT);
  pinMode(StepTheta, OUTPUT);
  pinMode(DirTheta, OUTPUT);

  // Fins de course (INPUT_PULLUP → LOW quand actif)
  pinMode(Xlim_m, INPUT_PULLUP);
  pinMode(Xlim_p, INPUT_PULLUP);
  pinMode(Ylim_m, INPUT_PULLUP);
  pinMode(Ylim_p, INPUT_PULLUP);
  pinMode(Tlim_m, INPUT_PULLUP);
  pinMode(Tlim_p, INPUT_PULLUP);

  // Écran OLED
  u8g2.setI2CAddress(0x3D * 2); // 0x3C est l'adresse la plus courante. Le *2 est requis par u8g2.
  u8g2.begin();
  ecranUpdate();

  // Télécommande IR
  IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); // Le paramètre active un clignotement de la LED de la carte à chaque réception
}

// ============================================================
//  oldloop
// ============================================================
// void loop() {
//   if (irrecv.decode(&results)) {
//     Serial.println(results.value, HEX);
//     switch (results.value) {

//       // ── Réglage du nombre de pas par commande ─────────────
//       case 0xFF6897: longueur_dep = 1;   break; // Bouton 1 → micro-pas
//       case 0xFF9867: longueur_dep = 50;  break; // Bouton 2 → pas moyen
//       case 0xFFB04F: longueur_dep = 150; break; // Bouton 3 → grand pas

//       // ── Axe Y ─────────────────────────────────────────────
//       case 0xFF629D: moveMotor(StepY, DirY, Ylim_p);        break; // Haut
//       case 0xFFA857: moveMotor(StepY, DirY, Ylim_m, false); break; // Bas

//       // ── Axe X ─────────────────────────────────────────────
//       case 0xFF22DD: moveMotor(StepX, DirX, Xlim_p);        break; // Gauche
//       case 0xFFC23D: moveMotor(StepX, DirX, Xlim_m, false); break; // Droite

//       // ── Axe Theta ─────────────────────────────────────────
//       case 0xFF02FD: moveMotor(StepTheta, DirTheta, Tlim_p);        break; // Rotation +
//       case 0xFF42BD: moveMotor(StepTheta, DirTheta, Tlim_m, false); break; // Rotation −

//       // ── Reset vers origine ────────────────────────────────
//       case 0xFF38C7: resetMotor(); break; // Bouton OK/Centre
//     }

//     ecranUpdate();          // Rafraîchit l'écran après chaque action
//     irrecv.resume();        // Prêt pour la prochaine commande IR
//   }
// }

void loop() {
  if (IrReceiver.decode()) {
    
    // Affiche le code reçu dans le moniteur série (INDISPENSABLE pour trouver tes boutons)
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX); 

    switch (IrReceiver.decodedIRData.decodedRawData) {

      // ── Réglage du nombre de pas par commande ─────────────
      case 0xE916FF00: longueur_dep = 1;   break; // Bouton 1 → micro-pas
      case 0xE619FF00: longueur_dep = 50;  break; // Bouton 2 → pas moyen
      case 0xF20DFF00: longueur_dep = 150; break; // Bouton 3 → grand pas
      case 0xA15EFF00: longueur_dep = 500; break; // Bouton 6 → tres grand pas
      case 0xF708FF00: longueur_dep = 2000; break; // Bouton 7 → tres tres grand pas

      // ── Axe Y ─────────────────────────────────────────────
      case 0xB946FF00: moveMotor(StepY, DirY, Ylim_p);        break; // Haut
      case 0xEA15FF00: moveMotor(StepY, DirY, Ylim_m, false); break; // Bas

      // ── Axe X ─────────────────────────────────────────────
      case 0xBB44FF00: moveMotor(StepX, DirX, Xlim_p);        break; // Gauche
      case 0xBC43FF00: moveMotor(StepX, DirX, Xlim_m, false); break; // Droite

      // ── Axe Theta ─────────────────────────────────────────
      case 0xF30CFF00: moveMotor(StepTheta, DirTheta, Tlim_p);        break; // Rotation + Bouton 4
      case 0xE718FF00: moveMotor(StepTheta, DirTheta, Tlim_m, false); break; // Rotation − Bouton 5

      // ── Reset vers origine ────────────────────────────────
      case 0xBF40FF00: resetMotor(); break; // Bouton OK/Centre
    }

    ecranUpdate();          // Rafraîchit l'écran après chaque action
    IrReceiver.resume();    // Prêt pour la prochaine commande IR
  }
}
