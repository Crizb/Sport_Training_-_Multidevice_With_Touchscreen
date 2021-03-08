 #include "SPI.h"
#include "Wire.h"
#include <EEPROM.h>
#include <String.h>
/* Platform specific includes */
#include "FT_VM800P43_50.h"

/*define interrupt register bits*/
#define TAG_INTERRUPT 0x04
#define TOUCH_INTERRUPT 0x02

/* Global object for FT800 Implementation */
FT800IMPL_SPI FTImpl(FT_CS_PIN,FT_PDN_PIN,FT_INT_PIN);

uint8_t touchedTag=0;

float previousMillis1 = 0;

/* Definition of the button positions*/
#define buttonYellow 1
#define buttonBlue A5
#define buttonRed 2
#define buttonGreen A1
#define buttonWhiteLeft A2
#define buttonWhiteRight A4

/* Definition of the Lights */
#define lightYellow 3
#define lightBlue 8
#define lightRed 5
#define lightGreen 6
#define lightWhiteLeft 7
#define lightWhiteRight A0

/* Battery */
#define batteryInput A7
int batteryLevelRaw = 0; 

/* Simon Says */
#define MAX_LEVEL 100
#define basicVelocity 500
#define increaseVelocity 5
int sequence[MAX_LEVEL];
int your_sequence[MAX_LEVEL];
int difficultyLevel = 1;
int level = 1;
int velocity = basicVelocity;
int score = 0;
bool wrongSequence = 0;

/* Speed Test */
unsigned long startTime = 0;
float timePassed = 0;
float timeSet = 60;
int lastPressed = 0;
int pointsGained = 0;
int toPress = 0;
int error1 = 0;

bool lastButton[6] = {false, false, false, false, false, false};

bool pressedGreen = 0;
bool pressedRed = 0;
bool pressedBlue = 0;
bool pressedYellow = 0;
bool pressedWhiteLeft = 0;
bool pressedWhiteRight = 0;

/* Reflex Test*/
float timeMeasured = 0;
float timeRandom = 0;
float timeFromStart = 0;
float resultInteger[20];
float resultToAverage[20];
bool leftPressed = 0;
bool rightPressed = 0;
int timeCurrent = 0;
int numberOfResults = 0;
int lowRandom = 0;
int highRandom = 0;
int reflexButton = 0;
int timeRandomLow = 0;
int timeRandomHigh = 0;

/* Hundred*/
int hundredIntRow[36];
int pointsHundred = 0;
/* ----------------------------------------------------------------------------- */

int soundVolume = 255;
int pressVolume = 255;
bool backFromGame = 0;
bool gameStarted = 0;
bool hundredGamemode = 0;
bool colorChange = true;

int16_t BootupConfigure()
{
	uint32_t chipid = 0;
  FTImpl.Init(FT_DISPLAY_RESOLUTION);

  delay(20);//for safer side
  
  /* Identify the chip */
  chipid = FTImpl.Read32(FT_ROM_CHIPID);
  if(FT800_CHIPID != chipid)
    return 1;
  
  /*main_menu screen pressure sensitivity adjustment*/
  FTImpl.Write16(REG_TOUCH_RZTHRESH,1200); 

  /* Set the Display & audio pins */
  FTImpl.SetDisplayEnablePin(FT_DISPENABLE_PIN);
  FTImpl.SetAudioEnablePin(FT_AUDIOENABLE_PIN); 
  
  /*Turn on display and audio*/
  FTImpl.DisplayOn();
  FTImpl.AudioOn();  
  return 0;
}

void Calibrate()
{
  FTImpl.DLStart();
  FTImpl.ClearColorRGB(64,64,64); 
  FTImpl.Clear(1,1,1);    
  FTImpl.ColorRGB(0xff, 0xff, 0xff);
  FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), (FT_DISPLAYHEIGHT/2), 30, FT_OPT_CENTER, "Prosze wcisnac kolejne kropki");
  FTImpl.Cmd_Calibrate(0);
  FTImpl.Finish();
}

void buttonState()
{
  pressedGreen = 0;
  pressedRed = 0;
  pressedBlue = 0;
  pressedYellow = 0;
  pressedWhiteLeft = 0;
  pressedWhiteRight = 0;

  bool tempButton = digitalRead(buttonGreen);
  if (tempButton != lastButton[0])
  {
    if (tempButton == HIGH) {} 
    else
      pressedGreen = 1;
  }
  lastButton[0] = tempButton;

  tempButton = digitalRead(buttonRed);
  if (tempButton != lastButton[1]) 
  {
    if (tempButton == HIGH ) {} 
    else 
      pressedRed = 1;
  }
  lastButton[1] = tempButton;

  tempButton = digitalRead(buttonBlue);
  if (tempButton != lastButton[2])
  {
    if (tempButton == HIGH) {} 
    else
      pressedBlue = 1;
  }
  lastButton[2] = tempButton;

  tempButton = digitalRead(buttonYellow);
  if (tempButton != lastButton[3]) 
  {
    if (tempButton == HIGH) {} 
    else
      pressedYellow = 1;
  }
  lastButton[3] = tempButton;

  tempButton = digitalRead(buttonWhiteLeft);
  if (tempButton != lastButton[4]) 
  {
    if (tempButton == HIGH) {} 
    else
      pressedWhiteLeft = 1;
  }
  lastButton[4] = tempButton;

  tempButton = digitalRead(buttonWhiteRight);
  if (tempButton != lastButton[5]) 
  {
    if (tempButton == HIGH) {} 
    else
      pressedWhiteRight = 1;
  }
  lastButton[5] = tempButton;
}

void lightState(bool state)
{
  digitalWrite(lightGreen, state);
  digitalWrite(lightRed, state);
  digitalWrite(lightBlue, state);
  digitalWrite(lightYellow, state);
  digitalWrite(lightWhiteLeft, state);
  digitalWrite(lightWhiteRight, state);
}

