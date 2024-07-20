#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROMex.h>
#include "control.h"
LiquidCrystal_I2C lcd(0x27, 20, 4);

void initializeLCD()
{
  lcd.init(); // initialize the lcd
  lcd.backlight();
}

// |--------------------------------------------------------------------------------------------------------------------------------------------|
// |                                                         MENU                                                                               |
// |--------------------------------------------------------------------------------------------------------------------------------------------|

// Declaration of LCD Variables
const int NUM_MAIN_ITEMS = 3;
const int NUM_SETTING_ITEMS = 4;
const int NUM_TESTMACHINE_ITEMS = 4;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool runAutoFlag, settingFlag, settingEditFlag, testMenuFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"RUN AUTO", "ENTER TO RUN AUTO"},
    {"TEST MACHINE", "ENTER TO TEST"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"AFTER SENSE DELAY", "SEC"},
    {"SEALING DELAY", "SEC"},
    {"AFTER SEAL DELAY", "SEC"},
    {"SAVE SETTINGS", ""}};

int parametersTimer[NUM_SETTING_ITEMS] = {1, 1, 1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {600, 600, 600};

String testmachine_items[NUM_TESTMACHINE_ITEMS] = { // array with item names
    "CONVEYOR",
    "SEALER PNEUMATIC",
    "START SEALING",
    "EXIT"};

Control Conveyor(0);
Control PneumaticCylinder(0);
Control StartSeal(0);
Control AfterSenseTimer(100);
Control AfterSealTimer(100);

int AfterSenseDelay = 20;
int SealDelayAdd = 30;
int AfterSealDelayAdd = 40;

char *secondsToHHMMSS(int total_seconds)
{
  int hours, minutes, seconds;

  hours = total_seconds / 3600;         // Divide by number of seconds in an hour
  total_seconds = total_seconds % 3600; // Get the remaining seconds
  minutes = total_seconds / 60;         // Divide by number of seconds in a minute
  seconds = total_seconds % 60;         // Get the remaining seconds

  // Format the output string
  static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
  sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
  return hhmmss_str;
}

void saveSettings()
{
  EEPROM.writeInt(AfterSenseDelay, parametersTimer[0]);
  EEPROM.writeInt(SealDelayAdd, parametersTimer[1]);
  EEPROM.writeInt(AfterSealDelayAdd, parametersTimer[2]);
}

void loadSettings()
{
  parametersTimer[0] = EEPROM.readInt(AfterSenseDelay);
  parametersTimer[1] = EEPROM.readInt(SealDelayAdd);
  parametersTimer[2] = EEPROM.readInt(AfterSealDelayAdd);
}

void stopAll()
{
  Conveyor.stop();
  PneumaticCylinder.stop();
  StartSeal.stop();
}

void setTimers()
{
  AfterSenseTimer.setTimer(secondsToHHMMSS(parametersTimer[0]));
  // Conveyor.setTimer(secondsToHHMMSS(parametersTimer[0]));
  PneumaticCylinder.setTimer(secondsToHHMMSS(parametersTimer[1]));
  AfterSealTimer.setTimer(secondsToHHMMSS(parametersTimer[2]));
  // StartSeal.setTimer(secondsToHHMMSS(60));
}

// Button Declaration
static const int buttonPin = 2;
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 3;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 4;
int buttonStatePrevious3 = HIGH;

unsigned long minButtonLongPressDuration = 2000;
unsigned long buttonLongPressUpMillis;
unsigned long buttonLongPressDownMillis;
unsigned long buttonLongPressEnterMillis;
bool buttonStateLongPressUp = false;
bool buttonStateLongPressDown = false;
bool buttonStateLongPressEnter = false;

const int intervalButton = 50;
unsigned long previousButtonMillis;
unsigned long buttonPressDuration;
unsigned long currentMillis;

const int intervalButton2 = 50;
unsigned long previousButtonMillis2;
unsigned long buttonPressDuration2;
unsigned long currentMillis2;

const int intervalButton3 = 50;
unsigned long previousButtonMillis3;
unsigned long buttonPressDuration3;
unsigned long currentMillis3;

unsigned long previousMillisTimerScreen;
unsigned long currentMillisTimerScreen;

void InitializeButtons()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
}

