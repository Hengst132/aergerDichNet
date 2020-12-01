
#define VERSION2
//#define LEDS8MM
//#define POCKET
#define SELECT_BUTTON 4
/*
Feld: Start im inneren oben rechts entgegen des Uhrzeigersinns:
       ___
      |   |
      v   ^
 __<__|   | _<__
|         ^     |
|__>__     __>__|
      |   |
      v   ^
      |___|
Fig. 1
*/
#define FIELD_OUT 6
/*
Ziel startet immer innen.
Zuerst nach rechts (vgl. Fig. 1), dann jedoch im Uhrzeigersinn:
        P4 15
           14
           13
P3         12           P1
11 10 09 08   >00 01 02 03
           04
           05
           06
        P2 07
Fig. 2
*/
#define FINISH_OUT 7
/*
Pixelanordnung des Wuerfels:
Arduino -> 0-----1
                 |
           4--3--2
           |
           5-----6
Fig. 3
*/
#define DICE_OUT 5

//Positionen der Spieler in Fig. 2
#define BUTTON_P1 8
#define BUTTON_P2 11
#define BUTTON_P3 10
#define BUTTON_P4 9



#define MINBRIGHTNESS 30
#define MAXBRIGHTNESS 255

#define HOME1   -1
#define HOME2   -2
#define HOME3   -3
#define HOME4   -4
#define EXIT     0
#define FINISH1 40
#define FINISH2 41
#define FINISH3 42
#define FINISH4 43

#include <Adafruit_NeoPixel.h>