int32_t Dec2Ascii(char *pSrc,int32_t value)
{
  int16_t Length;
  char *pdst,charval;
  int32_t CurrVal = value,tmpval,i;
  char tmparray[16],idx = 0;

  Length = strlen(pSrc);
  pdst = pSrc + Length;
  
  if(0 == value)
  {
    *pdst++ = '0';
    *pdst++ = '\0';
    return 0;
  }
  
  if(CurrVal < 0)
  {
    *pdst++ = '-';
    CurrVal = - CurrVal;
  }
  
  while(CurrVal > 0){
    tmpval = CurrVal;
    CurrVal /= 10;
    tmpval = tmpval - CurrVal*10;
    charval = '0' + tmpval;
    tmparray[idx++] = charval;
  }

  for(i=0;i<idx;i++)
  {
    *pdst++ = tmparray[idx - i - 1];
  }
  *pdst++ = '\0';
  return 0;
}

void battery_display()
{
  char batteryString[4];
  
  if(refreshTimer1(5000))
  {
    for(int i = 0; i < 50; i++)
      batteryLevelRaw += analogRead(batteryInput);
    batteryLevelRaw = batteryLevelRaw / 50;
    batteryLevelRaw = map(batteryLevelRaw, 307 , 430, 0, 100);
    if(batteryLevelRaw <= 0) 
      batteryLevelRaw = 0;
    if(batteryLevelRaw >= 100) 
      batteryLevelRaw = 100;
  }
  
  FTImpl.ColorRGB(0xff,0x00,0x00);
  itoa(batteryLevelRaw, batteryString, 10);
  strcat(batteryString, "%");
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH - 25, 15, 26, FT_OPT_CENTER, batteryString);
}

void main_menu()
{ 
  char cstr[16];
  int32_t LoopFlag = 0,wbutton,hbutton,tagVal,tagOption;
  char StringArray[100];
  uint32_t ReadWord;
  int16_t xvalue,yvalue,pendown;
  sTagXY sTagxy;

  while(1)
  {   
    backFromGame = 0;
    gameStarted = 0;
    lightState(0);

    wbutton = FT_DISPLAYWIDTH/4;
    hbutton = FT_DISPLAYHEIGHT/4;
  
    FTImpl.GetTagXY(sTagxy);     
    FTImpl.DLStart();  
    battery_display();
    FTImpl.ColorRGB(0xff,0x00,0x00);
    FTImpl.TagMask(0);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 55, FT_DISPLAYHEIGHT/8, 30, FT_OPT_CENTER, "COACH");
   
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 55, FT_DISPLAYHEIGHT/8, 30, FT_OPT_CENTER, "MIKESZ");
    
    ReadWord = sTagxy.tag;
    Dec2Ascii(StringArray,ReadWord);
    tagVal = ReadWord;

    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
    FTImpl.TagMask(1);

    tagOption = 0;
    if(11 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.Tag(11);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/8) - (wbutton/2) + 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Test Pamieci");

    tagOption = 0;
    if(12 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.Tag(12);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*4/8) - (wbutton/2),(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Test Refleksu");

    tagOption = 0;
    if(13 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.Tag(13);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*8/8) - (wbutton) - 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Test Szybkosci");

    tagOption = 0;
    if(14 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.Tag(14);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/8) - (wbutton/2) + 10,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2) + 10,wbutton,hbutton,27,tagOption,"Setka 6x6");

    tagOption = 0;
    if(15 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    //FTImpl.Tag(15);
    //FTImpl.Cmd_Button((FT_DISPLAYWIDTH*4/8) - (wbutton/2),(FT_DISPLAYHEIGHT*3/4) - (hbutton/2) + 10,wbutton,hbutton,27,tagOption,"EMPTY");

    tagOption = 0;
    if(16 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.Tag(16);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*8/8) - (wbutton) - 10,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2) + 10,wbutton,hbutton,27,tagOption,"Ustawienia");
    
    FTImpl.DLEnd();
    FTImpl.Finish();  

    if(tagVal == 11 || tagVal == 12 || tagVal == 13 || tagVal == 14 || tagVal == 15 || tagVal == 16)
    {
      FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
      delay(150);
      FTImpl.StopSound();
      if(tagVal == 11)
        simonSays_game();
      if(tagVal == 12)
        reflexTest_game();
      if(tagVal == 13)
        speedTest_menu();
      if(tagVal == 14)
        hundred_game();
      if(tagVal == 15);

      if(tagVal == 16)
        settings_display();
      lightState(1);
      delay(100);
    }
  }
}

void setup()
{
	analogReference(DEFAULT);
  
	if(BootupConfigure())
	{
		//error case - do not do any thing
	}
  else
  {
    FTImpl.EnableInterrupts(1,TAG_INTERRUPT|TOUCH_INTERRUPT);
    FTImpl.Write(REG_VOL_SOUND,soundVolume);
    FTImpl.Write(REG_ROTATE, 1);    
    Calibrate();
	}
  pinMode(buttonYellow, INPUT_PULLUP);
  pinMode(buttonBlue, INPUT_PULLUP);
  pinMode(buttonRed, INPUT_PULLUP);
  pinMode(buttonGreen, INPUT_PULLUP);
  pinMode(buttonWhiteLeft, INPUT_PULLUP);
  pinMode(buttonWhiteRight, INPUT_PULLUP);
  
  pinMode(lightYellow, INPUT);
  pinMode(lightBlue, INPUT);
  pinMode(lightRed, INPUT);
  pinMode(lightGreen, INPUT);
  pinMode(lightWhiteLeft, INPUT);
  pinMode(lightWhiteRight, INPUT);
  pinMode(batteryInput, INPUT);
  
  digitalWrite(lightYellow, HIGH);
  digitalWrite(lightBlue, HIGH);
  digitalWrite(lightRed, HIGH);
  digitalWrite(lightGreen, HIGH);
  digitalWrite(lightWhiteLeft, HIGH);
  digitalWrite(lightWhiteRight, HIGH);
  delay(200);
  EEPROM.get(90, pressVolume);
  EEPROM.get(92, timeRandomLow);
  EEPROM.get(94, timeRandomHigh);
  main_menu();
}

void loop()
{
}

