#include <stdio.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

/*************************************************************/

#define DEBUG            0    // set to 1/0: it enables/disabels sending the Serial.monitor usefull informations for debuging purposes
#ifdef DEBUG
#endif

//#define LOGIQUE_INVERSE_SIRENE // retirer le commentaire pour une sirene qui declenche meme si fil coupe
#ifndef LOGIQUE_INVERSE_SIRENE
#define SIRENE_OUVERTE HIGH
#define SIRENE_FERMEE  LOW
#else
#define SIRENE_OUVERTE LOW
#define SIRENE_FERMEE  HIGH
#endif

/*************************************************************/

#define BAUD_RATE     9600
#define _VERSION_   "v0.82"   // version of the sketch that is running on the Arduino board.
#define LCD_ID        0x27    // builtin internal identification of the LCD device
#define LCD_COLS        20
#define LCD_ROWS         4

#define buzzer          12
#define siren           11
#define door            13

#define REF_PASSWORD            "1234"  // Default password 
#define ARMED_COUNTDOWN_TIME    3   // the maximum is 9 (seconds) due to LCD display output
#define SIREN_DURATION_TIME     20  // Duration time of the siren in seconds (60 sec really last 55)
#define SIREN_DELAY_TIME        5   // When door opens, time duration to enter password - before triggering the siren
#define NB_DOOR_CLOSED_BEEPS    5   // Number of beeps when exit (countdown)
#define PASS_MAX_LENGTH         4
#define NB_MAX_PASS_ATTEMPTS    3
#define WRONG_PASSWD_DSP_TIME   2
#define NB_MAX_SIREN_CYCLES     5
#define SIREN_INTERVAL_DELAY    4

#define BUZZ_FREQ_COUNTDOWN       700
#define BUZZ_LEN_COUNTDOWN        100
#define BUZZ_FREQ_DOOR_OPENING    3000
#define BUZZ_FREQ_OPTION          1000
#define BUZZ_LEN_OPTION           200
#define BUZZ_FREQ_PASSWORD        5000
#define BUZZ_LEN_PASSWORD          100
#define BUZZ_FREQ_DOORSTATUS      4000
#define BUZZ_LEN_DOORSTATUS        100

#define DSP_COUNTDOWN_MESSAGE1     "L'alarme sera"
#define DSP_COUNTDOWN_MESSAGE2     "active dans"
#define DSP_ALARM_READY            "   *** ALARM ***"
#define DSP_CUR_PASSWD             "Current password"
#define DSP_NEW_PASSWD             "Set the New Password"
#define DSP_WRONG_PASS             "MAUVAIS PASSWORD"
#define DSP_OPTION1                "A - Activer l'alarme"
#define DSP_OPTION2                "B - Changer password"
#define DSP_ALARM_ON               "Alarme  ACTIVE"
#define DSP_ENTER_PASS             "Pass>"

#define ONE_SEC_MILLISEC           1000

byte pos                = 0;
byte nbSirenCycles      = 0;    

boolean standbyMode         = true;
boolean activatingCountdown = false;
boolean armed               = false;
boolean sirenOn             = false;
boolean passChangeMode      = false;

String refPasswd           = REF_PASSWORD; // this is the validation password to match
String typedPasswd         = "";           // this is the password typed in with the keypad
char   keypressed          = char(NULL);

