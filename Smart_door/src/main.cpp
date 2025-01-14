#include <Arduino.h>
#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define RX_PIN 16
#define TX_PIN 17
#define SDA_PIN 19
#define SCL_PIN 18
#define ROWS_PIN 4
#define COLS_PIN 3
#define SERVO_PIN 21

char keys[ROWS_PIN][COLS_PIN] = {
    {'3', '2', '1'},
    {'6', '5', '4'},
    {'9', '8', '7'},
    {'#', '0', '*'}};

HardwareSerial fingerSerial(2);

byte rowPins[ROWS_PIN] = {32, 33, 25, 26};
byte colPins[COLS_PIN] = {27, 14, 12};

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS_PIN, COLS_PIN);

uint8_t id;
bool fingerprintFound = false;
int incorrectAttempts = 0;
const int MAX_INCORRECT_ATTEMPTS = 3;
int pos = 0;
bool statusDoor = false;

const char *ssid = "IPhonexx";
const char *password = "051220022222";
#define BOTtoken "7111399237:AAHQBGfVgE93d3WF4wKgTEGkfNw0qcfbdx4"
#define CHAT_ID "1619252676"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
Servo myservo;

void startFingerprint();
void checkFingerprintAttempts();
uint8_t readnumber(void);
uint8_t getFingerprintEnroll();
uint8_t deleteFingerprint(uint8_t id);
uint8_t getFingerprintID();
int getFingerprintIDez();
void opendoor();

void setup()
{
  Serial.begin(9600);
  // setup wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // setup servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // standard 50 hz servo
  myservo.attach(SERVO_PIN, 1000, 2000);
  myservo.write(0);
  // setup lcd
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Unlock");
  // setup vân tay
  startFingerprint();
  // setup bot tele
  bot.sendMessage(CHAT_ID, "Smart Unlock");
}

void loop()
{
  char passcode[5];
  // char passMaster[5];
  int index = 0;
  // int indexMaster = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhap mat khau:");
  while (index < 4)
  {
    char key = keypad.getKey();

    if (key != NO_KEY)
    {
      if (key == '#' && fingerprintFound == false)
      {
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Hay an");
        lcd.setCursor(4, 1);
        lcd.print("van tay!");
        unsigned long startTime = 0;
        startTime = millis();
        while (millis() - startTime < 5000)
        {
          getFingerprintID();
          if (finger.fingerSearch() == FINGERPRINT_OK)
          {
            fingerprintFound = true;
            // Serial.println("Đã đến phần if");
            // lcd.clear();
            // lcd.setCursor(0, 0);
            // lcd.print("Match fingerprint");
            // lcd.setCursor(2, 1);
            // lcd.print("Unlock!");
            break;
          }
        }
        checkFingerprintAttempts();
        return;
      }
      passcode[index++] = key;
      lcd.setCursor(index, 1);
      lcd.print('*');
    }
  }

  if (fingerprintFound == true)
  {
    statusDoor = true;
  }
  else
  {
    incorrectAttempts++;
    checkFingerprintAttempts();
  }
  passcode[4] = '\0';

  if (strcmp(passcode, "1234") == 0)
  {
    statusDoor = true;
    // mở cửa
  }
  else if (strcmp(passcode, "0000") == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Them van tay!");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    Serial.println("Ready to enroll a fingerprint!");
    Serial.println("Please type in the ID # (from 1 to 9) you want to save this finger as...");
    id = readnumber();
    if (id == 0)
    {
      return;
    }
    Serial.print("Enrolling ID #");
    Serial.println(id);

    while (!getFingerprintEnroll())
      ;
  }
  else if (strcmp(passcode, "1111") == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Xoa van tay!");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    Serial.println("Please type in the ID # (from 1 to 9) you want to delete...");
    uint8_t id = readnumber();
    if (id == 0)
    {
      return;
    }

    Serial.print("Deleting ID #");
    Serial.println(id);

    deleteFingerprint(id);
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mat khau sai");
    statusDoor = false;
  }
  // else
  // {
  //   statusDoor = false;
  // }
  if (statusDoor == true)
  {
    // while (indexMaster < 4)
    // {
    //   char keyMaster = keypad.getKey();
    //   if (keyMaster != NO_KEY)
    //   {
    //     passMaster[indexMaster++] = keyMaster;
    //     lcd.setCursor(indexMaster, 1);
    //     lcd.print('*');
    //   }
    // }
    unsigned long startOpenTime = millis();
    opendoor();
    if (millis() - startOpenTime >= 5000)
    {
      // if (strcmp(passcode, "0000") == 0)
      // {
      //   lcd.clear();
      //   lcd.setCursor(0, 0);
      //   lcd.print("Add fingerprint!");
      //   lcd.setCursor(0, 1);
      //   lcd.print("ID: ");
      //   Serial.println("Ready to enroll a fingerprint!");
      //   Serial.println("Please type in the ID # (from 1 to 9) you want to save this finger as...");
      //   id = readnumber();
      //   if (id == 0)
      //   {
      //     return;
      //   }
      //   Serial.print("Enrolling ID #");
      //   Serial.println(id);

      //   while (!getFingerprintEnroll())
      //     ;
      // }
      // else if (strcmp(passcode, "1111") == 0)
      // {
      //   lcd.clear();
      //   lcd.setCursor(0, 0);
      //   lcd.print("Delete fingerprint!");
      //   lcd.setCursor(0, 1);
      //   lcd.print("ID: ");
      //   Serial.println("Please type in the ID # (from 1 to 9) you want to delete...");
      //   uint8_t id = readnumber();
      //   if (id == 0)
      //   {
      //     return;
      //   }

      //   Serial.print("Deleting ID #");
      //   Serial.println(id);

      //   deleteFingerprint(id);
      // }
      // else
      // {
      //   lcd.clear();
      //   lcd.setCursor(0, 0);
      //   lcd.print("Invalid passcode");
      // }
    }
  }
  delay(2000);
  lcd.clear();
  lcd.print("Nhap mat khau:");
}

