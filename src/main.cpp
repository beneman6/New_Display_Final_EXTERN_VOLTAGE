#include <Arduino.h>
#include <SmallFont_20.h>
#include <LargeFont20.h>
#include <TFT_eSPI.h>
#include <PNGdec.h>
#include <Metadata_large.h>
#include <FT6336U.h>
#include <Menue.h>
#include <SmallFont26.h>
#include <Bluetooth.h>
#include <pause.h>
#include <play.h>
#include <INA219_WE.h>

enum menueEbene
{
  hauptmenue,
  metadata,
  settings,
  steuerung,
  werte
};

volatile enum menueEbene aktuelleEbene = metadata;
enum menueEbene zwischnEbene;
enum menueEbene zwischnEbeneTwo;
enum menueEbene zwischnEbeneThree;
enum menueEbene zwischnEbeneFour;
enum menueEbene zwischnEbeneFive;
enum menueEbene zwischenEbeneSix;

boolean neueEingabe = false;
xTaskHandle metadataTaskHandle;
void metadataTask(void *parameter);
xTaskHandle menueTaskHandle;
void menueTask(void *parameter);
void scrollByOne(TFT_eSprite &sprite, char *text, uint8_t xPos, uint8_t yPos, uint16_t &scrollPos, uint16_t textLength, uint8_t type);
void drawTextToSprite(TFT_eSprite &sprite, char *text, uint8_t xPos, uint8_t yPos);
void serialRecieved();
void touchTask(void *parameter);
void timeTask(void *parameter);
void volumeTask(void *parameter);
void volumeUpdateTask(void *parameter);
void voltageTask(void *parameter);

xTaskHandle touchTaskHandle;
xTaskHandle timeTaskHandle;
xTaskHandle volumeTaskHandle;
xTaskHandle volumeUpdateTaskHandle;
xTaskHandle voltageTaskHandle;

void drawButton(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const char *text, uint16_t color, const uint8_t font[]);

char *title = new char[512];
char *album = new char[512];
char *artist = new char[512];
char *lastTitle = new char[512];
volatile boolean neueDaten = true;
boolean titleToLong = false;
boolean albumToLong = false;
boolean artistToLong = false;
uint16_t titlePos = 0;
uint16_t artistPos = 0;
uint16_t albumPos = 0;
uint16_t titleLength = 0;
uint16_t artistLength = 0;
uint16_t albumLength = 0;
uint16_t waitForTitle = 0, waitForArtist = 0, waitForAlbum = 0;

volatile boolean nebelmaschiene = false, led = false, movingHeads = false;

SemaphoreHandle_t displayMutex;
SemaphoreHandle_t mediaStateMutex;
SemaphoreHandle_t ebeneMutex;
SemaphoreHandle_t serialMutex;
SemaphoreHandle_t datenMutex;
SemaphoreHandle_t WIREMutex;

uint8_t i = 0;
uint16_t touchValue[5] = {400, 3507, 337, 3458, 1};

// Serial Objekte
HardwareSerial SerialPort(2);
void serialCallback();
void recieveEvent(int bytes);
char *buffer = new char[512];
char *inhalt = new char[512];
char *zeitUF = new char[512];
char *bufferTwo = new char[512];
char *bufferThree = new char[512];
char *bufferFour = new char[512];
char *bufferFive = new char[512];
char del[1];
char *ptr;
char *ptrTwo;
uint32_t zeitAktuell, zeitGesamt;
uint8_t minuteAktuell, minuteGesamt, sekundeAktuell, sekundeGesamt;
volatile uint8_t mediaState = 0;
volatile uint8_t a2dpState = 1;
volatile uint8_t spkvol = 0;
volatile uint8_t mediaStateBuffer = 0;

boolean gefunden = false;
boolean neueZeit = false;
uint8_t counter = 0;

#define UART_RX_PIN 25
#define UART_TX_PIN 26

#define VOLUME_UP_X_POS 420
#define VOLUME_UP_Y_POS 98
#define VOLUME_UP_WIDTH 30
#define VOLUME_UP_HEIGHT 30

#define VOLUME_DOWN_X_POS 420
#define VOLUME_DOWN_Y_POS 233
#define VOLUME_DOWN_WIDTH 33
#define VOLUME_DOWN_HEIGHT 33

#define BLUETOOTH_HEIGHT 44
#define BLUETOOTH_WIDTH 30
#define BLUETOOTH_X_POS 335
#define BLUETOOTH_Y_POS 15

#define TIME_POS_X 350
#define TIME_POS_Y 275
#define TIME_SPRITE_WIDTH 130
#define TIME_SPRITE_HEIGHT 45