/* Ustawienia - Settings ----------------------------------------------------------------------------------- */ 
void settings_display()
{
  while(1)
  {
    char cstr[5];
    int32_t tagVal,wbutton,hbutton,tagOption;
    uint32_t ReadWord;
    char StringArray[100];
    
    FTImpl.DLStart();
    
    sTagXY sTagxy;
    FTImpl.GetTagXY(sTagxy);
    ReadWord = sTagxy.tag;
    Dec2Ascii(StringArray,ReadWord);
    tagVal = ReadWord;
    
    wbutton = FT_DISPLAYWIDTH/6;
    hbutton = FT_DISPLAYHEIGHT/6;
  
    FTImpl.Tag(0);
    FTImpl.ColorRGB(0xff,0,0);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, 22, 30, FT_OPT_CENTER, "Ustawienia");
    
    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
      
    tagOption = 0;
    if(90 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_FGColor(0xff0000);
    FTImpl.Tag(90);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
  
    wbutton = FT_DISPLAYWIDTH/8;
    hbutton = FT_DISPLAYHEIGHT/8;
  
    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
    
    tagOption = 0;
    if(21 == tagVal)
      tagOption = FT_OPT_FLAT;
    FTImpl.Tag(21);
    FTImpl.Cmd_Button(10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Wl");
  
    tagOption = 0;
    if(31 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(31);
    FTImpl.Cmd_Button(FT_DISPLAYWIDTH/2 - 5 - wbutton,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"+");
  
    tagOption = 0;
    if(41 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(41);
    FTImpl.Cmd_Button(FT_DISPLAYWIDTH - wbutton*2 - 20,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"+");
    
    FTImpl.Cmd_FGColor(0xff0000);
    FTImpl.ColorRGB(0xff,0xff,0xff);
    tagOption = 0;
    if(22 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(22);
    FTImpl.Cmd_Button(20 + wbutton,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Wyl");
  
    tagOption = 0;
    if(32 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(32);
    FTImpl.Cmd_Button(FT_DISPLAYWIDTH/2 + 5,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"-");
  
    tagOption = 0;
    if(42 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(42);
    FTImpl.Cmd_Button(FT_DISPLAYWIDTH - wbutton - 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"-");
  
    FTImpl.Tag(0);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT*2/4) - (hbutton), 29, FT_OPT_CENTER, "Min. Czas");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH - wbutton - 15, (FT_DISPLAYHEIGHT*2/4) - (hbutton), 29, FT_OPT_CENTER, "Max. Czas");
    if(pressVolume > 0)
      FTImpl.Cmd_Text(wbutton + 15, (FT_DISPLAYHEIGHT*2/4) + (hbutton), 29, FT_OPT_CENTER, "Wl");  
    else
      FTImpl.Cmd_Text(wbutton + 15, (FT_DISPLAYHEIGHT*2/4) + (hbutton), 29, FT_OPT_CENTER, "Wyl");
    FTImpl.Cmd_Text(wbutton + 15, (FT_DISPLAYHEIGHT*2/4) - (hbutton), 29, FT_OPT_CENTER, "Dzwiek");
  
    float tmpFloat = timeRandomLow;
    tmpFloat = tmpFloat / 1000;
    dtostrf(tmpFloat, 3, 1, cstr);
    strcat(cstr, "s");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT*2/4) + (hbutton), 29, FT_OPT_CENTER, cstr);
    
    tmpFloat = timeRandomHigh;
    tmpFloat = tmpFloat / 1000;
    dtostrf(tmpFloat, 3, 1, cstr);
    strcat(cstr, "s");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH - wbutton - 15, (FT_DISPLAYHEIGHT*2/4) + (hbutton), 29, FT_OPT_CENTER, cstr);
  
    FTImpl.DLEnd();
    FTImpl.Finish();
    if(tagVal == 90 || tagVal == 21 || tagVal == 22 || tagVal == 31 || tagVal == 32 || tagVal == 41 || tagVal == 42) 
    {
      FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
      delay(150);
      FTImpl.StopSound();
      if(tagVal == 90)
      {
        EEPROM.put(90, pressVolume);
        EEPROM.put(92, timeRandomLow);
        EEPROM.put(94, timeRandomHigh);
        backFromGame = 1;
      }
      if(tagVal == 21)
        pressVolume = 255;
      if(tagVal == 22)
        pressVolume = 0;
      if(tagVal == 31)
        if(timeRandomLow < timeRandomHigh - 100)
          timeRandomLow = timeRandomLow + 100;
      if(tagVal == 32)
        if(timeRandomLow > 0)
          timeRandomLow = timeRandomLow - 100;
      if(tagVal == 41)
        timeRandomHigh = timeRandomHigh + 100;
      if(tagVal == 42)
        if(timeRandomHigh > timeRandomLow + 100)
          timeRandomHigh = timeRandomHigh - 100;
    }
    if(backFromGame == 1)
      return;
  }
}
/* ------------------------------------------------------------------------------------*/

/* Setka - Hundred ----------------------------------------------------------------------------------- */
void swap (int *a, int *b)  
{  
    int temp = *a;  
    *a = *b;  
    *b = temp;  
}  

void randomize(int arr[], int n)  
{  
  srand(millis());  
  for (int i = n - 1; i > 0; i--)  
  {  
    int j = rand() % (i + 1);    
    swap(&arr[i], &arr[j]);  
  }  
}  

void hundred_display()
{
  char cstr[16];
  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
   
  FTImpl.DLStart();
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  tagOption = 0;
  if(90 == tagVal)
    tagOption = FT_OPT_FLAT;
  
  FTImpl.ColorRGB(0xff,0xff,0xff);
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(90);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
    
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;
  if(pointsHundred > 36)
    hundredGamemode = 0;
  if(hundredGamemode == 0)
  {
    FTImpl.Tag(0);
    FTImpl.ColorRGB(0xff,0,0);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, 22, 30, FT_OPT_CENTER, "Setka 6x6");
  
    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
    
    tagOption = 0;
    if(51 == tagVal)
      tagOption = FT_OPT_FLAT;
    
    FTImpl.Tag(51);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton),(FT_DISPLAYHEIGHT/4) - (hbutton/2),wbutton*2,hbutton*2,30,tagOption,"START");

    tagOption = 0;
    if(52 == tagVal)
      tagOption = FT_OPT_FLAT;
    if(!colorChange)
    {
      FTImpl.Cmd_FGColor(0xff0000);
      FTImpl.ColorRGB(0xff,0xff,0xff);
    }
    else
    {
      FTImpl.Cmd_FGColor(0x00ff00);
      FTImpl.ColorRGB(0,0,0);
    }
    FTImpl.Tag(52);
    FTImpl.Cmd_Button(10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton*3/2,hbutton,27,tagOption,"Zmienny Kolor");
    if(pointsHundred > 0)
    {
      FTImpl.Tag(0);
      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT*3/4 - 50, 28, FT_OPT_CENTER, "Czas:");

      dtostrf(timePassed, 4, 1, cstr);
      strcat(cstr, "s");
      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT*3/4-25, 28, FT_OPT_CENTER, cstr);

      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH*5/8, FT_DISPLAYHEIGHT*3/4 + 5, 28, FT_OPT_CENTER, "Bledy:");

      itoa(error1, cstr, 10);
      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH*5/8, FT_DISPLAYHEIGHT*3/4 + 30, 28, FT_OPT_CENTER, cstr);

      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH*3/8, FT_DISPLAYHEIGHT*3/4 + 5, 28, FT_OPT_CENTER, "Punkty:");

      itoa(pointsHundred-1, cstr, 10);
      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH*3/8, FT_DISPLAYHEIGHT*3/4 + 30, 28, FT_OPT_CENTER, cstr);
    }
  }
  else if(hundredGamemode == 1)
  {
    FTImpl.Tag(0);
    FTImpl.ColorRGB(0xff,0,0);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, 22, 28, FT_OPT_CENTER, "Setka 6x6");

    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, FT_DISPLAYHEIGHT/4, 28, FT_OPT_CENTER, "Czas:");
    
    timePassed = millis() - startTime;
    timePassed = timePassed/1000;
    dtostrf(timePassed, 4, 1, cstr);
    strcat(cstr, "s");
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, FT_DISPLAYHEIGHT/4 + 25, 28, FT_OPT_CENTER, cstr);

    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, FT_DISPLAYHEIGHT/2, 28, FT_OPT_CENTER, "Bledy:");

    itoa(error1, cstr, 10);
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, FT_DISPLAYHEIGHT/2 + 25, 28, FT_OPT_CENTER, cstr);

    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, FT_DISPLAYHEIGHT*3/4, 28, FT_OPT_CENTER, "Cyfra:");

    itoa(pointsHundred, cstr, 10);
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/8, FT_DISPLAYHEIGHT*3/4 + 25, 28, FT_OPT_CENTER, cstr);

    wbutton = FT_DISPLAYHEIGHT/8;
    hbutton = FT_DISPLAYHEIGHT/8;
    int x = 0;
    for(int i = 0; i < 6; i ++)
    {
      for(int j = 0; j < 6; j++)
      {
        tagOption = 0;
        if(x + 1 == tagVal)
          tagOption = FT_OPT_FLAT;
        itoa(hundredIntRow[x], cstr, 10);
        if(pointsHundred > hundredIntRow[x] && colorChange == 1)
          FTImpl.Cmd_FGColor(0x00ff00);
        else
          FTImpl.Cmd_FGColor(0xff0000);
        FTImpl.Tag(x + 1);
        FTImpl.Cmd_Button(104 + FT_DISPLAYHEIGHT*(1 + j)/8,(FT_DISPLAYHEIGHT*(1+i)/8),wbutton,hbutton,26,tagOption,cstr); 
        x++;
      }
    }
  }
  if(tagVal == 90 || tagVal == 51 || tagVal == 52)
  {
    FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
    delay(150);
    FTImpl.StopSound();
    if(tagVal == 90)
    {
      if(hundredGamemode != 0)
        hundredGamemode = 0;
      else 
      {
        error1 = 0;
        pointsHundred = 0;
        timePassed = 0;
        backFromGame = 1;
      }
    }
    if(tagVal == 51)
    {
      for(int i = 0; i < 36; i++)
      {
        hundredIntRow[i] = i + 1;
      }
      int n = sizeof(hundredIntRow) / sizeof(hundredIntRow[0]);  
      randomize(hundredIntRow, n);
      pointsHundred = 1;
      startTime = millis();
      error1 = 0;
      hundredGamemode = 1;
    }
    if(tagVal == 52)
    {
      if(colorChange == 1)
        colorChange = 0;
      else
        colorChange = 1;
    }
  }
  if(tagVal == 1 || tagVal == 2 || tagVal == 3 || tagVal == 4 || tagVal == 5 || tagVal == 6 || 
     tagVal == 7 || tagVal == 8 || tagVal == 9 || tagVal == 10|| tagVal == 11|| tagVal == 12|| 
     tagVal == 13|| tagVal == 14|| tagVal == 15|| tagVal == 16|| tagVal == 17|| tagVal == 18|| 
     tagVal == 19|| tagVal == 20|| tagVal == 21|| tagVal == 22|| tagVal == 23|| tagVal == 24|| 
     tagVal == 25|| tagVal == 26|| tagVal == 27|| tagVal == 28|| tagVal == 29|| tagVal == 30|| 
     tagVal == 31|| tagVal == 32|| tagVal == 33|| tagVal == 34|| tagVal == 35|| tagVal == 36) 
  {
    FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
    delay(50);
    //FTImpl.StopSound();
    if(hundredIntRow[tagVal - 1] == pointsHundred)
    {
      pointsHundred++;
    }
    else
      if(hundredIntRow[tagVal - 1] > pointsHundred)
        error1++;
  }
  FTImpl.DLEnd(); //end the display list
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
}