void readButtonUpState()
{
  if (currentMillis - previousButtonMillis > intervalButton)
  {
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && buttonStatePrevious == HIGH && !buttonStateLongPressUp)
    {
      buttonLongPressUpMillis = currentMillis;
      buttonStatePrevious = LOW;
    }
    buttonPressDuration = currentMillis - buttonLongPressUpMillis;
    if (buttonState == LOW && !buttonStateLongPressUp && buttonPressDuration >= minButtonLongPressDuration)
    {
      buttonStateLongPressUp = true;
    }
    if (buttonStateLongPressUp == true)
    {
      // Insert Fast Scroll Up
      refreshScreen = true;
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
          {
            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
          }
          else
          {
            parametersTimer[currentSettingScreen] += 1;
          }
        }
        else
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            currentSettingScreen = 0;
          }
          else
          {
            currentSettingScreen++;
          }
        }
      }
      else if (testMenuFlag == true)
      {
        if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
        {
          currentTestMenuScreen = 0;
        }
        else
        {
          currentTestMenuScreen++;
        }
      }
      else
      {
        if (currentMainScreen == NUM_MAIN_ITEMS - 1)
        {
          currentMainScreen = 0;
        }
        else
        {
          currentMainScreen++;
        }
      }
    }

    if (buttonState == HIGH && buttonStatePrevious == LOW)
    {
      buttonStatePrevious = HIGH;
      buttonStateLongPressUp = false;
      if (buttonPressDuration < minButtonLongPressDuration)
      {
        // Short Scroll Up
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
            {
              parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
            }
            else
            {
              parametersTimer[currentSettingScreen] += 1;
            }
          }
          else
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              currentSettingScreen = 0;
            }
            else
            {
              currentSettingScreen++;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentTestMenuScreen = 0;
          }
          else
          {
            currentTestMenuScreen++;
          }
        }
        else
        {
          if (currentMainScreen == NUM_MAIN_ITEMS - 1)
          {
            currentMainScreen = 0;
          }
          else
          {
            currentMainScreen++;
          }
        }
      }
    }
    previousButtonMillis = currentMillis;
  }
}

void readButtonDownState()
{
  if (currentMillis2 - previousButtonMillis2 > intervalButton2)
  {
    int buttonState2 = digitalRead(buttonPin2);
    if (buttonState2 == LOW && buttonStatePrevious2 == HIGH && !buttonStateLongPressDown)
    {
      buttonLongPressDownMillis = currentMillis2;
      buttonStatePrevious2 = LOW;
    }
    buttonPressDuration2 = currentMillis2 - buttonLongPressDownMillis;
    if (buttonState2 == LOW && !buttonStateLongPressDown && buttonPressDuration2 >= minButtonLongPressDuration)
    {
      buttonStateLongPressDown = true;
    }
    if (buttonStateLongPressDown == true)
    {
      refreshScreen = true;
      if (settingFlag == true)
      {
        if (settingEditFlag == true)
        {
          if (parametersTimer[currentSettingScreen] <= 0)
          {
            parametersTimer[currentSettingScreen] = 0;
          }
          else
          {
            parametersTimer[currentSettingScreen] -= 1;
          }
        }
        else
        {
          if (currentSettingScreen == 0)
          {
            currentSettingScreen = NUM_SETTING_ITEMS - 1;
          }
          else
          {
            currentSettingScreen--;
          }
        }
      }
      else if (testMenuFlag == true)
      {
        if (currentTestMenuScreen == 0)
        {
          currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
        }
        else
        {
          currentTestMenuScreen--;
        }
      }
      else
      {
        if (currentMainScreen == 0)
        {
          currentMainScreen = NUM_MAIN_ITEMS - 1;
        }
        else
        {
          currentMainScreen--;
        }
      }
    }

    if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
    {
      buttonStatePrevious2 = HIGH;
      buttonStateLongPressDown = false;
      if (buttonPressDuration2 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (currentSettingScreen = 2)
            {
              if (parametersTimer[currentSettingScreen] <= 2)
              {
                parametersTimer[currentSettingScreen] = 2;
              }
              else
              {
                parametersTimer[currentSettingScreen] -= 1;
              }
            }
            else
            {
              if (parametersTimer[currentSettingScreen] <= 0)
              {
                parametersTimer[currentSettingScreen] = 0;
              }
              else
              {
                parametersTimer[currentSettingScreen] -= 1;
              }
            }
          }
          else
          {
            if (currentSettingScreen == 0)
            {
              currentSettingScreen = NUM_SETTING_ITEMS - 1;
            }
            else
            {
              currentSettingScreen--;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == 0)
          {
            currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
          }
          else
          {
            currentTestMenuScreen--;
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            currentMainScreen = NUM_MAIN_ITEMS - 1;
          }
          else
          {
            currentMainScreen--;
          }
        }
      }
    }
    previousButtonMillis2 = currentMillis2;
  }
}

