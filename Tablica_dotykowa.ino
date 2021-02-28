#include "SPI.h"
#include "Wire.h"

/* Platform specific includes */
#include "FT_VM800P43_50.h"

/*define interrupt register bits*/
#define TAG_INTERRUPT 0x04
#define TOUCH_INTERRUPT 0x02

/* Global object for FT800 Implementation */
FT800IMPL_SPI FTImpl(FT_CS_PIN,FT_PDN_PIN,FT_INT_PIN);

uint8_t touchedTag=0;

/* Definition of the button positions*/
const int buttonYellow = 1;
const int buttonBlue = A5;
const int buttonRed = 2;
const int buttonGreen = A1;
const int buttonWhiteLeft = A2;
const int buttonWhiteRight = 0;

/* Definition of the Lights */
const int lightYellow = 3;
const int lightBlue = 8;
const int lightRed = 5;
const int lightGreen = 6;
const int lightWhiteLeft = 7;
const int lightWhiteRight = A0;

/* Simon Says */
const int MAX_LEVEL = 100;
int sequence[MAX_LEVEL];
int your_sequence[MAX_LEVEL];
int level = 1;
const int basicVelocity = 500;
const int increaseVelocity = 20;
int velocity = basicVelocity;

/* Speed Test */
int LedPressedValue = 0;
unsigned long StartTime = 0;
float TimePassed = 0;
int ToPress = 0;
int Error1 = 0;
int SetTime = 15;
int LastPressed = 0;

int LastOne = 0;
int LastTwo = 0;
int LastThree = 0;
int LastFour = 0;
int LastFive = 0;
int LastSix = 0;

int OnePressed = 0;
int TwoPressed = 0;
int ThreePressed = 0;
int FourPressed = 0;
int FivePressed = 0;
int SixPressed = 0;
/* ----------------------------------------------------------------------------- */

int backFromGame = 0;
int gameStarted = 0;
int hundredGamemode = 0;

int16_t BootupConfigure()
{
	uint32_t chipid = 0;
  FTImpl.Init(FT_DISPLAY_RESOLUTION);//configure the display resolution

  delay(20);//for safer side
  
  /* Identify the chip */
  chipid = FTImpl.Read32(FT_ROM_CHIPID);
  if(FT800_CHIPID != chipid)
  {
    Serial.print("Error in chip id read ");
    Serial.println(chipid,HEX);
    return 1;
  }
  
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
  FTImpl.DLStart(); //start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
  FTImpl.ClearColorRGB(64,64,64); 
  FTImpl.Clear(1,1,1);    
  FTImpl.ColorRGB(0xff, 0xff, 0xff);
  FTImpl.Cmd_Text((FT_DISPLAYWIDTH/2), (FT_DISPLAYHEIGHT/2), 30, FT_OPT_CENTER, "Prosze wcisnac kolejne kropki"); //display string at the center of the screen using inbuilt font handle 29 
  FTImpl.Cmd_Calibrate(0);
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
}

int ButtonState(){
  OnePressed = 0;
  TwoPressed = 0;
  ThreePressed = 0;
  FourPressed = 0;
  FivePressed = 0;
  SixPressed = 0;

  int One = digitalRead(buttonGreen);
  int Two = digitalRead(buttonRed);
  int Three = digitalRead(buttonBlue);
  int Four = digitalRead(buttonYellow);
  int Five = digitalRead(buttonWhiteLeft);
  int Six = digitalRead(buttonWhiteRight);

  if (One != LastOne)
  {
    if (One == HIGH) {} 
    else
      OnePressed = 1;
  }
  LastOne = One;

  if (Two != LastTwo) 
  {
    if (Two == HIGH ) {} 
    else 
      TwoPressed = 1;
  }
  LastTwo = Two;

  if (Three != LastThree)
  {
    if (Three == HIGH) {} 
    else
      ThreePressed = 1;
  }
  LastThree = Three;

  if (Four != LastFour) 
  {
    if (Four == HIGH) {} 
    else
      FourPressed = 1;
  }
  LastFour = Four;

  if (Five != LastFive) 
  {
    if (Five == HIGH) {} 
    else
      FivePressed = 1;
  }
  LastFive = Five;

  if (Six != LastSix) 
  {
    if (Six == HIGH) {} 
    else
      SixPressed = 1;
  }
  LastSix = Six;
}