void hundred_game()
{
  hundredGamemode = 0;
  while(1)
  {
    hundred_display();
    if(backFromGame == 1) 
      return;
  }
}

/* Test Szybkosci - SpeedTest ------------------------------------------------------------------------------------ */
void speedTest_display()
{ 
  char cstr[16];
  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  timePassed = millis() - startTime;
  timePassed = timePassed/1000;
  
  FTImpl.DLStart();
  FTImpl.ColorRGB(0xFF,0,0);
  FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 20, 30, FT_OPT_CENTER, "Test Szybkosci");
  
  FTImpl.ColorRGB(0,0,0);
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;

  tagOption = 0;
  if(90 == tagVal)
    tagOption = FT_OPT_FLAT;

  FTImpl.ColorRGB(0xff,0xff,0xff);
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(90);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
    
  if(gameStarted == 0)
  {    
    tagOption = 0;
    if(11 == tagVal)
      tagOption = FT_OPT_FLAT;
      
    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
    FTImpl.Tag(11);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton),(FT_DISPLAYHEIGHT/4) - (hbutton/2),wbutton*2,hbutton*2,30,tagOption,"START");

    tagOption = 0;
    if(21 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(21);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 50,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"+");

    tagOption = 0;
    if(22 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(22);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 50,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"-");
    
    FTImpl.Tag(0);
    itoa(timeSet, cstr, 10);
    strcat(cstr, "s");
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT/2) + 20, 29, FT_OPT_CENTER, "Ilosc czasu:");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT*3/4), 29, FT_OPT_CENTER, cstr);
  }
  else
  {
    if(pointsGained == 0 && (timeSet - timePassed > 0))
    {
      FTImpl.ColorRGB(0,0xff,0);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2, 31, FT_OPT_CENTER, "START");
    }
    else
    {
      if(timeSet - timePassed > 0)
        dtostrf(timeSet - timePassed, 4, 1, cstr);
      else
        itoa(timeSet, cstr, 10);
      strcat(cstr, "s");
      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/4- 10, 29, FT_OPT_CENTER, "Czas:");
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/4 + 25, 31, FT_OPT_CENTER, cstr);
    
      itoa(pointsGained, cstr, 10);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT*2/4 - 5, 29, FT_OPT_CENTER, "Wynik:");
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT*2/4 + 30, 31, FT_OPT_CENTER, cstr);
  
      itoa(error1, cstr, 10);
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT*3/4 - 5, 29, FT_OPT_CENTER, "Bledy:");
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT*3/4 + 30, 31, FT_OPT_CENTER, cstr);
    }  
  }
  FTImpl.DLEnd();
  FTImpl.Finish();
  if(tagVal == 90 || tagVal == 11) 
  {
    FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
    delay(150);
    FTImpl.StopSound();
    if(tagVal == 90)
      backFromGame = 1;

    if(tagVal == 11)
      gameStarted = 1;
  }
  if(tagVal == 21 || tagVal == 22)
  {
    FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
    delay(75);
    FTImpl.StopSound();
    if(tagVal == 21)
        if(timeSet < 600)
          timeSet++;
        
    if(tagVal == 22)
      if(timeSet > 1)
        timeSet--;
  }
}

