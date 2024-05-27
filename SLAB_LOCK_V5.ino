#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN 8

float duration_us, distance_cm;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key mkey;
char d, d1, d2, d3, d10, d11, d12, d13;
int c, cpa, tag, uid_index;
bool cp, np, ok, m;
// Init array that will store new NUID
byte nuidPICC[4];

//-------keypad setting-----------------
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {14, 15, 16, 17};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  
  lcd.init();
  lcd.backlight();
  lcd.print("SCAN YOUR ID TAG");
  lcd.setCursor(0, 1);

  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    mkey.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scans the MIFARE Classic NUID."));

  // Set default password
  if (EEPROM.read(0) > 1 || EEPROM.read(0) < 1) {
    EEPROM.update(0, 1);
    EEPROM.update(1, '1');
    EEPROM.update(2, '2');
    EEPROM.update(3, '3');
    EEPROM.update(4, '4');
  }
  d = EEPROM.read(1);
  d1 = EEPROM.read(2);
  d2 = EEPROM.read(3);
  d3 = EEPROM.read(4);
}

void loop() {

  //--------------------------keypad sec code----------------------
  char k = keypad.getKey();

  if (k) {
    c++;
    Serial.println(k);

    lcd.print("*");
    if (c == 1) {
      d10 = k;
    }
    if (c == 2) {
      d11 = k;
    }
    if (c == 3) {
      d12 = k;
    }
    if (c == 4) {
      d13 = k;
    }
  }
  if (k == '#') {
    m = c = cp = np = tag = 0;
    lcd.clear();
    lcd.print("SCAN YOUR ID TAG");
  }

  if (c == 4 && np == 0) {
    c = 0;
    if (d10 == '*' && d11 == '0' && d12 == '0') {
      m = 1;
      lcd.clear();
      lcd.print("ENTER PASSWORD   ");
      switch (d13) {
        case '0':
          cp = 1;
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
          tag = d13 - '0';
          break;
      }
      d10 = d11 = d12 = d13 = 0;
    }
    if (d == d10 && d1 == d11 && d2 == d12 && d3 == d13) {
      if (cp == 0 && m == 0) {
        lcd.setCursor(0, 1);
        lcd.print("PASSWORD ACCEPTED");
        digitalWrite(RELAY_PIN, HIGH);
        delay(3000);
        digitalWrite(RELAY_PIN, LOW);
        delay(6000);
        d10 = d11 = d12 = d13 = 0;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
      } else if (cp == 1 && tag == 0) {
        lcd.clear();
        lcd.print("NEW PASSWORD");
        lcd.setCursor(0, 1);
        np = 1;
      } else if (tag > 0) {
        lcd.clear();
        lcd.print("SCAN ID TAG # ");
        lcd.print(tag);
        lcd.setCursor(0, 1);
      }
      m = 0;
    } else {
      if (!m) {
        lcd.setCursor(0, 1);
        lcd.print("WRONG PASSWORD");
        delay(3000);
      }
      d10 = d11 = d12 = d13 = 0;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
    }
  }

  if (c == 4 && np == 1) {
    m = 0;
    d = d10;
    d1 = d11;
    d2 = d12;
    d3 = d13;
    EEPROM.update(1, d);
    EEPROM.update(2, d1);
    EEPROM.update(3, d2);
    EEPROM.update(4, d3);
    np = 0;
    cp = 0;
    c = 0;
    d10 = d11 = d12 = d13 = 0;
    lcd.clear();
    lcd.print("SCAN YOUR ID TAG");
    lcd.setCursor(0, 1);
  }

  //---rfid----------------------
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial())
    return;

  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  if (tag > 0) {
    uid_index = (tag - 1) * 4;
    EEPROM.update(5 + uid_index, rfid.uid.uidByte[0]);
    EEPROM.update(6 + uid_index, rfid.uid.uidByte[1]);
    EEPROM.update(7 + uid_index, rfid.uid.uidByte[2]);
    EEPROM.update(8 + uid_index, rfid.uid.uidByte[3]);
    tag = 0;
    d10 = d11 = d12 = d13 = 0;
    lcd.setCursor(0, 1);
    lcd.print("  TAG ID SAVED  ");
    delay(2000);
    lcd.clear();
    lcd.print("SCAN YOUR ID TAG");
    cp = 0;
    c = 0;
  } else {
    ok = 0;
    for (int i = 0; i < 30; i++) {
      uid_index = i * 4;
      if (rfid.uid.uidByte[0] == EEPROM.read(5 + uid_index) &&
          rfid.uid.uidByte[1] == EEPROM.read(6 + uid_index) &&
          rfid.uid.uidByte[2] == EEPROM.read(7 + uid_index) &&
          rfid.uid.uidByte[3] == EEPROM.read(8 + uid_index)) {
        ok = 1;
        break;
      }
    }

    if (ok == 1) {
      Serial.println(F("A new card has been detected."));
      lcd.setCursor(0, 1);
      lcd.print("ID TAG ACCEPTED ");
      digitalWrite(RELAY_PIN, HIGH);
      delay(3000);
      digitalWrite(RELAY_PIN, LOW);
      delay(6000);
      lcd.setCursor(0, 1);
      lcd.print("                ");
    } else {
      lcd.setCursor(0, 1);
      lcd.print(" ACCESS DENIED ");
      delay(2000);
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  // Handle serial input for EEPROM commands
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'r') {
      readEEPROM();
    } else if (command == 's') {
      setEEPROM();
    } else if (command == 'h') {
      sendHistory();
    } else if (command == 'd') {
      deleteEEPROM();
    }
  }
}