int32_t Dec2Ascii(char *pSrc,int32_t value)
{
  int16_t Length;
  char *pdst,charval;
  int32_t CurrVal = value,tmpval,i;
  char tmparray[16],idx = 0;//assumed that output string will not exceed 16 characters including null terminated character

  //get the length of the string
  Length = strlen(pSrc);
  pdst = pSrc + Length;
  
  //cross check whether 0 is sent
  if(0 == value)
  {
    *pdst++ = '0';
    *pdst++ = '\0';
    return 0;
  }
  
  //handling of -ve number
  if(CurrVal < 0)
  {
    *pdst++ = '-';
    CurrVal = - CurrVal;
  }
  /* insert the digits */
  while(CurrVal > 0){
    tmpval = CurrVal;
    CurrVal /= 10;
    tmpval = tmpval - CurrVal*10;
    charval = '0' + tmpval;
    tmparray[idx++] = charval;
  }

  //flip the digits for the normal order
  for(i=0;i<idx;i++)
  {
    *pdst++ = tmparray[idx - i - 1];
  }
  *pdst++ = '\0';

  return 0;
}

int main_menu()
{  
  int32_t LoopFlag = 0,wbutton,hbutton,tagVal,tagOption;
  char StringArray[100];
  uint32_t ReadWord;
  int16_t xvalue,yvalue,pendown;
  sTagXY sTagxy;

  wbutton = FT_DISPLAYWIDTH/4;
  hbutton = FT_DISPLAYHEIGHT/4;
  while(1)
  { 
    backFromGame = 0;
    gameStarted = 0;
    digitalWrite(lightYellow, LOW);
    digitalWrite(lightBlue, LOW);
    digitalWrite(lightRed, LOW);
    digitalWrite(lightGreen, LOW);
    digitalWrite(lightWhiteLeft, LOW);
    digitalWrite(lightWhiteRight, LOW);
    /* Read the touch screen xy and tag from GetTagXY API */
    FTImpl.GetTagXY(sTagxy);
    /* Construct a screen shot with grey color as background, check constantly the touch registers,
       form the infromative string for the coordinates of the touch, check for tag */      
    FTImpl.DLStart();
    //FTImpl.ClearColorRGB(64,64,64);
    //FTImpl.Clear(1,1,1);    
    FTImpl.ColorRGB(0xff,0x00,0x00);
    FTImpl.TagMask(0);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 55, FT_DISPLAYHEIGHT/4, 30, FT_OPT_CENTER, "COACH");

    //FTImpl.ClearColorRGB(64,64,64);
    //FTImpl.Clear(1,1,1);    
    FTImpl.ColorRGB(0xff,0xff,0xff);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 55, FT_DISPLAYHEIGHT/4, 30, FT_OPT_CENTER, "MIKESZ");
    
    ReadWord = sTagxy.tag;//or the tag can be directly read from register //ReadWord = FTImpl.Read( REG_TOUCH_TAG);
    Dec2Ascii(StringArray,ReadWord);
    tagVal = ReadWord;

    /* demonstration of tag assignment and based on the tag report from ft800 change the button properties */
    FTImpl.Cmd_FGColor(0x008000);
    FTImpl.TagMask(1);
    tagOption = 0;//no touch is default 3d effect and touch is flat effect
    if(11 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      simonSays_game();
    }

    //assign tag value 12 to the button
    FTImpl.Tag(11);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/8) - (wbutton/2) + 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Simon Says");
    
    tagOption = 0;//no touch is default 3d effect and touch is flat effect
    if(12 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      reflexTest_game();
    }
    //assign tag value 13 to the button
    FTImpl.Tag(12);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*4/8) - (wbutton/2),(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Test Refleksu");

    tagOption = 0;//no touch is default 3d effect and touch is flat effect
    if(13 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      speedTraining_menu();
    }
    //assign tag value 13 to the button
    FTImpl.Tag(13);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH*8/8) - (wbutton) - 10,(FT_DISPLAYHEIGHT*2/4) - (hbutton/2),wbutton,hbutton,27,tagOption,"Test Szybkosci");

    tagOption = 0;//no touch is default 3d effect and touch is flat effect
    if(14 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      hundred_game();
    }

    //assign tag value 12 to the button
    FTImpl.Tag(14);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/8) - (wbutton/2) + 10,(FT_DISPLAYHEIGHT*3/4) - (hbutton/2) + 10,wbutton,hbutton,27,tagOption,"Setka");
    
    FTImpl.DLEnd();
    FTImpl.Finish();  
  }
}