void speedTest_game()
{
  char cstr[1];
  for(int i = 0; i < 3; i++)
  {
    FTImpl.DLStart();
    FTImpl.ColorRGB(0xFF,0,0);
    itoa(3 - i, cstr, 10);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2, 31, FT_OPT_CENTER, cstr);
    FTImpl.DLEnd();
    FTImpl.Finish();
    delay(1000);
  }
  startTime = millis();
  
  while(timePassed <= timeSet)
  {
    speedTest_display();
    lightState(0);
    toPress = random(1, 5);
    while(lastPressed == toPress)
      toPress = random(1, 5);
    lastPressed = toPress;
    while(toPress == 1 && timePassed <= timeSet)
    {
      speedTest_display();
      digitalWrite(lightGreen, HIGH);
      buttonState();
      if(pressedGreen == 1) 
        toPress = 0;
      if(pressedRed == 1 || pressedBlue == 1 || pressedYellow == 1) 
        error1++;
      if(backFromGame == 1)
        return;    
    }
    while(toPress == 2 && timePassed <= timeSet)
    {
      speedTest_display();
      digitalWrite(lightRed, HIGH);
      buttonState();
      if(pressedRed == 1)
        toPress = 0;
      if(pressedGreen == 1 || pressedBlue == 1 || pressedYellow == 1) 
        error1++;
      if(backFromGame == 1)
        return;
    }
    while(toPress == 3 && timePassed <= timeSet)
    {
      speedTest_display();
      digitalWrite(lightBlue, HIGH);
      buttonState();
      if(pressedBlue == 1)
        toPress = 0;
      if(pressedGreen == 1 || pressedRed == 1 || pressedYellow == 1) 
        error1++;
      if(backFromGame == 1)
        return;
    }
    while(toPress == 4 && timePassed <= timeSet)
    {
      speedTest_display();
      digitalWrite(lightYellow, HIGH);
      buttonState();
      if(pressedYellow == 1)
        toPress = 0;
      if(pressedGreen == 1 || pressedRed == 1 || pressedBlue == 1) 
        error1++;  
      if(backFromGame == 1)
        return;
    }
    pointsGained++; 
  }
  pointsGained--;
  FTImpl.DLStart();
  FTImpl.ColorRGB(0xff,0,0);
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2, 31, FT_OPT_CENTER, "STOP");
  FTImpl.DLEnd();
  FTImpl.Finish();
  lightState(0);
  delay(1000);
  lightState(1);
  delay(1000);
  lightState(0);
  while(1)
  {
    speedTest_display();
    if(backFromGame == 1)
      return;
  }
}

void speedTest_menu()
{  
  while(1)
  {
    randomSeed(millis());
    lightState(0);
    pointsGained = 0;
    error1 = 0;
    while(1)
    {
      startTime = millis();
      speedTest_display();
      if(gameStarted == 1) 
        break;
      if(backFromGame == 1) 
        return;
    }
    speedTest_game();
    gameStarted = 0;
    backFromGame = 0;
  }
}