// Width of the Metadata Window
#define METADATA_WINDOW_WIDTH 340
// Top left position of the Metadata Window
#define METADATA_X_POS 82
#define METADATA_Y_START_POS 73
#define METADATA_DISTANCE 55

// Metadata Background Color
#define METADATA_BACKGROUND_COLOR 0x31A6
// Metadata Button Color
#define METADATA_BUTTON_COLOR TFT_BLUE

#define METADATA_SPRITE_HEIGHT 45
#define METADATA_SPRITE_WIDTH 1200

// Settings Icon
#define BUTTON_ONE_X_POS 400
#define BUTTON_ONE_Y_POS 5
#define BUTTON_ONE_WIDTH 75
#define BUTTON_ONE_HEIGHT 75

// Song back Button
#define BUTTON_TWO_X_POS 128
#define BUTTON_TWO_Y_POS 257
#define BUTTON_TWO_WIDTH 60
#define BUTTON_TWO_HEIGHT 60
// Play Pause Button
#define BUTTON_THREE_X_POS 211
#define BUTTON_THREE_Y_POS 258
#define BUTTON_THREE_WIDTH 58
#define BUTTON_THREE_HEIGHT 58
// Song forward Button
#define BUTTON_FOUR_X_POS 303
#define BUTTON_FOUR_Y_POS 257
#define BUTTON_FOUR_WIDTH 60
#define BUTTON_FOUR_HEIGHT 60

// Menue ONE BUTTON
#define MENUE_ONE_X_POS 355
#define MENUE_ONE_Y_POS 70
#define MENUE_ONE_WIDTH 80
#define MENUE_ONE_HEIGHT 30

// Menue TWO BUTTON
#define MENUE_TWO_X_POS 355
#define MENUE_TWO_Y_POS 115
#define MENUE_TWO_WIDTH 80
#define MENUE_TWO_HEIGHT 30

// Menue THREE BUTTON
#define MENUE_THREE_X_POS 355
#define MENUE_THREE_Y_POS 160
#define MENUE_THREE_WIDTH 80
#define MENUE_THREE_HEIGHT 30

#define MAX_IMAGE_WIDTH 480

// Bildschirm Objekte

PNG png;
// PNG menuePNG;
TFT_eSPI mainScreen;
TFT_eSprite albumTextSprite(&mainScreen);
TFT_eSprite artistTextSprite(&mainScreen);
TFT_eSprite titleTextSprite(&mainScreen);
TFT_eSprite timeSprite(&mainScreen);
TFT_eSprite bluetoothSprite(&mainScreen);
TFT_eSprite playButtonSprite(&mainScreen);
TFT_eSprite timeBarSprite(&mainScreen);
TFT_eSprite volumeBarSprite(&mainScreen);
TFT_eSprite voltageSprite(&mainScreen);

void pngDrawCallback(PNGDRAW *pDraw);

// Touch Objekte

#define I2C_SDA 18
#define I2C_SCL 19
#define RST_N_PIN 16
#define INT_N_PIN 34

FT6336U ft6336u(I2C_SDA, I2C_SCL, NULL, NULL);
uint16_t touchPosX = 0, touchPosY = 0;
uint16_t prevTouchPosX = 1, prevTouchPosY = 1;

// Spannungsüberwachung

INA219_WE ina219 = INA219_WE(0x40);
float_t busVoltage_V = 0.0;
char *outputVoltage = new char[128];