void setup()
{
  Serial.begin(9600);   
	
	if(BootupConfigure())
	{
		//error case - do not do any thing
	}
  else
  {
    FTImpl.EnableInterrupts(1,TAG_INTERRUPT|TOUCH_INTERRUPT);
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

  digitalWrite(lightYellow, LOW);
  digitalWrite(lightBlue, LOW);
  digitalWrite(lightRed, LOW);
  digitalWrite(lightGreen, LOW);
  digitalWrite(lightWhiteLeft, LOW);
  digitalWrite(lightWhiteRight, LOW);
  main_menu();
}

void loop()
{
}

/* ------------------------------------------------------------------------------------ */
void ten_display()
{
  const char Display_string[22] = "Setka";
  int num = level - 1;
  char cstr[16];
  itoa(num, cstr, 10);
  
  FTImpl.DLStart(); //start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
  FTImpl.ColorRGB(0xFF,0xFF,0xFF); //set the color of the string to while color
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, 22, 30, FT_OPT_CENTER, Display_string);

  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;
  
  tagOption = 0;
  if(21 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
    hundredGamemode = 0;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(21);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");

  wbutton = FT_DISPLAYHEIGHT/10;
  hbutton = FT_DISPLAYHEIGHT/10;
  
  tagOption = 0;
  if(22 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(22);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 5,(FT_DISPLAYHEIGHT/2) - (hbutton/2) - 5,wbutton,hbutton,27,tagOption,"10x10");

  tagOption = 0;
  if(23 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(23);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 5,(FT_DISPLAYHEIGHT/2) - (hbutton/2) - 5,wbutton,hbutton,27,tagOption,"9x9");

  tagOption = 0;
  if(24 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(24);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 5,(FT_DISPLAYHEIGHT/2) + (hbutton/2) + 5,wbutton,hbutton,27,tagOption,"8x8");

  tagOption = 0;
  if(25 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(25);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 5,(FT_DISPLAYHEIGHT/2) + (hbutton/2) + 5,wbutton,hbutton,27,tagOption,"7x7");
  
  FTImpl.DLEnd(); //end the display list
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
}

void hundred_display()
{
  hundredGamemode = 0;
  const char Display_string[22] = "Setka";
  int num = level - 1;
  char cstr[16];
  itoa(num, cstr, 10);
  
  FTImpl.DLStart(); //start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
  FTImpl.ColorRGB(0xFF,0xFF,0xFF); //set the color of the string to while color
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, 22, 30, FT_OPT_CENTER, Display_string);

  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;
  
  tagOption = 0;
  if(21 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
    backFromGame = 1;
  }
  else backFromGame = 0;
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(21);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");

  tagOption = 0;
  if(22 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
    hundredGamemode = 10;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(22);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 5,(FT_DISPLAYHEIGHT/2) - (hbutton/2) - 5,wbutton,hbutton,27,tagOption,"10x10");

  tagOption = 0;
  if(23 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(23);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 5,(FT_DISPLAYHEIGHT/2) - (hbutton/2) - 5,wbutton,hbutton,27,tagOption,"9x9");

  tagOption = 0;
  if(24 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(24);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 5,(FT_DISPLAYHEIGHT/2) + (hbutton/2) + 5,wbutton,hbutton,27,tagOption,"8x8");

  tagOption = 0;
  if(25 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
  }
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(25);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 5,(FT_DISPLAYHEIGHT/2) + (hbutton/2) + 5,wbutton,hbutton,27,tagOption,"7x7");
  
  FTImpl.DLEnd(); //end the display list
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
}

void hundred_game()
{
  while(1)
  {
    hundred_display();
    while(hundredGamemode == 10)
    {
      ten_display();
    }
    if(backFromGame == 1) 
      return;
  }
}
/* ------------------------------------------------------------------------------------ */
void reflexTest_display()
{
  const char Display_string[22] = "Test Refleksu";
  int num = level - 1;
  char cstr[16];
  itoa(num, cstr, 10);
  
  FTImpl.DLStart(); //start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
  FTImpl.ColorRGB(0xFF,0xFF,0xFF); //set the color of the string to while color
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2, 29, FT_OPT_CENTER, Display_string);
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 25, FT_DISPLAYHEIGHT/2 + 50, 29, FT_OPT_CENTER, "Wynik: ");
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 25, FT_DISPLAYHEIGHT/2 + 50, 29, FT_OPT_CENTER, cstr);

  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;
  
  tagOption = 0;
  if(21 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
    backFromGame = 1;
  }
  else backFromGame = 0;
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(21);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
  
  FTImpl.DLEnd(); //end the display list
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
}