/* Test Refleksu - Reflex Test----------------------------------------------------------------------------------- */
void reflexTest_display()
{
  char cstr[16];
  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  uint32_t points[5];
  char StringArray[100];

  backFromGame = 0;
  FTImpl.DLStart();
  FTImpl.ColorRGB(0xff,0,0);
  FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 20, 30, FT_OPT_CENTER, "Test Refleksu");
  FTImpl.ColorRGB(0xFF,0xFF,0xFF);
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;

  tagOption = 0;
  if(90 == tagVal)
    tagOption = FT_OPT_FLAT;
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(90);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
    
  if(gameStarted == 0)
  {
    tagOption = 0;
    if(99 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(99);
    FTImpl.Cmd_Button(10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Test");

    wbutton = FT_DISPLAYWIDTH/4;
    hbutton = FT_DISPLAYHEIGHT/4;

    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);

    tagOption = 0;
    if(11 == tagVal)
      tagOption = FT_OPT_FLAT;
      
    FTImpl.Tag(11);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/8) - (wbutton/2) + 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Przyciski");

    tagOption = 0;
    if(12 == tagVal)
      tagOption = FT_OPT_FLAT;
      
    FTImpl.Tag(12);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*4/8) - (wbutton/2),(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Kolory");

    tagOption = 0;
    if(13 == tagVal)
      tagOption = FT_OPT_FLAT;
      
    FTImpl.Tag(13);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*8/8) - (wbutton) - 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Dzwiek");

    tagOption = 0;
    if(14 == tagVal)
      tagOption = FT_OPT_FLAT;
      
    FTImpl.Tag(14);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - wbutton,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2) + 10,wbutton*2,hbutton/2,27,tagOption,"Przyciski + Kolory");

    tagOption = 0;
    if(15 == tagVal)
      tagOption = FT_OPT_FLAT;
      
    FTImpl.Tag(15);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton*3/4),(FT_DISPLAYHEIGHT) - (FT_DISPLAYHEIGHT/6),wbutton*3/2,hbutton/2,27,tagOption,"Losowe");
  }
  else
  {
    if(leftPressed == 0 || rightPressed == 0)
    {
      FTImpl.ColorRGB(0xff,0xff,0xff);
      FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 50, 30, FT_OPT_CENTER, "Aby rozpoczac wcisnij:");
    
      if(leftPressed == 0 ^ rightPressed == 0)
      {
        if(leftPressed == 0)
          FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 80, 30, FT_OPT_CENTER, "Lewy przycisk");
        if(rightPressed == 0)
          FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 80, 30, FT_OPT_CENTER, "Prawy przycisk");
      }
      else
        FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 80, 30, FT_OPT_CENTER, "Podswietlone przyciski");
    }
    if(reflexButton == 5 || reflexButton == 6 || reflexButton == 7 || reflexButton == 8)
    {
      if(reflexButton == 5)
        FTImpl.Cmd_FGColor(0x00ff00);
      if(reflexButton == 6)
        FTImpl.Cmd_FGColor(0xff0000);
      if(reflexButton == 7)
        FTImpl.Cmd_FGColor(0x0000ff);
      if(reflexButton == 8)
        FTImpl.Cmd_FGColor(0xffff00);
      FTImpl.Tag(00);
      FTImpl.Cmd_Button(FT_DISPLAYWIDTH/2 - 150,FT_DISPLAYHEIGHT/4 - 25,300,75,20,0,"");
    }
    if(timeCurrent == 1)
    {
      if(reflexButton == 5 || reflexButton == 66 || reflexButton == 77 || reflexButton == 8)
        FTImpl.ColorRGB(0,0,0);
      else
        FTImpl.ColorRGB(0xff,0xff,0xff);
      dtostrf((millis() - timeFromStart) / 1000, 4, 3, cstr);
      strcat(cstr, "s");
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/4 + 12, 29, FT_OPT_CENTER, cstr);
    }
    FTImpl.ColorRGB(0xff,0xff,0xff);
    
    if(numberOfResults != 0)
    {
      int x = 1;
      for(int i = 0; i < 3; i++)
      {   
        for(int j = 0; j < 5; j ++)
        {
          if(x < numberOfResults)
          {
            if(resultToAverage[x] == 1)
            {
              char tempResult[8];
              dtostrf(resultInteger[x], 4, 3, tempResult);
              strcat(tempResult, "s");
              FTImpl.Cmd_Text(FT_DISPLAYWIDTH*(1 + 1*j)/6, FT_DISPLAYHEIGHT/2 + 25*i, 26, FT_OPT_CENTER, tempResult);
            }
            else
              FTImpl.Cmd_Text(FT_DISPLAYWIDTH*(1 + 1*j)/6, FT_DISPLAYHEIGHT/2 + 25*i, 26, FT_OPT_CENTER, "XX");
          }
          x++;
        }
      }
    }
       
    if(numberOfResults >= 1)
    {
      int x = 0;
      float resultAverage = 0;
      char resultAverageChar[8];
      for(int i = 0; i < numberOfResults; i++)
        if(resultToAverage[i] == 1)
        {
          x++;
          resultAverage += resultInteger[i];       
        }
      if(x > 0)
        resultAverage = resultAverage / x;
    
      dtostrf(resultAverage, 5, 4, resultAverageChar);
      strcat(resultAverageChar, "s");
      FTImpl.Cmd_Text(95,(FT_DISPLAYHEIGHT) - 35,28, FT_OPT_CENTER, "Srednia:");
      FTImpl.Cmd_Text(175,(FT_DISPLAYHEIGHT) - 35,28, FT_OPT_CENTER, resultAverageChar); 
    }
  }

  FTImpl.DLEnd();
  FTImpl.Finish();
  
  if(tagVal == 90|| tagVal == 11 || tagVal == 12 || tagVal == 13 || tagVal == 14 || tagVal == 15 || tagVal == 99) 
  {
    FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
    delay(150);
    FTImpl.StopSound();
    if(tagVal == 90)
    {
      if(gameStarted == 0)
      {
        lightState(0);
        backFromGame = 1;
      }
      else
      {
        lightState(0);
        gameStarted = 0;
        numberOfResults = 0;
      }
    }

    if(tagVal == 11)
    {
      gameStarted = 1;
      lowRandom = 1;
      highRandom = 5;
    }

    if(tagVal == 12)
    {
      gameStarted = 1;
      lowRandom = 5;
      highRandom = 9;
    }

    if(tagVal == 13)
    {
      gameStarted = 1;
      lowRandom = 9;
      highRandom = 11;
    }

    if(tagVal == 14)
    {
      gameStarted = 1;
      lowRandom = 1;
      highRandom = 9;
    }

    if(tagVal == 15)
    {
      gameStarted = 1;
      lowRandom = 1;
      highRandom = 11;
    }
    if(tagVal == 99)
    {
      delay(100);
      FTImpl.PlaySound(255, FT_MIDI_C3 | FT_SQUAREWAVE);
      digitalWrite(lightGreen, HIGH);
      delay(500);
      FTImpl.StopSound();
      digitalWrite(lightGreen, LOW);
      delay(100);
      FTImpl.PlaySound(255, FT_MIDI_D3 | FT_SQUAREWAVE);
      digitalWrite(lightRed, HIGH);
      delay(500);
      FTImpl.StopSound();
      digitalWrite(lightRed, LOW);
    }
  }
}