/*****************************************************************************
  SETUP THE KEYPAD
*****************************************************************************/
const byte ROWS = 4; //keypad has four rows
const byte COLS = 4; //keypad has four columns
char keyMap[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; //Pin number for keypad rows
byte colPins[COLS] = {5, 4, 3, 2}; //pin number for keypad columns
Keypad myKeypad = Keypad( makeKeymap(keyMap), rowPins, colPins, ROWS, COLS); 

/*****************************************************************************/

LiquidCrystal_I2C lcd(LCD_ID, LCD_COLS, LCD_ROWS);  // device address, num columns, num lines (16)

/***************************************************************/

void setup() 
{ 
  Serial.begin(BAUD_RATE);
  Serial.println(__FILE__);
  Serial.println(__DATE__);
  
  lcd.init(); 
  lcd.backlight();
  lcd.clear(); 
  
  pinMode(buzzer, OUTPUT);       // Set buzzer as an output
  pinMode(siren,  OUTPUT);       // Sets the siren as an Output
  pinMode(door,   INPUT_PULLUP); // Sets the door as an Input
}

/***************************************************************/

void loop() 
{
  if (activatingCountdown == true ) 
  {
    activatingCountdown = false;
    armed = true;
    displayCountdownScreen(ARMED_COUNTDOWN_TIME);
    doorCloseBuzzer();
  }

  if (armed == true)
  {
    if (digitalRead(door)==LOW) 
    {
      tone(buzzer, BUZZ_FREQ_DOOR_OPENING); // Send 2KHz sound signal
      displayPasswdScreen(DSP_ALARM_READY); 
      enterPassword();
    } 
  }

  if (armed == false) 
  {
    if (standbyMode == true )
    {
      displayStandbyScreen();
      standbyMode = false;
    }
    
    keypressed = myKeypad.getKey();

    if (keypressed =='A')
    {       
      tone(buzzer, BUZZ_FREQ_OPTION, BUZZ_LEN_OPTION);
      activatingCountdown = true;            
    }
    else if (keypressed =='B') 
    {
      pos=0;
      typedPasswd = "";
      passChangeMode = true;
      tone(buzzer, BUZZ_FREQ_OPTION, BUZZ_LEN_OPTION);
      displayPasswdScreen(DSP_CUR_PASSWD);

      while(passChangeMode) 
      {      
        getKbdPasswdChar();

         if (pos > PASS_MAX_LENGTH || keypressed == '#')  
         {
           tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);
           displayPasswdScreen(DSP_CUR_PASSWD);
           typedPasswd = "";
           pos=0;
         }
      
         if ( keypressed == '*') 
         {
           pos=0;
           tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);
           if (refPasswd == typedPasswd) 
           {
             typedPasswd="";
             displayPasswdScreen(DSP_NEW_PASSWD);
                          
             while(passChangeMode) 
             {
               getKbdPasswdChar();
                              
               if (pos > PASS_MAX_LENGTH || keypressed == '#') 
               {
                 typedPasswd = "";
                 pos=0;
				         tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);	
                 displayPasswdScreen(DSP_NEW_PASSWD);
               }
               
               if ( keypressed == '*') 
               {
                 pos=0;
				         tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);
                 refPasswd = typedPasswd;
                 passChangeMode = false;
                 standbyMode = true;
               }            
             }  // end while
           }  // end if this is the right password
        } // end if the keypress is '*'
      } 
    } 
  }  
} 

/***************************************************************/

void enterPassword() 
{
  uint32_t openTime = millis();
  uint32_t temps_ouvert = 0;

  byte msgLen       = strlen(DSP_ENTER_PASS);   
  typedPasswd       = "";
  sirenOn           = true;
  byte nbPassAttempts   = 0;
  byte nbSirenCycles= 0;
  
  while(sirenOn) 
  {
    temps_ouvert = (uint32_t)((millis()-openTime)/1000.0);

    if ( (temps_ouvert > SIREN_DELAY_TIME) || (nbPassAttempts > NB_MAX_PASS_ATTEMPTS))
    {
      Serial.println("La porte est ouverte depuis trop longtemps sans mot de passe ou le nombre d'essai est dépassé ");
      noTone(buzzer);
      //digitalWrite(siren, HIGH);
      digitalWrite(siren, SIRENE_OUVERTE);
    }

    if (temps_ouvert > SIREN_DURATION_TIME)
    {
      Serial.println("La sirene est trop longue et sera arretee ");
      //digitalWrite(siren, LOW);
      digitalWrite(siren, SIRENE_FERMEE);
      pause(SIREN_INTERVAL_DELAY);  // Sert a attendre avec sirene eteinte avant de la repartir un autre cycle 
 //     delay(SIREN_INTERVAL_DELAY*ONE_SEC_MILLISEC); // Sert a attendre avec sirene eteinte avant de la repartir un autre cycle 
      Serial.println(" .....   pause de sirene terminee ");
      nbSirenCycles++; 
      openTime = millis(); // servira  a faire un reset du temps_ouvert pour qu'au prochain cycle la sirenne sonne ...
                           // ....  le temps qu'il faut comme si la porte avait ouvert pour la 1ere fois        
    }

    if (nbSirenCycles > NB_MAX_SIREN_CYCLES-1)
    {
      Serial.print("Sirene arretee car nombre "); Serial.print(nbSirenCycles); Serial.println(" de cycle max atteint");
      sirenOn = false;
      armed = false;
      standbyMode = true;
      typedPasswd = "";
      noTone(buzzer);
      digitalWrite(siren, SIRENE_FERMEE);
    }

    getKbdPasswdChar();
    
    if (pos > PASS_MAX_LENGTH || keypressed == '#') {       
      tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);
      typedPasswd = "";
      pos=0; 
      displayPasswdScreen(DSP_ALARM_READY);
    }//endif
    
    if ( keypressed == '*') 
    {
      Serial.print("Chaine a comparer:>");Serial.print(typedPasswd);Serial.print("< avec >"); Serial.print(refPasswd);Serial.println("<");
      if (typedPasswd == refPasswd )
      {
        Serial.println("Bon password");
        sirenOn = false;
        armed = false;
        noTone(buzzer);
        digitalWrite(siren, SIRENE_FERMEE);
        standbyMode = true;
        typedPasswd = "";
      }//endif
      else
      {
        typedPasswd = "";
        Serial.println("Mauvais password");
        lcd.setCursor(0,1);
        lcd.print(DSP_WRONG_PASS);
        pause(WRONG_PASSWD_DSP_TIME);
        //delay(WRONG_PASSWD_DSP_TIME*ONE_SEC_MILLISEC);
        lcd.setCursor(0,1);
        lcd.print("                    ");
        displayPasswdScreen(DSP_ALARM_READY);
        nbPassAttempts++;
        pos=0;  
      } 

      tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);
      pos=0;
    } 
  } //while
}