void readButtonEnterState()
{
  if (currentMillis3 - previousButtonMillis3 > intervalButton3)
  {
    int buttonState3 = digitalRead(buttonPin3);
    if (buttonState3 == LOW && buttonStatePrevious3 == HIGH && !buttonStateLongPressEnter)
    {
      buttonLongPressEnterMillis = currentMillis3;
      buttonStatePrevious3 = LOW;
    }
    buttonPressDuration3 = currentMillis3 - buttonLongPressEnterMillis;
    if (buttonState3 == LOW && !buttonStateLongPressEnter && buttonPressDuration3 >= minButtonLongPressDuration)
    {
      buttonStateLongPressEnter = true;
    }
    if (buttonStateLongPressEnter == true)
    {
      // Insert Fast Scroll Enter
    }

    if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
    {
      buttonStatePrevious3 = HIGH;
      buttonStateLongPressEnter = false;
      if (buttonPressDuration3 < minButtonLongPressDuration)
      {
        refreshScreen = true;
        if (currentMainScreen == 0 && settingFlag == true)
        {
          if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
          {
            settingFlag = false;
            saveSettings();
            loadSettings();
            currentSettingScreen = 0;
            setTimers();
          }
          else
          {
            if (settingEditFlag == true)
            {
              settingEditFlag = false;
            }
            else
            {
              settingEditFlag = true;
            }
          }
        }
        else if (currentMainScreen == 1 && testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentMainScreen = 0;
            currentTestMenuScreen = 0;
            testMenuFlag = false;
            stopAll();
          }
          else if (currentTestMenuScreen == 0)
          {
            if (Conveyor.getMotorState() == false)
            {
              Conveyor.relayOn();
            }
            else
            {
              Conveyor.relayOff();
            }
          }
          else if (currentTestMenuScreen == 1)
          {
            if (PneumaticCylinder.getMotorState() == false)
            {
              PneumaticCylinder.relayOn();
            }
            else
            {
              PneumaticCylinder.relayOff();
            }
          }
          else if (currentTestMenuScreen == 2)
          {
            if (StartSeal.getMotorState() == false)
            {
              StartSeal.relayOn();
            }
            else
            {
              StartSeal.relayOff();
            }
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            settingFlag = true;
          }
          else if(currentMainScreen == 1){
            runAutoFlag == true;
          }
          else if (currentMainScreen == 2)
          {
            testMenuFlag = true;
          }
        }
      }
    }
    previousButtonMillis3 = currentMillis3;
  }
}

void ReadButtons()
{
  currentMillis = millis();
  currentMillis2 = millis();
  currentMillis3 = millis();
  readButtonEnterState();
  readButtonUpState();
  readButtonDownState();
}

// |--------------------------------------------------------------------------------------------------------------------------------------------|
// |                                                         INITIALIZE METHOD                                                                  |
// |--------------------------------------------------------------------------------------------------------------------------------------------|
void printMainMenu(String MenuItem, String Action)
{
  lcd.clear();
  lcd.print(MenuItem);
  lcd.setCursor(0, 3);
  lcd.write(0);
  lcd.setCursor(2, 3);
  lcd.print(Action);
  refreshScreen = false;
}

void printSettingScreen(String SettingTitle, String Unit, double Value, bool EditFlag, bool SaveFlag)
{
  lcd.clear();
  lcd.print(SettingTitle);
  lcd.setCursor(0, 1);

  if (SaveFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    lcd.print("ENTER TO SAVE ALL");
  }
  else
  {
    lcd.print(Value);
    lcd.print(" ");
    lcd.print(Unit);
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    if (EditFlag == false)
    {
      lcd.print("ENTER TO EDIT");
    }
    else
    {
      lcd.print("ENTER TO SAVE");
    }
  }
  refreshScreen = false;
}

void printTestScreen(String TestMenuTitle, String Job, bool Status, bool ExitFlag)
{
  lcd.clear();
  lcd.print(TestMenuTitle);
  if (ExitFlag == false)
  {
    lcd.setCursor(0, 2);
    lcd.print(Job);
    lcd.print(" : ");
    if (Status == true)
    {
      lcd.print("ON");
    }
    else
    {
      lcd.print("OFF");
    }
  }

  if (ExitFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Exit Test");
  }
  else
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Run Test");
  }
  refreshScreen = false;
}

void printRunAuto(String Job, String TimeRemaining)
{
  lcd.clear();
  lcd.print("Run Auto");
  lcd.setCursor(0, 1);
  lcd.print("Status : ");
  lcd.setCursor(0, 2);
  lcd.print(Job);
  lcd.setCursor(0, 3);
  lcd.print(TimeRemaining);
}

void printScreens()
{
  if (settingFlag == true)
  {
    if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
    {
      printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, true);
    }
    else
    {
      printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, false);
    }
  }
  else if (testMenuFlag == true)
  {
    switch (currentTestMenuScreen)
    {
    case 0:
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", Conveyor.getMotorState(), false);
      break;
    case 1:
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", PneumaticCylinder.getMotorState(), false);
      break;
    case 2:
      printTestScreen(testmachine_items[currentTestMenuScreen], "Status", StartSeal.getMotorState(), false);
      break;
    case 3:
      printTestScreen(testmachine_items[currentTestMenuScreen], "", true, true);
      break;

    default:
      break;
    }
  }
  else if (runAutoFlag == true)
  {
  }
  else
  {
    printMainMenu(menu_items[currentMainScreen][0], menu_items[currentMainScreen][1]);
  }
}

void setup()
{
  // saveSettings();
  loadSettings();
  initializeLCD();
  InitializeButtons();
  Serial.begin(9600);
  refreshScreen = true;
}

void loop()
{
  // put your main code here, to run repeatedly:
  ReadButtons();
  if (refreshScreen == true)
  {
    printScreens();
  }

  if (runAutoFlag == true)
  {
    if (currentMillisTimerScreen - previousMillisTimerScreen >= 1000)
    {
      // save the last time you blinked the LED
      previousMillisTimerScreen = currentMillisTimerScreen;
      refreshScreen = true;
    }
    
  }
}