void reflexTest_game()
{
  bool resultOk = 0;
  char cstr[16];
  numberOfResults = 0;
  timeCurrent = 0;
  while(1)
  {
    reflexTest_display();
    randomSeed(millis());
    while(gameStarted == 1)
    {
      if(digitalRead(buttonWhiteLeft) == 0)
      {
        digitalWrite(lightWhiteLeft, LOW);
        leftPressed = 1;
      }
      else
      {
        digitalWrite(lightWhiteLeft, HIGH);
        leftPressed = 0;
      }
      
      if(digitalRead(buttonWhiteRight) == 0)
      {
        digitalWrite(lightWhiteRight, LOW);
        rightPressed = 1;
      }
      else
      {
        digitalWrite(lightWhiteRight, HIGH);
        rightPressed = 0;
      }
      reflexTest_display();
      timeRandom = random(timeRandomLow, timeRandomHigh);
      timeMeasured = millis();
      if(leftPressed == 1 && rightPressed == 1)
      {
        if(numberOfResults > 15)
          numberOfResults = 0;
        while((millis() - timeMeasured) <= timeRandom)
        {
          if(digitalRead(buttonWhiteLeft) == 0 && digitalRead(buttonWhiteRight) == 0);
          else
          {
            goto goFalstart;
          }
        }
        reflexButton = random(lowRandom, highRandom);
        timeFromStart = millis();
        if(reflexButton == 1)
          digitalWrite(lightGreen, HIGH);
        if(reflexButton == 2)
          digitalWrite(lightRed, HIGH);
        if(reflexButton == 3)
          digitalWrite(lightBlue, HIGH);
        if(reflexButton == 4)
          digitalWrite(lightYellow, HIGH);
        if(reflexButton == 9)
          FTImpl.PlaySound(255, FT_MIDI_C3 | FT_SQUAREWAVE);
        if(reflexButton == 10)
          FTImpl.PlaySound(255, FT_MIDI_D3 | FT_SQUAREWAVE);
          
        while(1)
        {
          timeCurrent = 1;
          reflexTest_display();
          buttonState();
          if(reflexButton == 1 || reflexButton == 5 || reflexButton == 9)
          {
            if(pressedGreen == 1)
            {
              resultOk = 1;
              goto goResults;
            }
            if(pressedRed == 1 || pressedBlue == 1 || pressedYellow == 1)
            {
              resultOk = 0;
              goto goResults;
            }
          }
          else if(reflexButton == 2 || reflexButton == 6 || reflexButton == 10)
          {
            if(pressedRed == 1)
            {
              resultOk = 1;
              goto goResults;
            }
            if(pressedGreen == 1 || pressedBlue == 1 || pressedYellow == 1)
            {
              resultOk = 0;
              goto goResults;
            }
          }
          else if(reflexButton == 3 || reflexButton == 7)
          {
            if(pressedBlue == 1)
            {
              resultOk = 1;
              goto goResults;
            }
            if(pressedGreen == 1 || pressedRed == 1 || pressedYellow == 1)
            {
              resultOk = 0;
              goto goResults;
            }
          }
          else if(reflexButton == 4 || reflexButton == 8)
          {
            if(pressedYellow == 1)
            {
              resultOk = 1;
              goto goResults;
            }
            if(pressedGreen == 1 || pressedRed == 1 || pressedBlue == 1)
            {
              resultOk = 0;
              goto goResults;
            }
          }
          if(gameStarted == 0)
            goto goFalstart;
        }
        goResults:
        if(resultOk == 1)
        {
          timeMeasured = (millis() - timeFromStart) / 1000;
          if(numberOfResults == 0)
            numberOfResults = 1;
          resultToAverage[numberOfResults] = 1;
          resultInteger[numberOfResults] = timeMeasured; 
        }
        else
        {
          if(numberOfResults == 0)
            numberOfResults = 1;
          resultToAverage[numberOfResults] = 0;
        }
        numberOfResults++;
      }
      goFalstart:
      lightState(0);
      FTImpl.StopSound();
      reflexButton = 0;
      timeCurrent = 0;
    }
    if(backFromGame == 1)
      return;
  }
}

/* Test Pamieci - Simon Says---------------------------------------------------------------------------------------- */
void simonSays_display()
{
  char cstr[16];
  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  FTImpl.DLStart();
  FTImpl.ColorRGB(0xff,0xff,0xff);
  FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), 20, 30, FT_OPT_CENTER, "Test Pamieci");
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;

  tagOption = 0;
  if(90 == tagVal)
    tagOption = FT_OPT_FLAT;
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(90);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
    
  if(gameStarted == 0)
  {
    itoa(difficultyLevel, cstr, 10);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT/2) + 20, 29, FT_OPT_CENTER, "Poziom Trudnosci: ");
    if(difficultyLevel == 10)
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT*3/4), 29, FT_OPT_CENTER, "MAX");
    else
      FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT*3/4), 29, FT_OPT_CENTER, cstr);
  
    tagOption = 0;
    if(11 == tagVal)
      tagOption = FT_OPT_FLAT;

    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
    FTImpl.Tag(11);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton),(FT_DISPLAYHEIGHT/4) - (hbutton/2),wbutton*2,hbutton*2,30,tagOption,"START");

    tagOption = 0;
    if(21 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(21);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 50,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"+");

    tagOption = 0;
    if(22 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Tag(22);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 50,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"-");
  }
  else if(wrongSequence == 1)
  {
    tagOption = 0;
    if(41 == tagVal)
      tagOption = FT_OPT_FLAT;
  
    FTImpl.Cmd_FGColor(0x00ff00);
    FTImpl.ColorRGB(0,0,0);
    FTImpl.Tag(41);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton),(FT_DISPLAYHEIGHT/4) - (hbutton/2),wbutton*2,hbutton*2,30,tagOption,"OK");
    
    itoa(score, cstr, 10);
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT/2) + 20, 29, FT_OPT_CENTER, "Wynik:");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT/2) + 60, 31, FT_OPT_CENTER, cstr);
  }
  else
  {
    itoa(score, cstr, 10);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT/2) + 20, 29, FT_OPT_CENTER, "Wynik:");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, (FT_DISPLAYHEIGHT/2) + 60, 31, FT_OPT_CENTER, cstr);

    tagOption = 0;
    if(31 == tagVal)
      tagOption = FT_OPT_FLAT;

    FTImpl.ColorRGB(0,0,0);
    FTImpl.Cmd_FGColor(0x00ff00);
    
    FTImpl.Tag(31);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton),(FT_DISPLAYHEIGHT*1/4) - (hbutton/2),wbutton*2,hbutton*2,27,tagOption,"Powtorz sekwencje");
  }

  FTImpl.DLEnd();
  FTImpl.Finish();
  if(tagVal == 90 || tagVal == 11 || tagVal == 21 || tagVal == 22 || tagVal == 31 || tagVal == 41) 
  {
    FTImpl.PlaySound(pressVolume, FT_MIDI_D5| FT_XYLOPHONE);
    delay(150);
    FTImpl.StopSound();
    if(tagVal == 90)
    {
      if(gameStarted == 0)
      {
        level = 1;
        velocity = basicVelocity;
        backFromGame = 1;
      }
      else
      {
        if(wrongSequence == 0)
        {
          level = 1;
          wrongSequence = 1;
          velocity = basicVelocity;
          backFromGame = 1;
        }
        else
          tagVal = 41;
      }        
    }
    if(tagVal == 11)
    {
      gameStarted = 1;
      wrongSequence = 0;
      level = 1;
    }
    if(tagVal == 21)
      if(difficultyLevel < 10)
        difficultyLevel++;
    if(tagVal == 22)
      if(difficultyLevel > 1)
        difficultyLevel--;
    if(tagVal == 31)
      simonSays_showSequence();
    if(tagVal == 41)
    {
      score = 0;
      wrongSequence = 0;
      gameStarted = 0;
      level = 1;
    }
  }
}