/***************************************************************/

void displayStandbyScreen()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(DSP_OPTION1);
  lcd.setCursor(0,1);
  lcd.print(DSP_OPTION2);
  lcd.setCursor(2,3);

  // the following sends to the LCD the version of the sketch + the uploaded date to the board
  lcd.print(__DATE__); lcd.print(" "); lcd.print(_VERSION_);
}
/****************************************************************************************/

void displayCountdownScreen(byte seconds)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(DSP_COUNTDOWN_MESSAGE1);
  lcd.setCursor(0,1);
  lcd.print(DSP_COUNTDOWN_MESSAGE2);
  while (seconds != 0) 
  {
    lcd.setCursor(14,1);
    lcd.print(seconds--);
    tone(buzzer, BUZZ_FREQ_COUNTDOWN, BUZZ_LEN_COUNTDOWN);
    pause(1);  // fonction de delai pour le compte a rebours
// _MR_ TEST DE LA FONCTION PAUSE  delay(ONE_SEC_MILLISEC);
  }
  lcd.clear();
  lcd.setCursor(3,1);
  lcd.print(DSP_ALARM_ON);   
}

/****************************************************************************************/
void displayPasswdScreen(String message)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(message);
  lcd.setCursor(0,1);
  lcd.print(DSP_ENTER_PASS);
}

/****************************************************************************************/
void doorCloseBuzzer()
{
  if (digitalRead(door)==HIGH) // after countdown, it tells if the dore is effectively closed or not
  {
    byte j=0;
    for (j=0; j<NB_DOOR_CLOSED_BEEPS; j++)  
    {
      tone(buzzer, BUZZ_FREQ_DOORSTATUS, BUZZ_LEN_DOORSTATUS);
      delay(BUZZ_LEN_DOORSTATUS*2);
    }
  }
}

/****************************************************************************************/
boolean isNum(char)
{
  return((keypressed == '0' || keypressed == '1' || keypressed == '2' || keypressed == '3' ||
          keypressed == '4' || keypressed == '5' || keypressed == '6' || keypressed == '7' ||
          keypressed == '8' || keypressed == '9' ));
}

/****************************************************************************************/
void getKbdPasswdChar(void)
{
  //static byte i=0;
  byte col = strlen(DSP_ENTER_PASS);
  keypressed = myKeypad.getKey();

  if (keypressed != NO_KEY)
  {
    if (isNum(keypressed))
    {
      if((pos > PASS_MAX_LENGTH-1))
      {
        pos=0;
        lcd.setCursor(col,1);  // it brings back the position of the cursor inside the LCD screen width
        
         //char *buff = (char *) malloc((LCD_COLS - col -1)*sizeof(char));
         //Serial.print("espace de filling si dépassement password>");Serial.print(buff);Serial.println("<");          
        //lcd.print(buff);   //_MR_ set this par more dynamic
        lcd.print("          ");   //_MR_ set this par more dynamic
        lcd.setCursor(col,1);
      }

      typedPasswd += keypressed;
      lcd.setCursor(col+pos,1);
      lcd.print("*");
      tone(buzzer, BUZZ_FREQ_PASSWORD, BUZZ_LEN_PASSWORD);
      Serial.println(col);Serial.println(col+pos);Serial.print(">");Serial.print(typedPasswd);Serial.print("<"); Serial.println();
      //i++; 
      pos++;
    }  
  }
}

/**********************************************************************************************/

uint32_t pause(int seconds)
{
  uint32_t wait_time  = (uint32_t) (seconds*ONE_SEC_MILLISEC);
  uint32_t start_time = millis();

  Serial.print("*** Pause de "); Serial.print(seconds); Serial.println(" secondes ");
  while (millis()-start_time < wait_time) {}
  Serial.println(".... fin de la pause");
}

/**********************************************************************************************/