void reflexTest_game()
{
  return;
}

/*------------------------------------------------------------------------------------- */
void speedTest_display()
{ 
  const char Display_string[15] = "Test Szybkosci";
  char cstr[16];
  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  TimePassed = millis() - StartTime;
  TimePassed = TimePassed/1000;
  
  FTImpl.DLStart();
  FTImpl.ColorRGB(0xFF,0xFF,0xFF);
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;
  
  tagOption = 0;
  if(21 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
    backFromGame = 1;
  }
  else backFromGame = 0;
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(21);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");

  if(gameStarted == 0)
  {
    tagOption = 0;
    if(22 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      gameStarted = 1;
    }
  
    FTImpl.Cmd_FGColor(0x09ff00);
    FTImpl.Tag(22);
    FTImpl.Cmd_Button(10,10,wbutton,hbutton,27,tagOption,"Start");

    tagOption = 0;
    if(23 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      SetTime++;
      delay(50);
    }
  
    FTImpl.Cmd_FGColor(0x09ff00);
    FTImpl.Tag(23);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) - (wbutton) - 50,(FT_DISPLAYHEIGHT*3/4),wbutton,hbutton,27,tagOption,"+");
    
    tagOption = 0;
    if(24 == tagVal)
    {
      tagOption = FT_OPT_FLAT;
      if(SetTime > 0)
        SetTime--;
        delay(50);
    }
  
    FTImpl.Cmd_FGColor(0x09ff00);
    FTImpl.Tag(24);
    FTImpl.Cmd_Button((FT_DISPLAYWIDTH/2) + 50,(FT_DISPLAYHEIGHT*3/4),wbutton,hbutton,27,tagOption,"-");

    itoa(SetTime, cstr, 10);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2, 29, FT_OPT_CENTER, cstr);
  }
  else
  {
    itoa(LedPressedValue, cstr, 10);
    
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2 - 50, 29, FT_OPT_CENTER, Display_string);
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 14, FT_DISPLAYHEIGHT/2, 29, FT_OPT_CENTER, "Wynik: ");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 50, FT_DISPLAYHEIGHT/2, 29, FT_OPT_CENTER, cstr);
  
    itoa(Error1, cstr, 10);
  
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 50, FT_DISPLAYHEIGHT/2 + 25, 29, FT_OPT_CENTER, "Ilosc Bledow: ");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 50, FT_DISPLAYHEIGHT/2 + 25, 29, FT_OPT_CENTER, cstr);
    if(SetTime - TimePassed > 0)
      itoa(SetTime - TimePassed, cstr, 10);
    else
      itoa(0, cstr, 10);
    
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 65, FT_DISPLAYHEIGHT/2 + 50, 29, FT_OPT_CENTER, "Pozostaly Czas: ");
    FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 50, FT_DISPLAYHEIGHT/2 + 50, 29, FT_OPT_CENTER, cstr);
  }  

  FTImpl.DLEnd(); //end the display list
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
  if((tagVal == 21) || (tagVal == 22)) delay (100);
}



void RandomAll()
{
  ToPress = random(1, 5);
}