void simonSays_game()
{
  while(1)
  {
    simonSays_display();
    if (level == 1)
      simonSays_generateSequence(); 
    if ((gameStarted == 1 || level != 1) && wrongSequence != 1)
    {
      score = level - 1;
      simonSays_display();
      delay(500);
      simonSays_showSequence();    
      simonSays_getSequence();
      backFromGame = 0;     
    }
    if(backFromGame == 1) 
      return;
  }
}

void simonSays_showSequence()
{
  lightState(0);
  for (int i = 0; i < level; i++)
  {
    if(sequence[i] == 2)
    {
      FTImpl.PlaySound(pressVolume, FT_MIDI_C3 | FT_SQUAREWAVE);
      digitalWrite(lightGreen, HIGH);
      delay(velocity);
      FTImpl.StopSound();
      digitalWrite(lightGreen, LOW);
      delay(200);
    }
    else if(sequence[i] == 3)
    {
      FTImpl.PlaySound(pressVolume, FT_MIDI_D3 | FT_SQUAREWAVE);
      digitalWrite(lightRed, HIGH);
      delay(velocity);
      FTImpl.StopSound();
      digitalWrite(lightRed, LOW);
      delay(200);
    }
    else if(sequence[i] == 4)
    {
      FTImpl.PlaySound(pressVolume, FT_MIDI_G3 |FT_SQUAREWAVE);
      digitalWrite(lightYellow, HIGH);
      delay(velocity);
      FTImpl.StopSound();
      digitalWrite(lightYellow, LOW);
      delay(200);
    }
    else if(sequence[i] == 5)
    {
      FTImpl.PlaySound(pressVolume, FT_MIDI_E3 |FT_SQUAREWAVE);
      digitalWrite(lightBlue, HIGH);
      delay(velocity);
      FTImpl.StopSound();
      digitalWrite(lightBlue, LOW);
      delay(200);
    }
  }
}

void simonSays_getSequence()
{
  bool flag = 0;
  for (int i = 0; i < level; i++)
  {
    flag = 0;
    while(flag == 0)
    {
      simonSays_display();
      buttonState();
      if (pressedBlue == 1)
      {
        FTImpl.PlaySound(pressVolume, FT_MIDI_E3 |FT_SQUAREWAVE);
        digitalWrite(lightBlue, HIGH);
        your_sequence[i] = 5;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          FTImpl.StopSound();
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightBlue, LOW);
      }

      if(pressedYellow == 1)
      {
        FTImpl.PlaySound(pressVolume, FT_MIDI_G3 |FT_SQUAREWAVE);
        digitalWrite(lightYellow, HIGH);
        your_sequence[i] = 4;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          FTImpl.StopSound();
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightYellow, LOW);
      }

      if(pressedRed == 1)
      {
        FTImpl.PlaySound(pressVolume, FT_MIDI_D3 |FT_SQUAREWAVE);
        digitalWrite(lightRed, HIGH);
        your_sequence[i] = 3;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          FTImpl.StopSound();
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightRed, LOW);
      }

      if(pressedGreen == 1)
      {
        FTImpl.PlaySound(pressVolume, FT_MIDI_C3 |FT_SQUAREWAVE);
        digitalWrite(lightGreen, HIGH);
        your_sequence[i] = 2;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          FTImpl.StopSound();
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightGreen, LOW);
      }
      if(backFromGame == 1) 
        return;
    }
    FTImpl.StopSound();
  }
  simonSays_rightSequence();
}

void simonSays_generateSequence()
{
  randomSeed(millis());
  for (int i = 0; i < MAX_LEVEL; i++)
    sequence[i] = random(2,6);
}

void simonSays_wrongSequence()
{
  for (int i = 0; i < 3; i++)
  {
    lightState(1);
    delay(175);
    lightState(0);
    delay(175);
  }
  level = 1;
  wrongSequence = 1;
  velocity = basicVelocity;
}

void simonSays_rightSequence()
{
  lightState(0);
  delay(250);
  lightState(1);
  delay(250);
  lightState(0);
  delay(250);
  if(level < MAX_LEVEL) level++;
  velocity -= (increaseVelocity + (increaseVelocity*(difficultyLevel - 1))) ;
  if(velocity <= 20) 
    velocity = 20;
}

/* ------------------------------------------------------------------------------- */
bool refreshTimer1(int ms)
{
  float currentMillis = millis();
  if(currentMillis - previousMillis1 > ms)
  {
    previousMillis1 = currentMillis;  
    return true;
  }
  else return false;
}