void setup()
{

  // Touch Initialiserung

  ft6336u.begin();

  prevTouchPosX = (ft6336u.read_touch1_y());
  prevTouchPosY = (320 - ft6336u.read_touch1_x());

  // Bildschirm Initialisierung

  mainScreen.begin();
  mainScreen.setRotation(1);
  // mainScreen.fillScreen(0x31A6);
  mainScreen.fillScreen(TFT_BLACK);
  // menuePNG.openFLASH((uint8_t *)Menue,sizeof(Menue),pngDrawCallback);
  png.openFLASH((uint8_t *)Metadata_large, sizeof(Metadata_large), pngDrawCallback);

  mainScreen.startWrite();

  png.decode(NULL, 0);

  mainScreen.endWrite();

  delay(500);

  artistTextSprite.createSprite(METADATA_SPRITE_WIDTH, METADATA_SPRITE_HEIGHT);
  albumTextSprite.createSprite(METADATA_SPRITE_WIDTH, METADATA_SPRITE_HEIGHT);
  titleTextSprite.createSprite(METADATA_SPRITE_WIDTH, METADATA_SPRITE_HEIGHT);
  timeSprite.createSprite(TIME_SPRITE_WIDTH, TIME_SPRITE_HEIGHT);
  timeSprite.loadFont(GothamMedium26);
  artistTextSprite.loadFont(CircularStdLight34);
  albumTextSprite.loadFont(CircularStdLight34);
  titleTextSprite.loadFont(GothamMedium34);
  bluetoothSprite.createSprite(BLUETOOTH_WIDTH, BLUETOOTH_HEIGHT);
  bluetoothSprite.setSwapBytes(true);
  bluetoothSprite.pushImage(0, 0, BLUETOOTH_WIDTH, BLUETOOTH_HEIGHT, Bluetooth);
  playButtonSprite.createSprite(BUTTON_THREE_WIDTH, BUTTON_THREE_HEIGHT);
  playButtonSprite.setSwapBytes(true);
  timeBarSprite.createSprite(330, 21);
  volumeBarSprite.createSprite(15, 115);
  voltageSprite.createSprite(90, 40);
  voltageSprite.loadFont(GothamMedium26);

  displayMutex = xSemaphoreCreateMutex();
  ebeneMutex = xSemaphoreCreateMutex();
  mediaStateMutex = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();
  datenMutex = xSemaphoreCreateMutex();
  WIREMutex = xSemaphoreCreateMutex();

  strcpy(title, "Es wird nichts wiedergegeben");
  strcpy(artist, "");
  strcpy(album, "");

  // Seriial Communication

  Serial.begin(115200);
  Serial.printf("Die Übertragung beginnt!\n");

  SerialPort.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  SerialPort.onReceive(serialCallback);
  del[0] = 0xff;

  // Task Creation

  xTaskCreate(metadataTask, "Metadata Task", 4096, NULL, 5, &metadataTaskHandle);
  xTaskCreate(menueTask, "Menue Task", 4096, NULL, 3, &menueTaskHandle);
  xTaskCreate(touchTask, "Touch Task", 4096, NULL, 4, &touchTaskHandle);
  xTaskCreate(timeTask, "Time Task", 4096, NULL, 4, &timeTaskHandle);
  xTaskCreate(volumeTask, "Volume Task", 4096, NULL, 4, &volumeTaskHandle);
  xTaskCreate(volumeUpdateTask, "Volume Update Task", 2048, NULL, 4, &volumeUpdateTaskHandle);
  xTaskCreate(voltageTask, "Voltage Task", 2048, NULL, 5, &voltageTaskHandle);

  // Digital Pin Setup

  pinMode(32, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);
  delay(pdMS_TO_TICKS(3000));
  digitalWrite(33, LOW);

  // Spannungsüberwachung
  ina219.init();
  ina219.setADCMode(SAMPLE_MODE_128);
  ina219.setBusRange(BRNG_16);
}

void loop()
{
  // put your main code here, to run repeatedly:
}