void speedTraining_game()
{
  while(TimePassed <= SetTime)
  {
    speedTest_display();
    digitalWrite(lightGreen, LOW);
    digitalWrite(lightRed, LOW);
    digitalWrite(lightBlue, LOW);
    digitalWrite(lightYellow, LOW);
    digitalWrite(lightWhiteLeft, LOW);
    digitalWrite(lightWhiteRight, LOW);
    RandomAll();
    while((LastPressed == ToPress) || (ToPress >= 5 ))
    {
      RandomAll();
    }
    LastPressed = ToPress;
    while((ToPress == 1) && (TimePassed <= SetTime))
    {
      speedTest_display();
      digitalWrite(lightGreen, HIGH);
      ButtonState();
      if(OnePressed == 1){LedPressedValue++; ToPress = 0;}
      if(TwoPressed == 1 || ThreePressed == 1 || FourPressed == 1) Error1++;
      if(backFromGame == 1) return;      
    }
    while((ToPress == 2) && (TimePassed <= SetTime))
    {
      speedTest_display();
      digitalWrite(lightRed, HIGH);
      ButtonState();
      if(TwoPressed == 1){LedPressedValue++; ToPress = 0;}    
      if(OnePressed == 1 || ThreePressed == 1 || FourPressed == 1) Error1++;
      if(backFromGame == 1) return;
    }
    while((ToPress == 3) && (TimePassed <= SetTime))
    {
      speedTest_display();
      digitalWrite(lightBlue, HIGH);
      ButtonState();
      if(ThreePressed == 1){LedPressedValue++; ToPress = 0;}    
      if(OnePressed == 1 || TwoPressed == 1 || FourPressed == 1) Error1++;
      if(backFromGame == 1) return;
    }
    while((ToPress == 4) && (TimePassed <= SetTime))
    {
      speedTest_display();
      digitalWrite(lightYellow, HIGH);
      ButtonState();
      if(FourPressed == 1){LedPressedValue++; ToPress = 0;} 
      if(OnePressed == 1 || TwoPressed == 1 || ThreePressed == 1) Error1++;  
      if(backFromGame == 1) return;
    }
  }
  digitalWrite(lightYellow, LOW);
  digitalWrite(lightBlue, LOW);
  digitalWrite(lightRed, LOW);
  digitalWrite(lightGreen, LOW);
  digitalWrite(lightWhiteLeft, LOW);
  digitalWrite(lightWhiteRight, LOW);
  delay(1000);
  digitalWrite(lightYellow, HIGH);
  digitalWrite(lightBlue, HIGH);
  digitalWrite(lightRed, HIGH);
  digitalWrite(lightGreen, HIGH);
  digitalWrite(lightWhiteLeft, HIGH);
  digitalWrite(lightWhiteRight, HIGH);
  delay(1000);
  digitalWrite(lightYellow, LOW);
  digitalWrite(lightBlue, LOW);
  digitalWrite(lightRed, LOW);
  digitalWrite(lightGreen, LOW);
  digitalWrite(lightWhiteLeft, LOW);
  digitalWrite(lightWhiteRight, LOW);
  while(1)
  {
    speedTest_display();
    if(backFromGame == 1) return;
  }
}
void speedTraining_menu()
{  
  while(1)
  {
    randomSeed(millis());
    Error1 = 0;
    while(1)
    {
      StartTime = millis();
      speedTest_display();
      if(gameStarted == 1) 
      {
        gameStarted = 0;
        break;
      }
      if(backFromGame == 1) return;
    }
    speedTraining_game();
  }
}

/*----------------------------------------------------------------------------------------- */
void simonSays_display()
{
  const char Display_string[22] = "Simon Says";
  int num = level - 1;
  char cstr[16];
  itoa(num, cstr, 10);
  
  FTImpl.DLStart(); //start the display list. Note DLStart and DLEnd are helper apis, Cmd_DLStart() and Display() can also be utilized.
  FTImpl.ColorRGB(0xFF,0xFF,0xFF); //set the color of the string to while color
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2, FT_DISPLAYHEIGHT/2, 29, FT_OPT_CENTER, Display_string);
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 - 25, FT_DISPLAYHEIGHT/2 + 50, 29, FT_OPT_CENTER, "Wynik: ");
  FTImpl.Cmd_Text(FT_DISPLAYWIDTH/2 + 25, FT_DISPLAYHEIGHT/2 + 50, 29, FT_OPT_CENTER, cstr);

  int32_t tagVal,wbutton,hbutton,tagOption;
  uint32_t ReadWord;
  char StringArray[100];
  
  wbutton = FT_DISPLAYWIDTH/6;
  hbutton = FT_DISPLAYHEIGHT/6;
  
  sTagXY sTagxy;
  FTImpl.GetTagXY(sTagxy);
  ReadWord = sTagxy.tag;
  Dec2Ascii(StringArray,ReadWord);
  tagVal = ReadWord;
  
  tagOption = 0;
  if(21 == tagVal)
  {
    tagOption = FT_OPT_FLAT;
    backFromGame = 1;
  }
  else backFromGame = 0;
  
  FTImpl.Cmd_FGColor(0xff0000);
  FTImpl.Tag(21);
  FTImpl.Cmd_Button((FT_DISPLAYWIDTH) - (wbutton) - 10,(FT_DISPLAYHEIGHT) - (hbutton) - 10,wbutton,hbutton,27,tagOption,"Powrot");
  
  FTImpl.DLEnd(); //end the display list
  FTImpl.Finish(); //render the display list and wait for the completion of the DL
}