void checkFingerprintAttempts()
{
  if (incorrectAttempts >= MAX_INCORRECT_ATTEMPTS)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Canh bao nhap");
    lcd.setCursor(0, 1);
    lcd.print("mat khau sai");
    bot.sendMessage(CHAT_ID, "Cảnh báo! Có đối tượng đang cố gắng đột nhập");
    delay(2000);
    lcd.clear();
    lcd.print("Nhap mat khau:");
    incorrectAttempts = 0;
  }
}

void startFingerprint()
{
  while (!Serial)
    ;
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
    {
      delay(1);
    }
  }
}
uint8_t readnumber(void)
{
  char key;
  uint8_t num = 0;

  while (num == 0)
  {
    while (true)
    {
      key = keypad.getKey();
      if (key != NO_KEY)
      {
        if (isdigit(key))
        {
          num = num * 10 + (key - '0');
          lcd.setCursor(5, 1);
          lcd.print(num);
          break;
        }
      }
    }
  }

  return num;
}
uint8_t getFingerprintEnroll()
{
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tha van tay");
  lcd.setCursor(0, 1);
  lcd.print("va an lai");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Thu lai");
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Prints matched!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Thanh cong");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_ENROLLMISMATCH)
  {
    Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ngon tay khong khop");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Stored!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Thanh cong");
    delay(100);
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not store in that location");
    return p;
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}
uint8_t deleteFingerprint(uint8_t id)
{
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK)
  {
    Serial.println("Deleted!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not delete in that location");
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
  }
  else
  {
    Serial.print("Unknown error: 0x");
    Serial.println(p, HEX);
  }

  return p;
}
uint8_t getFingerprintID()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image taken");
    break;
  case FINGERPRINT_NOFINGER:
    Serial.println("No finger detected");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_IMAGEFAIL:
    Serial.println("Imaging error");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Found a print match!");
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Dung van tay");
    lcd.setCursor(4, 1);
    lcd.print("Mo khoa!");
    for (pos = 0; pos <= 180; pos += 1)
    {
      myservo.write(pos);
      delay(15);
    }
    delay(5000);
    // unsigned long startOpenDoor = millis();
    // if (millis() - startOpenDoor >= 5000)
    // {
    for (pos = 180; pos >= 0; pos -= 1)
    {
      myservo.write(pos);
      delay(15);
    }
    statusDoor = false;
    // }
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_NOTFOUND)
  {
    Serial.println("Did not find a match");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  // return finger.fingerID;
  return finger.fingerSearch();
  // return statusDoor;
}
int getFingerprintIDez()
{
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)
    return -1;

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}
void opendoor()
{
  for (pos = 0; pos <= 180; pos += 1)
  {
    myservo.write(pos);
    delay(15);
  }
  delay(5000);
  // unsigned long startOpenDoor = millis();
  // if (millis() - startOpenDoor >= 5000)
  // {
  for (pos = 180; pos >= 0; pos -= 1)
  {
    myservo.write(pos);
    delay(15);
  }
  statusDoor = false;
  // }
}