#ifdef LEDS8MM
Adafruit_NeoPixel field = Adafruit_NeoPixel(56, FIELD_OUT, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel finish = Adafruit_NeoPixel(16, FINISH_OUT, NEO_RGB + NEO_KHZ800);
#else
Adafruit_NeoPixel field = Adafruit_NeoPixel(56, FIELD_OUT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel finish = Adafruit_NeoPixel(16, FINISH_OUT, NEO_GRB + NEO_KHZ800);
#endif
Adafruit_NeoPixel dice = Adafruit_NeoPixel(7, DICE_OUT, NEO_GRB + NEO_KHZ800);

const uint8_t MAXPLAYERS = 4;
const uint8_t MINPLAYERS = 2;
const uint8_t FIGURES = 4;
uint32_t player_colors[MAXPLAYERS] = { 0xFF0000, 0xFFFF00, 0x00FF00, 0x0000FF };
uint32_t undimmed_colors[MAXPLAYERS] = { 0xFF0000, 0xFFFF00, 0x00FF00, 0x0000FF };
int8_t player_positions[MAXPLAYERS][FIGURES];
uint8_t home_players[FIGURES];
uint8_t activePlayer = 0;
uint8_t players = MAXPLAYERS;
uint8_t activeFigure = 0;
int8_t selection = 0;
boolean autoPlay[MAXPLAYERS];
uint8_t playerRanking[MAXPLAYERS];
uint8_t wonPlayers;
uint8_t dimmed = MINBRIGHTNESS;
byte player_wheel[] = { 0, 64, 128, 192 };

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("setup()");
  field.begin();
  field.show();
  finish.begin();
  finish.show();
  dice.begin();
  dice.show();
  pinMode(SELECT_BUTTON, INPUT_PULLUP);
#ifndef VERSION2
  pinMode(BUTTON_P1, INPUT_PULLUP);
  pinMode(BUTTON_P2, INPUT_PULLUP);
  pinMode(BUTTON_P3, INPUT_PULLUP);
  pinMode(BUTTON_P4, INPUT_PULLUP);
#endif
  delay(1000);
  byte c = 0;
  while (!select()) {
    Serial.println("start-up animation");
    uint32_t color = dim(Wheel(c += 8), 20);
    for (uint8_t i = 0; i < 8; i++) {
      dice.setPixelColor(i, color);
    }
    dice.show();
    for (uint8_t i = 0; i < 5; i++) {
      for (uint8_t j = 0; j < 4; j++) {
        field.setPixelColor(i + 14 * j, color);
        field.setPixelColor(14 * (4 - j) - i, color);
        finish.setPixelColor(i + j * 4, color);
      }

      field.show();
      finish.show();
      delay(50);
      if (select()) break;
    }
#ifndef POCKET
    if (select()) break;
    for (uint8_t i = 0; i < 3; i++) {
      for (uint8_t j = 0; j < 4; j++) {
        field.setPixelColor(4 + i + 14 * j, color);
        field.setPixelColor(14 * (4 - j) - 5 - i, color);
      }
      field.show();
      delay(50);
      if (select()) break;
    }
    if (select()) break;
    for (uint8_t i = 2; i < 3; i--) {
      for (uint8_t j = 0; j < 4; j++) {
        field.setPixelColor(4 + i + 14 * j, 0);
        field.setPixelColor(14 * (4 - j) - 5 - i, 0);
      }
      field.show();
      delay(50);
      if (select()) break;
    }
#endif
    if (select()) break;
    for (uint8_t i = 4; i < 5; i--) {
      for (uint8_t j = 0; j < 4; j++) {
        field.setPixelColor(i + 14 * j, 0);
        field.setPixelColor(14 * (4 - j) - i, 0);
        if (i < 4)
          finish.setPixelColor(i + j * 4, 0);
      }
      field.show();
      finish.show();
      delay(50);
      if (select()) break;
    }
  }
  
  Serial.println("start-up beendet");
  resetGame();
  setDefault();
  show();
}

void resetGame() {
  Serial.println("resetGame()");
  bool waitForSelect = 1;
  uint32_t selectCounter = 0;
  randomSeed(millis());
  wonPlayers = 0;
  Serial.println("Spieler zurueck setzem...");
  for (uint8_t player = 0; player < MAXPLAYERS; player++) {
    player_positions[player][0] = -1;
    player_positions[player][1] = -2;
    player_positions[player][2] = -3;
    player_positions[player][3] = -4;
    playerRanking[player] = 0;
    undimmed_colors[player] = Wheel(player_wheel[player]);
    player_colors[player] = dim(undimmed_colors[player], dimmed);
  }
  setDefault();
  show();
  Serial.println("Spieler zurueck gesetzt");
  selection = 0;
  selectCounter = 0;

  Serial.println("Helligkeit auswaehlen...");
  Serial.println("auf Wuerfel sichtbar");
  //Helligkeit
  while (true) {
    if (select()) {
      if (!waitForSelect) {
        if (selectCounter == 0) {
          selectCounter = millis();
        }
        else if (millis() - selectCounter > 300) {
          break;
        }
      }
    }
    else {
      if (selectCounter > 0) {
        dimmed += 5;
        if (dimmed < MINBRIGHTNESS) //Overflow
          dimmed = MINBRIGHTNESS;
        for (uint8_t player = 0; player < MAXPLAYERS; player++)
          player_colors[player] = dim(Wheel(player_wheel[player]), dimmed);
        setDefault();
        show();
      }
      selectCounter = 0;
      waitForSelect = 0;
    }
    int8_t b = millis() / 10;
    for (uint8_t i = 0; i < 7; i++) {
      dice.setPixelColor(i, dim(0xFF0000, abs(b)));
    }
    show();
  }
  Serial.println("Helligkeit ausgewaehlt");
  waitForSelect = 1;
  selectCounter = 0;
  
  Serial.println("Spieleranzahl auswaehlen... (mit select)");
  Serial.println("kurz drücken um zu wechseln");
  Serial.println("300 ms drücken bestätigen");
  //Spielerzahl
  diceNumber(players);
  while (true) {
    if (select()) {
      if (!waitForSelect) {
        if (selectCounter == 0) {
          selectCounter = millis();
        }
        else if (millis() - selectCounter > 300) {
          break;
        }
      }
    }
    else {
      if (selectCounter > 0) {
        players++;
        if (players > MAXPLAYERS) players = MINPLAYERS;
        setDefault();
        diceNumber(players);
        show();
        Serial.print(players);
        Serial.println(" Spieler? ");
      }
      selectCounter = 0;
      waitForSelect = 0;
    }
  }
  Serial.print(players);
  Serial.println("  Spieler ausgewaehlt");
  waitForSelect = 1;
  selectCounter = 0;

  Serial.println("Farbe auswaehlen ???...");
  //Farbe
  diceNumber(0);
  byte c = 0;
  while (true) {
    if (select()) {
      if (!waitForSelect) {
        if (selectCounter == 0) {
          selectCounter = millis();
        }
        else if (millis() - selectCounter > 300) {
          break;
        }
      }
    }
    else {
      waitForSelect = 0;
      selectCounter = 0;
    }
    for (uint8_t player = 0; player < players; player++) {
      if (getTouch(player)) {
        player_wheel[player]++;
        undimmed_colors[player] = Wheel(player_wheel[player]);
        player_colors[player] = dim(undimmed_colors[player], dimmed);
      }
      setDefault();
      show();
    }
    uint32_t color = dim(Wheel(c++), dimmed);
    uint8_t m = (c / 4) % 4;
    dice.setPixelColor(0, dim(color, 255 * (m == 0)));
    dice.setPixelColor(1, dim(color, 255 * (m == 1)));
    dice.setPixelColor(6, dim(color, 255 * (m == 2)));
    dice.setPixelColor(5, dim(color, 255 * (m == 3)));
    dice.show();
    delay(20);
  }
  Serial.println("Farbe ausgewaehlt");
  waitForSelect = 1;

  diceNumber(0);

  Serial.println("Computerspieler auswaehlen (mit Player tasten ein/aus)...");
  //Computerspieler
  uint32_t rerand = 0;
  boolean touched[players];
  autoPlay[0] = false;
  autoPlay[1] = false;
  autoPlay[2] = false;
  autoPlay[3] = false;
  while (true) {
    if (select()) {
      if (!waitForSelect) {
        if (selectCounter == 0) {
          selectCounter = millis();
        }
        else if (millis() - selectCounter > 300) {
          break;
        }
      }
    }
    else {
      selectCounter = 0;
      waitForSelect = 0;
    }
    //Zufaellige Pixel aufleuchten lassen
    if (rerand < millis()) {
      for (uint8_t i = 0; i < 7; i++) {
        dice.setPixelColor(i, dim(player_colors[random(players)], 255 * random(2)));
      }
      rerand = millis() + 125;
    }
    setDefault();
    uint8_t s = (millis() / 200) % 5;
    for (uint8_t player = 0; player < players; player++) {
      boolean v = getTouch(player);
      if (v != touched[player]) {
        touched[player] = v;
        if (v) {
          autoPlay[player] = !autoPlay[player];
          Serial.print("Spieler ");
          Serial.print(player + 1);
          Serial.print(" ist vom ");
          Serial.print(autoPlay[player] ? "Comuter" : "Menschen");
          Serial.println(" gesteuert");
        }
      }
      if (autoPlay[player]) {
        for (uint8_t si = 0; si < s; si++) {
          setPixel(player, FINISH1 + si, player_colors[player]);
        }
      }
    }
    show();
  }
  Serial.println("Computerspieler ausgewaehlt");
  
  Serial.println("resetGame() beendet");
}

void loop() {
  unsigned long delayTo;

  Serial.println("prüfen ob Spiel zuende ist...");
  //Gucken, ob Spiel bereits zu Ende
  boolean br;
  for (uint8_t player = 0; player < players; player++) {
    if (playerRanking[player] == 0) {
      br = false;
      for (uint8_t figure = 0; figure < FIGURES; figure++) {
        if (player_positions[player][figure] < 40) {
          br = true;
          break;
        }
      }
      if (!br) {
        playerRanking[player] = ++wonPlayers;
      }
    }
  }
  //Spiel zu Ende
  if (wonPlayers + 1 >= players) {
    Serial.println("Spiel zuende!");
    //blackout
    for (uint8_t px = 0; px < field.numPixels(); px++)
      field.setPixelColor(px, 0);
    for (uint8_t px = 0; px < finish.numPixels(); px++)
      finish.setPixelColor(px, 0);
    for (uint8_t px = 0; px < dice.numPixels(); px++)
      dice.setPixelColor(px, 0);
    //Platzierungen anzeigen
    Serial.println("Platzierung:");
    for (uint8_t player = 0; player < players; player++) {
      Serial.print("Spieler ");
      Serial.print(player +1);
      Serial.print(" = Platz ");
      Serial.println(playerRanking[player]);
      for (uint8_t px = 40; px < 40 + (playerRanking[player] == 0 ? players : playerRanking[player]); px++) {
        setPixel(player, px, player_colors[player]);
      }
    }
    show();
    //Neues Spiel
    Serial.println("zum neu starten select-drücken");
    while (!select());
    resetGame();
    return;
  }

  Serial.print("Spieler: ");
  Serial.print(activePlayer+1);
  Serial.println(" ist am Zug");
  diceNumber(0);
  while (true) {
    br = true;
    //Muss der Spieler noch ziehen?
    for (uint8_t figure = 0; figure < FIGURES; figure++) {
      if (player_positions[activePlayer][figure] < 40) {
        br = false;
        Serial.println("Spieler muss noch ziehen...");
        break;
      }
    }
    if (br) {
      Serial.println("Spieler muss nicht mehr ziehen, Zug beendet!");
      break;
    }
    uint8_t movement = 0;
    Serial.println("Würfeln (max 3xMal)");
    //Maximal 3 Mal würfeln zum rauskommen
    for (uint8_t rolledTimes = 0; rolledTimes < 3; rolledTimes++) {
      movement = rollDice(rolledTimes);
      //Nur, wenn keine Figur bewegbar ist (alle im Ziel oder Haus und keine 6), dann noch einmal versuchen
      if (nextFigure(movement) || nextFigure(1)) break;
      Serial.println("keine Figur bewegbar (alle im Ziel oder Haus und keine 6), noch einmal würfeln");
    }

    activeFigure = 0;
    //Wenn keine Figur mit der Augenzahl setzbar ist
    if (nextFigure(movement) == false) {
      delayTo = millis() + 500;
      //Blinken
      while (millis() < delayTo) {
        blink(activePlayer);
      }
      setDefault();
      show();
      Serial.println("es kann mit keiner Figur gesetzt werden, Zug beendet.");
      break;
    }
    previousFigure(movement); // Reset to next playable figure

    Serial.println("Figur zum setzen auswählen:");
    if (autoPlay[activePlayer]) {
      Serial.println("Computerspieler wählt Figur...");
      // Computerspieler
      boolean collides = false;
      //Zuerst versuchen, jemanden zu schlagen
      for (uint8_t i = 0; i < 4 && !collides; i++) {
        for (uint8_t f = 0; f < FIGURES; f++) {
          for (uint8_t p = 0; p < players; p++) {
            for (uint8_t pf = 0; pf < FIGURES; pf++) {
              collides |= collision(activePlayer, activeFigure, p, pf, movement);
              if (collides) {
                Serial.print("Computer kann mit Figur ");
                Serial.print(activeFigure +1);
                Serial.println(" schlagen");
                break;
              }
            }
            if (collides) break;
          }
          if (collides) break;
          nextFigure(movement);
        }
      }
      //Wenn das nicht geht, versuchen ins Ziel zu setzen
      for (uint8_t f = 0; f < FIGURES && !collides; f++) {
        if (player_positions[activePlayer][f] < FINISH1 && player_positions[activePlayer][f] + movement >= FINISH1) {
          collides = true; //Variable setzen, damit nicht zufaellig gewaehlt wird
          Serial.print("Computer kann mit Figur ");
          Serial.print(activeFigure +1);
          Serial.println(" ins Ziel gehen");
          break;
        }
        nextFigure(movement);
      }
      //Ansonsten einfach irgendeine Figur waehlen
      for (uint8_t f = 0; f < random(FIGURES) && !collides; f++) {
        nextFigure(movement);
      }
      if (!collides) {
        Serial.print("Computer wählt zufällig Figur ");
        Serial.println(activeFigure +1);
      }
    }
    else {
      Serial.print("Spieler ");
      Serial.print(activePlayer +1);
      Serial.println(" muss Figur auswählen (mit Player-Taste)");
      Serial.println("kurz drücken um zwischen den Figuren zu wechseln");
      Serial.println("500 ms drücken setzt die ausgewählten Figur");
      // Normaler Spieler
      boolean isTouched = false;
      unsigned long touchStart;
      while (true) {
        if (isTouched && touchStart + 500 < millis()) {
          // 500ms druecken setzt den ausgewaehlten Spieler
          Serial.print("Figur ");
          Serial.print(activeFigure +1);
          Serial.println(" bestätigt!");
          break;
        }
        if (getTouch(activePlayer) != isTouched) {
          isTouched = getTouch(activePlayer);
          if (isTouched) {
            touchStart = millis();
          }
          else {
            //losgelassen -> naechste Figur
            nextFigure(movement);
            Serial.print("soll Figur ");
            Serial.print(activeFigure +1);
            Serial.println(" ausgewählt werden?");
          }
        }
        animateFigure(activePlayer, activeFigure, movement);
      }
    }
    //Figur bewegen
    Serial.println("Figur wird bewegt...");
    moveFigure(activePlayer, activeFigure, movement);
    if (movement != 6) {
      break;
    }
    //Busy wait
    Serial.println("Player Taste betätigen um Zug zu beenden");
    while (getTouch(activePlayer) && !autoPlay[activePlayer]);
  }
  activePlayer = (activePlayer + players - 1) % players;
}

void blink(uint8_t player) {
  uint8_t f = (millis() / 100) % 2;
  setDefault();
  setPlayerBrightness(player, f * 255);
  show();
}

void fade(uint8_t player) {
  int8_t f = 127 - (millis() / 3) % 256;
  setDefault();
  setPlayerBrightness(player, abs(f) + 127);
  show();
}

void setPlayerBrightness(uint8_t player, uint8_t brightness) {
  uint32_t color = dim(player_colors[player], brightness);
  for (uint8_t figure = 0; figure < FIGURES; figure++) {
    setPixel(player, player_positions[player][figure], color);
  }
}

void setOtherPlayersBrightness(uint8_t player, uint8_t brightness) {
  for (uint8_t p = 0; p < players; p++) {
    if (p == player) continue;
    uint32_t color = dim(player_colors[p], brightness);
    for (uint8_t figure = 0; figure < FIGURES; figure++) {
      setPixel(p, player_positions[p][figure], color);
    }
  }
}

//Figuren alle auf ihre Position setzen
void setDefault(void) {
  //Serial.println("setDefault()");
  for (uint8_t px = 0; px < field.numPixels(); px++) {
    field.setPixelColor(px, 0);
  }
  for (uint8_t px = 0; px < finish.numPixels(); px++) {
    finish.setPixelColor(px, 0);
  }
  for (uint8_t player = 0; player < players; player++) {
    for (uint8_t figure = 0; figure < FIGURES; figure++) {
      setPixel(player, player_positions[player][figure], player_colors[player]);
    }
  }
}

//Shortcut zum Anzeigen aller Daten
void show(void) {
  finish.show();
  field.show();
  dice.show();
}

boolean getTouch(uint8_t player) {
  uint8_t p = player;
  if (players == 2 && player == 1) p++;
  switch (p) {
#ifdef VERSION2
  case 0:
    return digitalRead(BUTTON_P1);
  case 1:
    return digitalRead(BUTTON_P2);
  case 2:
    return digitalRead(BUTTON_P3);
  case 3:
    return digitalRead(BUTTON_P4);
#else
  case 0:
    return !digitalRead(BUTTON_P1);
  case 1:
    return !digitalRead(BUTTON_P2);
  case 2:
    return !digitalRead(BUTTON_P3);
  case 3:
    return !digitalRead(BUTTON_P4);
#endif
  }
  return 0;
}

uint8_t rollDice(void) {
  return rollDice(false);
}
uint8_t rollDice(boolean again) {
  if (!again)
    diceNumber(0);
  uint8_t number = random(6);
  uint8_t i = 0;
  if (autoPlay[activePlayer]) {
    Serial.print("Computerspieler-");
    Serial.print(activePlayer +1);
    Serial.println(", schnelles Würfeln");
    for (char c = 0; c < 100 + number; c++) {
      i++;
      i %= 6;
      diceNumber(i + 1);
      delay(1); //Schnelle Animation
    }
  }
  else {
    Serial.print("Spieler ");
    Serial.print(activePlayer +1);
    Serial.println(" muss Würfeln (Player Taste gedrückt halten)");
    while (!getTouch(activePlayer)) {
      fade(activePlayer);
      if (!again)
        animateDice();
    }
    setDefault();
    show();
    while (getTouch(activePlayer)) {
      i++;
      i %= 6;
      diceNumber(i + 1);
      delay(1); //Schnelle Animation
    }
  }
  for (char c = 0; c < 11; c++) {
    diceNumber(i % 6 + 1);
    delay(c);
    i++;
  }
  for (char c = 1; c < 8; c++) {
    diceNumber(i % 6 + 1);
    delay(10 * c);
    i++;
  }
  for (char c = 1; c < 9; c++) {
    diceNumber(i % 6 + 1);
    delay(80 * c);
    i++;
  }
  diceNumber(i % 6 + 1);
  if (autoPlay[activePlayer]) {
    delay(700);
  }
  Serial.print("geworfene Augenzahl: ");  
  Serial.println(i % 6 + 1);  
  return i % 6 + 1;
}

//Wuerfelaugen anzeigen (volle Helligkeit)
void diceNumber(uint8_t number) {
  diceNumber(number, 255);
}
void diceNumber(uint8_t number, uint8_t brightness) {
  dice.setPixelColor(0, dim(player_colors[activePlayer], brightness * (number & 4)));
  dice.setPixelColor(6, dim(player_colors[activePlayer], brightness * (number & 4)));
  dice.setPixelColor(2, dim(player_colors[activePlayer], brightness * (number == 6)));
  dice.setPixelColor(4, dim(player_colors[activePlayer], brightness * (number == 6)));
  dice.setPixelColor(3, dim(player_colors[activePlayer], brightness * (number & 1)));
  dice.setPixelColor(1, dim(player_colors[activePlayer], brightness * ((number & 2) | (number & 4))));
  dice.setPixelColor(5, dim(player_colors[activePlayer], brightness * ((number & 2) | (number & 4))));
  dice.show();
}
//Wuerfel drehen lassen
void animateDice(void) {
  uint8_t m = (millis() / 80) % 4;
  dice.setPixelColor(0, dim(player_colors[activePlayer], 255 * (m == 0)));
  dice.setPixelColor(1, dim(player_colors[activePlayer], 255 * (m == 1)));
  dice.setPixelColor(6, dim(player_colors[activePlayer], 255 * (m == 2)));
  dice.setPixelColor(5, dim(player_colors[activePlayer], 255 * (m == 3)));
  dice.show();
}

boolean select(void) {
  return !digitalRead(SELECT_BUTTON);
}

void animateFigure(uint8_t player, uint8_t figure, uint8_t movement) {
  uint8_t step = (millis() / 250) % (movement + 2);
  int8_t destination = player_positions[player][figure] + step - (step == movement + 1);
  if (player_positions[player][figure] < 0) {
    destination = 0;
    step = (millis() / 250) % 3;
  }
  setDefault();
  if (step > 0) {
    setPixel(player, destination, dim(player_colors[player], 127)); //override color
  }
  show();
}

void moveFigure(uint8_t player, uint8_t figure, uint8_t movement) {
  int8_t destination = player_positions[player][figure] + movement;
  uint8_t remainingMoves = movement;
  if (player_positions[player][figure] < 0) {
    destination = 0;
    remainingMoves = 1;
  }
  while (player_positions[player][figure] < destination) {
    if (player_positions[player][figure] < 0) {
      player_positions[player][figure] = 0;
    }
    else {
      player_positions[player][figure]++;
    }

    setDefault();
    setPixel(player, player_positions[player][figure], player_colors[player]); //override color
    diceNumber(--remainingMoves);
    show();
    delay(250);
  }
  diceNumber(0);
  for (uint8_t p = 0; p < players; p++) {
    if (p == player) continue;
    for (uint8_t f = 0; f < FIGURES; f++) {
      if (collision(player, figure, p, f, 0)) {
        goHome(p, f);
        return;
      }
    }
  }
  setDefault();
  show();
}

void goHome(uint8_t player, uint8_t figure) {
  uint8_t homed = homeFigures(player) + 1;
  while (player_positions[player][figure] >= 0) {
    player_positions[player][figure]--;
    if (player_positions[player][figure] < 0) player_positions[player][figure] = -homed; //skip homed
    delay(50);
    setDefault();
    setPixel(player, player_positions[player][figure], player_colors[player]); //override color
    show();
  }
}

uint8_t homeFigures(uint8_t player) {
  uint8_t figures = 0;
  for (uint8_t i = 0; i < FIGURES; i++) {
    figures += player_positions[player][i] < 0;
  }
  return figures;
}

void setPixel(uint8_t player, int8_t position, uint32_t color) {
  if (position < 40) {
    field.setPixelColor(getPixel(player, position), color);
  }
  else {
    if (players == 2 && player == 1) player++;
    switch (player) {
    case 0:
      finish.setPixelColor(((43 - position) + FIGURES * 0) & 0b1111, color);
      break;
    case 1:
      finish.setPixelColor(((43 - position) + FIGURES * 3) & 0b1111, color);
      break;
    case 2:
      finish.setPixelColor(((43 - position) + FIGURES * 2) & 0b1111, color);
      break;
    case 3:
      finish.setPixelColor(((43 - position) + FIGURES * 1) & 0b1111, color);
      break;
    }
  }
}

uint8_t getPixel(uint8_t player, int8_t position) {
  if (players == 2 && player == 1) player++;
  uint8_t pixel = 74;
  player = (player + 2) & 0b11;
  pixel -= position;
  pixel += field.numPixels() / MAXPLAYERS * player; //Turn field by 90deg depending on player
  if (position >= 0)
    pixel -= position / 10 * FIGURES;       //compensate home fields
  return pixel % field.numPixels();
}

uint32_t dim(uint32_t color, uint8_t factor) {
  uint32_t r, g, b;
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
  r = (r * factor) >> 8;
  g = (g * factor) >> 8;
  b = (b * factor) >> 8;
  color = (r << 16) + (g << 8) + b;
  return color;
}

//Naechste bewegbare Figur waehlen, false falls es keine gibt
boolean nextFigure(uint8_t movement) {
  uint8_t figure = activeFigure;
  for (uint8_t i = 1; i <= FIGURES; i++) {
    figure = (activeFigure + i) % FIGURES;
    if (isMoveable(activePlayer, figure, movement)) {
      activeFigure = figure;
      return true;
    }
  }
  return false;
}
//Und vorherige
boolean previousFigure(uint8_t movement) {
  for (int8_t i = 3; i >= 0; i--) {
    if (isMoveable(activePlayer, (activeFigure + i) % FIGURES, movement)) {
      activeFigure = (activeFigure + i) % FIGURES;
      return true;
    }
  }
  return false;
}

//Hier passiert die mathematische Magie...
boolean isMoveable(uint8_t player, uint8_t figure, uint8_t movement) {
  boolean startBlocked = false;
  uint8_t homed = homeFigures(player);
  for (uint8_t i = 0; i < FIGURES; i++) {
    if (player_positions[player][i] == 0) {
      startBlocked = true;
      if (homed > 0 && i != figure && isMoveable(player, i, movement)) {
        //Startposition muss geraeumt werden
        return false;
      }
    }
  }
  if (movement == 6 && !startBlocked && homed != 0) {
    //Haus muss leer werden, sofern die Startposition nicht versperrt ist
    return player_positions[player][figure] == -homed;
  }
  if ((movement != 6 || startBlocked) && player_positions[player][figure] < 0) {
    //Startoisition versperrt oder keine 6
    return false;
  }
  if (player_positions[player][figure] + movement > FINISH4) {
    //Nicht ueber das Ziel hinausschiessen!
    return false;
  }
  for (uint8_t i = 0; i < FIGURES; i++) {
    //Der Weg wird von einer eigenen Figur blockiert
    if (player_positions[player][figure] + movement == player_positions[player][i]) {
      return false;
    }
  }
  for (uint8_t i = 0; i < FIGURES; i++) {
    if (i == figure) continue;
    //Im Ziel nicht ueberspringen, auskommentieren, wenn man es doch duerfen soll
    if (player_positions[player][figure] + movement >= FINISH1 && player_positions[player][i] >= FINISH1 && player_positions[player][figure] < player_positions[player][i] && player_positions[player][figure] + movement > player_positions[player][i]) {
      return false;
    }
  }
  return true;
}

boolean collision(uint8_t player1, uint8_t figure1, uint8_t player2, uint8_t figure2, uint8_t movement) {
  //Simple Kollisionserkennung zwischen P1F1+Wuerfelaugen und P2F2
  if (player_positions[player1][figure1] + movement >= FINISH1
    || player_positions[player2][figure2] >= FINISH1
    || player_positions[player1][figure1] <= HOME1
    || player_positions[player2][figure2] <= HOME1) {
    return false;
  }
  return getPixel(player1, player_positions[player1][figure1] + movement) == getPixel(player2, player_positions[player2][figure2]);
}

uint32_t Wheel(byte WheelPos) {
  //HSB Hue zu RGB
  if (WheelPos < 85) {
    return field.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170) {
    WheelPos -= 85;
    return field.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else {
    WheelPos -= 170;
    return field.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