void simonSays_game()
{
  while(1)
  {
    simonSays_display();
    if (level == 1)
    {
      simonSays_generateSequence();//generate a sequence;  
    } 
    if (digitalRead(buttonWhiteLeft) == LOW || level != 1) //If start button is pressed or you're winning
    {
      digitalWrite(lightWhiteLeft, LOW);
      simonSays_showSequence();    //show the sequence
      simonSays_getSequence();     //wait for your sequence
    }
    else digitalWrite(lightWhiteLeft, HIGH);
    if(backFromGame == 1) 
    {
      digitalWrite(lightWhiteLeft, LOW);
      return;
    }
  }
}

void simonSays_showSequence()
{
  digitalWrite(lightGreen, LOW);
  digitalWrite(lightRed, LOW);
  digitalWrite(lightYellow, LOW);
  digitalWrite(lightBlue, LOW);

  for (int i = 0; i < level; i++)
  {
    if(sequence[i] == 2)
    {
      digitalWrite(lightGreen, HIGH);
      delay(velocity);
      digitalWrite(lightGreen, LOW);
      delay(200);
    }
    else if(sequence[i] == 3)
    {
      digitalWrite(lightRed, HIGH);
      delay(velocity);
      digitalWrite(lightRed, LOW);
      delay(200);
    }
    else if(sequence[i] == 4)
    {
      digitalWrite(lightYellow, HIGH);
      delay(velocity);
      digitalWrite(lightYellow, LOW);
      delay(200);
    }
    else if(sequence[i] == 5)
    {
      digitalWrite(lightBlue, HIGH);
      delay(velocity);
      digitalWrite(lightBlue, LOW);
      delay(200);
    }
    
  }
}

void simonSays_getSequence()
{
  int flag = 0; //this flag indicates if the sequence is correct

  for (int i = 0; i < level; i++)
  {
    flag = 0;
    while(flag == 0)
    {
      if (digitalRead(buttonBlue) == LOW)
      {
        digitalWrite(lightBlue, HIGH);
        your_sequence[i] = 5;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightBlue, LOW);
      }

      if(digitalRead(buttonYellow) == LOW)
      {
        digitalWrite(lightYellow, HIGH);
        your_sequence[i] = 4;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightYellow, LOW);
      }

      if(digitalRead(buttonRed) == LOW)
      {
        digitalWrite(lightRed, HIGH);
        your_sequence[i] = 3;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightRed, LOW);
      }

      if(digitalRead(buttonGreen) == LOW)
      {
        digitalWrite(lightGreen, HIGH);
        your_sequence[i] = 2;
        flag = 1;
        delay(200);
        if (your_sequence[i] != sequence[i])
        {
          simonSays_wrongSequence();
          return;
        }
        digitalWrite(lightGreen, LOW);
      }
      simonSays_display();
      if(backFromGame == 1) return;
    }
  }
  simonSays_rightSequence();
}

void simonSays_generateSequence()
{
  randomSeed(millis()); //in this way is really random!!!

  for (int i = 0; i < MAX_LEVEL; i++)
  {
    sequence[i] = random(2,6);
  }
}

void simonSays_wrongSequence()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(lightGreen, HIGH);
    digitalWrite(lightRed, HIGH);
    digitalWrite(lightYellow, HIGH);
    digitalWrite(lightBlue, HIGH);
    delay(250);

    digitalWrite(lightGreen, LOW);
    digitalWrite(lightRed, LOW);
    digitalWrite(lightYellow, LOW);
    digitalWrite(lightBlue, LOW);
    delay(250);
  }
  level = 1;
  velocity = basicVelocity;
}

void simonSays_rightSequence()
{
  digitalWrite(lightGreen, LOW);
  digitalWrite(lightRed, LOW);
  digitalWrite(lightYellow, LOW);
  digitalWrite(lightBlue, LOW);
  delay(250);

  digitalWrite(lightGreen, HIGH);
  digitalWrite(lightRed, HIGH);
  digitalWrite(lightYellow, HIGH);
  digitalWrite(lightBlue, HIGH);
  delay(500);
  
  digitalWrite(lightGreen, LOW);
  digitalWrite(lightRed, LOW);
  digitalWrite(lightYellow, LOW);
  digitalWrite(lightBlue, LOW);
  delay(500);

  if (level < MAX_LEVEL) level++;
    velocity -= increaseVelocity;
}