void pngDrawCallback(PNGDRAW *pDraw)
{
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  mainScreen.pushImage(0, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void metadataTask(void *parameter)
{

  for (;;)
  {
    xSemaphoreTake(mediaStateMutex, portMAX_DELAY);
    if (mediaState == 1)
    {
      xSemaphoreGive(mediaStateMutex);
      playButtonSprite.pushImage(0, 0, BUTTON_THREE_WIDTH, BUTTON_THREE_HEIGHT, Play);
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      playButtonSprite.pushSprite(BUTTON_THREE_X_POS, BUTTON_THREE_Y_POS);
      xSemaphoreGive(displayMutex);
    }
    else
    {
      xSemaphoreGive(mediaStateMutex);
      playButtonSprite.pushImage(0, 0, BUTTON_THREE_WIDTH, BUTTON_THREE_HEIGHT, Pause);
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      playButtonSprite.pushSprite(BUTTON_THREE_X_POS, BUTTON_THREE_Y_POS);
      xSemaphoreGive(displayMutex);
    }
    if (a2dpState >= 3)
    {
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      bluetoothSprite.pushSprite(BLUETOOTH_X_POS, BLUETOOTH_Y_POS);
      xSemaphoreGive(displayMutex);
    }
    else
    {
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      mainScreen.fillRect(BLUETOOTH_X_POS, BLUETOOTH_Y_POS, BLUETOOTH_WIDTH, BLUETOOTH_HEIGHT, METADATA_BACKGROUND_COLOR);

      xSemaphoreGive(displayMutex);

      xSemaphoreTake(datenMutex, portMAX_DELAY);
      if (mediaStateBuffer != a2dpState)
      {
        strcpy(title, "Es wird nichts wiedergegeben");
        strcpy(artist, "");
        strcpy(album, "");
        neueDaten = true;
        mediaStateBuffer = a2dpState;
      }
      xSemaphoreGive(datenMutex);
    }

    voltageSprite.fillRect(0, 0, 90, 40, METADATA_BACKGROUND_COLOR);
    sprintf(outputVoltage, "%2.2fV", busVoltage_V);
    voltageSprite.drawString(outputVoltage, 5, 5);
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    voltageSprite.pushSprite(245, 25);
    xSemaphoreGive(displayMutex);
    if (neueDaten)
    {

      neueDaten = false;
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      mainScreen.drawRect(85, 85, 370, 172, METADATA_BACKGROUND_COLOR);

      xSemaphoreGive(displayMutex);

      titleLength = titleTextSprite.textWidth(title);
      artistLength = artistTextSprite.textWidth(artist);
      albumLength = albumTextSprite.textWidth(album);
      drawTextToSprite(titleTextSprite, title, METADATA_X_POS, METADATA_Y_START_POS);
      drawTextToSprite(artistTextSprite, artist, METADATA_X_POS, METADATA_Y_START_POS + METADATA_DISTANCE);
      drawTextToSprite(albumTextSprite, album, METADATA_X_POS, METADATA_Y_START_POS + (METADATA_DISTANCE * 2));
      titleToLong = false;
      artistToLong = false;
      albumToLong = false;
      if (albumLength > (METADATA_WINDOW_WIDTH))
      {
        albumToLong = true;
      }
      if (artistLength > (METADATA_WINDOW_WIDTH))
      {
        artistToLong = true;
      }
      if (titleLength > (METADATA_WINDOW_WIDTH))
      {
        titleToLong = true;
      }

      titlePos = 0;
      artistPos = 0;
      albumPos = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(40));
    if (aktuelleEbene != metadata)
    {
      vTaskSuspend(NULL);
    }
    if (titleToLong)
    {
      scrollByOne(titleTextSprite, title, METADATA_X_POS, METADATA_Y_START_POS, titlePos, titleLength, 0);
    }
    if (artistToLong)
    {
      scrollByOne(artistTextSprite, artist, METADATA_X_POS, METADATA_Y_START_POS + METADATA_DISTANCE, artistPos, artistLength, 1);
    }
    if (albumToLong)
    {
      scrollByOne(albumTextSprite, album, METADATA_X_POS, METADATA_Y_START_POS + (METADATA_DISTANCE * 2), albumPos, albumLength, 2);
    }
  }
}
void scrollByOne(TFT_eSprite &sprite, char *text, uint8_t xPos, uint8_t yPos, uint16_t &scrollPos, uint16_t textLength, uint8_t type)
{
  if (scrollPos > textLength - 300)
  {
    sprite.fillSprite(METADATA_BACKGROUND_COLOR);
    sprite.drawString(text, 0, 8);
    scrollPos = 0;
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    sprite.pushSprite(xPos, yPos, 0, 0, (METADATA_WINDOW_WIDTH), METADATA_SPRITE_HEIGHT);
    xSemaphoreGive(displayMutex);
    switch (type)
    {
    case 0:
    {
      waitForTitle = 100;
      break;
    }
    case 1:
    {
      waitForArtist = 100;
      break;
    }
    case 2:
    {
      waitForAlbum = 100;
      break;
    }
    }
  }

  switch (type)
  {
  case 0:
  {
    if (waitForTitle > 0)
      waitForTitle--;
    else
    {
      sprite.scroll(-1);
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      sprite.pushSprite(xPos, yPos, 0, 0, (METADATA_WINDOW_WIDTH), METADATA_SPRITE_HEIGHT);
      xSemaphoreGive(displayMutex);

      scrollPos += 1;
    }
    break;
  }
  case 1:
  {
    if (waitForArtist > 0)
      waitForArtist--;
    else
    {
      sprite.scroll(-1);
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      sprite.pushSprite(xPos, yPos, 0, 0, (METADATA_WINDOW_WIDTH), METADATA_SPRITE_HEIGHT);
      xSemaphoreGive(displayMutex);

      scrollPos += 1;
    }
    break;
  }
  case 2:
  {
    if (waitForAlbum > 0)
      waitForAlbum--;
    else
    {
      sprite.scroll(-1);
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      sprite.pushSprite(xPos, yPos, 0, 0, (METADATA_WINDOW_WIDTH), METADATA_SPRITE_HEIGHT);
      xSemaphoreGive(displayMutex);

      scrollPos += 1;
    }
    break;
  }
  }
}
void drawTextToSprite(TFT_eSprite &sprite, char *text, uint8_t xPos, uint8_t yPos)
{
  sprite.fillSprite(METADATA_BACKGROUND_COLOR);
  sprite.drawString(text, 0, 8);
  xSemaphoreTake(displayMutex, portMAX_DELAY);
  sprite.pushSprite(xPos, yPos, 0, 0, (METADATA_WINDOW_WIDTH), METADATA_SPRITE_HEIGHT);
  xSemaphoreGive(displayMutex);
}

void menueTask(void *parameter)
{

  for (;;)
  {
    xTaskNotifyWait(ULONG_MAX, ULONG_MAX, NULL, portMAX_DELAY);
    Serial.printf("Der Menuetask ist aktiv\n");
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    mainScreen.startWrite();
    png.openFLASH((uint8_t *)Menue, sizeof(Menue), pngDrawCallback);
    png.decode(NULL, 0);
    mainScreen.endWrite();
    mainScreen.fillRoundRect(MENUE_ONE_X_POS, MENUE_ONE_Y_POS, MENUE_ONE_WIDTH, MENUE_ONE_HEIGHT, 5, TFT_BLACK);
    mainScreen.fillRoundRect(MENUE_TWO_X_POS, MENUE_TWO_Y_POS, MENUE_TWO_WIDTH, MENUE_TWO_HEIGHT, 5, TFT_BLACK);
    mainScreen.fillRoundRect(MENUE_THREE_X_POS, MENUE_THREE_Y_POS, MENUE_THREE_WIDTH, MENUE_THREE_HEIGHT, 5, TFT_BLACK);

    if (nebelmaschiene)
    {
      // on
      mainScreen.fillRoundRect(400, 155, 40, 40, 5, TFT_WHITE);
    }
    else
    {
      // off
      mainScreen.fillRoundRect(350, 155, 40, 40, 5, TFT_WHITE);
    }
    if (led)
    {
      // on
      mainScreen.fillRoundRect(400, 110, 40, 40, 5, TFT_WHITE);
    }
    else
    {
      // off
      mainScreen.fillRoundRect(350, 110, 40, 40, 5, TFT_WHITE);
    }
    if (movingHeads)
    {
      // on
      mainScreen.fillRoundRect(400, 65, 40, 40, 5, TFT_WHITE);
    }
    else
    {
      // off
      mainScreen.fillRoundRect(350, 65, 40, 40, 5, TFT_WHITE);
    }

    xSemaphoreGive(displayMutex);
  }
}

void touchTask(void *parameter)
{

  for (;;)
  {
    Serial.printf("Der Touch Task lebt noch\n");
    if (xSemaphoreTake(WIREMutex, portMAX_DELAY) == pdTRUE)
    {
      touchPosX = (ft6336u.read_touch1_y());
      touchPosY = (320 - ft6336u.read_touch1_x());

      xSemaphoreGive(WIREMutex);
    }

    neueEingabe = false;

    if (prevTouchPosX != touchPosX && prevTouchPosY != touchPosY)
    {
      if (touchPosX != 257 && touchPosY != 63)
      {
        Serial.printf("X: %d,Y: %d aktuelle Ebene: %d, \n", touchPosX, touchPosY, aktuelleEbene);

        neueEingabe = true;
        prevTouchPosX = touchPosX;
        prevTouchPosY = touchPosY;
      }
    }
    Serial.printf("Landmarke\n");

    if (neueEingabe)
    {

      xSemaphoreTake(ebeneMutex, portMAX_DELAY);
      zwischnEbene = aktuelleEbene;
      xSemaphoreGive(ebeneMutex);
      switch (zwischnEbene)
      {

      case metadata:
      {

        if (((VOLUME_UP_X_POS <= touchPosX) && ((VOLUME_UP_X_POS + VOLUME_UP_WIDTH) >= touchPosX)) && ((VOLUME_UP_Y_POS <= touchPosY) && ((VOLUME_UP_Y_POS + VOLUME_UP_WIDTH) >= touchPosY)))
        {
          xSemaphoreTake(serialMutex, portMAX_DELAY);
          SerialPort.printf("AT+SPKVOL=+\r\n");
          SerialPort.flush();
          xSemaphoreGive(serialMutex);
        }
        if (((VOLUME_DOWN_X_POS <= touchPosX) && ((VOLUME_DOWN_X_POS + VOLUME_DOWN_WIDTH) >= touchPosX)) && ((VOLUME_DOWN_Y_POS <= touchPosY) && ((VOLUME_DOWN_Y_POS + VOLUME_DOWN_WIDTH) >= touchPosY)))
        {
          xSemaphoreTake(serialMutex, portMAX_DELAY);
          SerialPort.printf("AT+SPKVOL=-\r\n");
          SerialPort.flush();
          xSemaphoreGive(serialMutex);
        }
        if (((BUTTON_ONE_X_POS <= touchPosX) && ((BUTTON_ONE_X_POS + BUTTON_ONE_WIDTH) >= touchPosX)) && ((BUTTON_ONE_Y_POS <= touchPosY) && ((BUTTON_ONE_Y_POS + BUTTON_ONE_WIDTH) >= touchPosY)))
        {
          Serial.println("Knopf 1 gedrückt");
          Serial.printf("Es wird versucht den Mutex zu bekommen\n");
          vTaskSuspend(metadataTaskHandle);
          if (xSemaphoreTake(ebeneMutex, portMAX_DELAY == pdTRUE))
          {
            Serial.printf("Mutex erhalten\n");
            aktuelleEbene = hauptmenue;

            xSemaphoreGive(ebeneMutex);
          }
          vTaskDelay(20);
          xTaskNotify(menueTaskHandle, 0, eNoAction);
        }

        if ((((BUTTON_TWO_X_POS - 10) <= touchPosX) && ((BUTTON_TWO_X_POS + BUTTON_TWO_WIDTH + 10) >= touchPosX)) && (((BUTTON_TWO_Y_POS - 10) <= touchPosY) && ((BUTTON_TWO_Y_POS + BUTTON_TWO_WIDTH + 10) >= touchPosY)))
        {
          Serial.println("Knopf 2 gedrückt");
          xSemaphoreTake(serialMutex, portMAX_DELAY);
          SerialPort.printf("AT+BACKWARD\r\n");
          SerialPort.flush();
          xSemaphoreGive(serialMutex);
        }
        if ((((BUTTON_THREE_X_POS - 10) <= touchPosX) && ((BUTTON_THREE_X_POS + BUTTON_THREE_WIDTH + 10) >= touchPosX)) && (((BUTTON_THREE_Y_POS - 10) <= touchPosY) && ((BUTTON_THREE_Y_POS + BUTTON_THREE_WIDTH + 10) >= touchPosY)))
        {
          Serial.println("Knopf 3 gedrückt");
          xSemaphoreTake(serialMutex, portMAX_DELAY);
          SerialPort.printf("AT+PLAYPAUSE\r\n");
          SerialPort.flush();
          xSemaphoreGive(serialMutex);
        }
        if ((((BUTTON_FOUR_X_POS - 10) <= touchPosX) && ((BUTTON_FOUR_X_POS + BUTTON_FOUR_WIDTH + 10) >= touchPosX)) && (((BUTTON_FOUR_Y_POS - 10) <= touchPosY) && ((BUTTON_FOUR_Y_POS + BUTTON_FOUR_WIDTH + 10) >= touchPosY)))
        {
          Serial.println("Knopf 4 gedrückt");
          xSemaphoreTake(serialMutex, portMAX_DELAY);
          SerialPort.printf("AT+FORWARD\r\n");
          SerialPort.flush();
          xSemaphoreGive(serialMutex);
        }

        break;
      }

      case hauptmenue:
      {
        if (touchPosX > (MENUE_ONE_X_POS - 20) && touchPosX < (MENUE_ONE_X_POS + MENUE_ONE_WIDTH + 20) && touchPosY > (MENUE_ONE_Y_POS - 5) && touchPosY < (MENUE_ONE_Y_POS + MENUE_ONE_HEIGHT + 5))
        {
          movingHeads = !movingHeads;
          mainScreen.fillRect((MENUE_ONE_X_POS - 10), (MENUE_ONE_Y_POS - 10), (MENUE_ONE_WIDTH + 20), (MENUE_ONE_HEIGHT + 20), METADATA_BACKGROUND_COLOR);
          mainScreen.fillRoundRect(MENUE_ONE_X_POS, MENUE_ONE_Y_POS, MENUE_ONE_WIDTH, MENUE_ONE_HEIGHT, 5, TFT_BLACK);
          if (movingHeads)
          {
            digitalWrite(32, HIGH);
            // on
            mainScreen.fillRoundRect(400, 65, 40, 40, 5, TFT_WHITE);
          }
          else
          {
            // off
            digitalWrite(32, LOW);
            mainScreen.fillRoundRect(350, 65, 40, 40, 5, TFT_WHITE);
          }
        }
        if (touchPosX > (MENUE_TWO_X_POS - 20) && touchPosX < (MENUE_TWO_X_POS + MENUE_TWO_WIDTH + 20) && touchPosY > (MENUE_TWO_Y_POS - 5) && touchPosY < (MENUE_TWO_Y_POS + MENUE_TWO_HEIGHT + 5))
        {
          led = !led;
          mainScreen.fillRect((MENUE_TWO_X_POS - 10), (MENUE_TWO_Y_POS - 10), (MENUE_TWO_WIDTH + 20), (MENUE_TWO_HEIGHT + 20), METADATA_BACKGROUND_COLOR);
          mainScreen.fillRoundRect(MENUE_TWO_X_POS, MENUE_TWO_Y_POS, MENUE_TWO_WIDTH, MENUE_TWO_HEIGHT, 5, TFT_BLACK);
          if (led)
          {
            digitalWrite(27, HIGH);
            // on
            mainScreen.fillRoundRect(400, 110, 40, 40, 5, TFT_WHITE);
          }
          else
          {
            digitalWrite(27, LOW);
            // off
            mainScreen.fillRoundRect(350, 110, 40, 40, 5, TFT_WHITE);
          }
        }
        if (touchPosX > (MENUE_THREE_X_POS - 20) && touchPosX < (MENUE_THREE_X_POS + MENUE_THREE_WIDTH + 20) && touchPosY > (MENUE_THREE_Y_POS - 5) && touchPosY < (MENUE_THREE_Y_POS + MENUE_THREE_HEIGHT + 5))
        {
          nebelmaschiene = !nebelmaschiene;
          mainScreen.fillRect((MENUE_THREE_X_POS - 10), (MENUE_THREE_Y_POS - 10), (MENUE_THREE_WIDTH + 20), (MENUE_THREE_HEIGHT + 20), METADATA_BACKGROUND_COLOR);
          mainScreen.fillRoundRect(MENUE_THREE_X_POS, MENUE_THREE_Y_POS, MENUE_THREE_WIDTH, MENUE_THREE_HEIGHT, 5, TFT_BLACK);
          if (nebelmaschiene)
          {
            digitalWrite(4, HIGH);
            // on
            mainScreen.fillRoundRect(400, 155, 40, 40, 5, TFT_WHITE);
          }
          else
          {
            digitalWrite(4, LOW);
            // off
            mainScreen.fillRoundRect(350, 155, 40, 40, 5, TFT_WHITE);
          }
        }
        if (touchPosX > 400 && touchPosX < 479 && touchPosY > 1 && touchPosY < 60)
        {
          // Serial.printf("Es wird versucht den Mutex zu bekommen2\n");
          xSemaphoreTake(ebeneMutex, portMAX_DELAY);
          // Serial.printf("Mutex erhalten\n");
          aktuelleEbene = metadata;
          // Serial.printf("Rueckgabestatus: %d\n",xSemaphoreGive(ebeneMutex));
          xSemaphoreGive(ebeneMutex);
          xSemaphoreTake(displayMutex, portMAX_DELAY);
          png.openFLASH((uint8_t *)Metadata_large, sizeof(Metadata_large), pngDrawCallback);

          mainScreen.startWrite();

          png.decode(NULL, 0);

          mainScreen.endWrite();
          xSemaphoreGive(displayMutex);
          vTaskDelay(20);
          neueDaten = true;
          vTaskResume(metadataTaskHandle);
        }

        break;
      }
        Serial.printf("Landmarke\n");
      }
      vTaskDelay(pdMS_TO_TICKS(120));
    }

    // Serial.printf("X: %d,Y: %d\n",touchPosX,touchPosY);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
void serialCallback()
{
  Serial.println("Serial Revived");
  gefunden = false;
  neueZeit = false;
  while (SerialPort.available())
  {

    strcpy(buffer, SerialPort.readStringUntil('\n').c_str());

    ptr = strtok(buffer, "=+");
    while (ptr != NULL)
    {
      if (!strcmp(ptr, "TRACKINFO"))
      {
        ptr = strtok(NULL, "=\n\r");
        strcpy(inhalt, ptr);
        gefunden = true;
      }
      if (!strcmp(ptr, "TRACKSTAT"))
      {
        ptr = strtok(NULL, "=\n\r");
        strcpy(zeitUF, ptr);
        neueZeit = true;
      }
      if (!strcmp(ptr, "A2DPSTAT"))
      {
        ptr = strtok(NULL, "=\n\r");
        a2dpState = atoi(ptr);
      }
      if (!strcmp(ptr, "PLAYSTAT"))
      {
        ptr = strtok(NULL, "=\n\r");
        mediaState = atoi(ptr);
      }
      if (!strcmp(ptr, "SPKVOL"))
      {
        ptr = strtok(NULL, "=\n\r");
        spkvol = atoi(ptr);
        Serial.printf("Volume: %s", ptr);
        xTaskNotify(volumeTaskHandle, 0, eNoAction);
      }
      ptr = strtok(NULL, "=+");
    }
  }

  if (gefunden)
  {
    xSemaphoreTake(ebeneMutex, portMAX_DELAY);
    zwischnEbene = aktuelleEbene;
    xSemaphoreGive(ebeneMutex);
    if (zwischnEbene == metadata)
    {
      vTaskSuspend(metadataTaskHandle);
    }
    counter = 0;
    bufferTwo = strtok(inhalt, del);
    xSemaphoreTake(datenMutex, portMAX_DELAY);
    while (bufferTwo != NULL)
    {
      Serial.printf("\n%s\n", bufferTwo);
      switch (counter)
      {
      case 0:
      {
        strcpy(title, bufferTwo);
        break;
      }
      case 1:
      {
        strcpy(artist, bufferTwo);
        break;
      }
      case 2:
      {

        strcpy(album, bufferTwo);
        break;
      }
      }
      counter++;
      bufferTwo = strtok(NULL, del);
    }
    xSemaphoreGive(datenMutex);
    neueDaten = true;
    xSemaphoreTake(ebeneMutex, portMAX_DELAY);
    zwischnEbeneTwo = aktuelleEbene;
    xSemaphoreGive(ebeneMutex);
    if (zwischnEbeneTwo == metadata)
    {
      vTaskResume(metadataTaskHandle);
    }
  }
  if (neueZeit)
  {
    xSemaphoreTake(ebeneMutex, portMAX_DELAY);
    zwischnEbeneThree = aktuelleEbene;
    xSemaphoreGive(ebeneMutex);

    if (zwischnEbeneThree == metadata)
    {
      ptrTwo = strtok(zeitUF, ",");

      ptrTwo = strtok(NULL, ",");

      strcpy(bufferThree, ptrTwo);
      zeitAktuell = atoi(bufferThree);

      ptrTwo = strtok(NULL, ",");
      strcpy(bufferThree, ptrTwo);
      zeitGesamt = atoi(bufferThree);
      zeitAktuell /= 1000;
      zeitGesamt /= 1000;
      minuteAktuell = zeitAktuell / 60;
      sekundeAktuell = zeitAktuell % 60;
      minuteGesamt = zeitGesamt / 60;
      sekundeGesamt = zeitGesamt % 60;
      xTaskNotify(timeTaskHandle, 0, eNoAction);
    }
  }

  gefunden = false;
}
void timeTask(void *parameter)
{
  for (;;)
  {
    xTaskNotifyWait(ULONG_MAX, ULONG_MAX, NULL, portMAX_DELAY);
    xSemaphoreTake(ebeneMutex, portMAX_DELAY);
    zwischnEbeneFour = aktuelleEbene;
    xSemaphoreGive(ebeneMutex);

    if (zwischnEbeneFour == metadata)
    {
      sprintf(bufferFive, "%d:%02d / %d:%02d", minuteAktuell, sekundeAktuell, minuteGesamt, sekundeGesamt);
      timeSprite.fillSprite(METADATA_BACKGROUND_COLOR);
      timeSprite.drawString(bufferFive, 0, 0);
      timeBarSprite.fillSprite(METADATA_BACKGROUND_COLOR);
      timeBarSprite.fillRoundRect(10, 5, 310, 10, 3, 0x1924);

      timeBarSprite.fillSmoothCircle((((zeitAktuell + 0.0) / (zeitGesamt + 0.0)) * 310 + 10), 10, 10, TFT_WHITE);

      xSemaphoreTake(displayMutex, portMAX_DELAY);
      timeBarSprite.pushSprite(75, 235);
      timeSprite.pushSprite(TIME_POS_X, TIME_POS_Y);
      xSemaphoreGive(displayMutex);
    }
  }
}
void volumeTask(void *parameter)
{

  for (;;)
  {
    xTaskNotifyWait(ULONG_MAX, ULONG_MAX, NULL, portMAX_DELAY);
    xSemaphoreTake(ebeneMutex, portMAX_DELAY);
    zwischnEbeneFive = aktuelleEbene;
    xSemaphoreGive(ebeneMutex);
    if (zwischnEbeneFive == metadata)
    {
      volumeBarSprite.fillSprite(METADATA_BACKGROUND_COLOR);
      volumeBarSprite.fillSmoothRoundRect(5, 10, 5, 95, 2, TFT_BLACK);
      volumeBarSprite.fillSmoothCircle(7, 105 - ((spkvol + 0.0) / 15.0) * 95, 7, TFT_WHITE);
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      volumeBarSprite.pushSprite(428, 122);
      xSemaphoreGive(displayMutex);
    }
  }
}
void volumeUpdateTask(void *parameter)
{

  for (;;)
  {
    xSemaphoreTake(mediaStateMutex, portMAX_DELAY);
    if (mediaState == 1)
    {
      xSemaphoreGive(mediaStateMutex);
      xSemaphoreTake(serialMutex, portMAX_DELAY);
      SerialPort.printf("AT+SPKVOL\r\n");
      SerialPort.flush();
      xSemaphoreGive(serialMutex);
    }
    else
    {
      xSemaphoreGive(mediaStateMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void voltageTask(void *parameter)
{

  for (;;)
  {

    xSemaphoreTake(ebeneMutex, portMAX_DELAY);
    zwischenEbeneSix = aktuelleEbene;
    xSemaphoreGive(ebeneMutex);
    if (zwischenEbeneSix == metadata)
    {
      xSemaphoreTake(WIREMutex, portMAX_DELAY);
      busVoltage_V = ina219.getBusVoltage_V();

      xSemaphoreGive(WIREMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}