void readEEPROM() {
  Serial.print("Password: ");
  Serial.print((char)EEPROM.read(1));
  Serial.print((char)EEPROM.read(2));
  Serial.print((char)EEPROM.read(3));
  Serial.print((char)EEPROM.read(4));
  Serial.println();
  Serial.println("Stored UID numbers: ");
  for (int i = 0; i < 30; i++) {
    int address = 5 + i * 4;
    Serial.print(EEPROM.read(address));
    Serial.print(EEPROM.read(address + 1));
    Serial.print(EEPROM.read(address + 2));
    Serial.println(EEPROM.read(address + 3));
  }
}

void setEEPROM() {
  Serial.println("Enter new password (4 characters): ");
  while (Serial.available() < 4);
  char newPassword[4];
  for (int i = 0; i < 4; i++) {
    newPassword[i] = Serial.read();
  }
  EEPROM.update(1, newPassword[0]);
  EEPROM.update(2, newPassword[1]);
  EEPROM.update(3, newPassword[2]);
  EEPROM.update(4, newPassword[3]);
  Serial.print("New password set to: ");
  Serial.print(newPassword[0]);
  Serial.print(newPassword[1]);
  Serial.print(newPassword[2]);
  Serial.print(newPassword[3]);
  Serial.println();
}

void sendHistory() {
  Serial.println("Stored UID numbers: ");
  for (int i = 0; i < 30; i++) {
    int address = 5 + i * 4;
    Serial.print("UID ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(EEPROM.read(address));
    Serial.print(" ");
    Serial.print(EEPROM.read(address + 1));
    Serial.print(" ");
    Serial.print(EEPROM.read(address + 2));
    Serial.print(" ");
    Serial.println(EEPROM.read(address + 3));
  }
}

void deleteEEPROM() {
  Serial.println("Enter index to delete (1-30): ");
  while (!Serial.available());
  int index = Serial.parseInt();
  if (index >= 1 && index <= 30) {
    int address = 5 + (index - 1) * 4;
    for (int i = 0; i < 4; i++) {
      EEPROM.update(address + i, 0);
    }
    Serial.print("Deleted UID at index ");
    Serial.println(index);
  } else {
    Serial.println("Invalid index");
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
