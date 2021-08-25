/* * * * * * * * * * * * * * * * * * */
/*        Emulator miša V1.2.2       */
/*        by Daniel Đuranović        */
/*           siječanj 2021.          */
/* * * * * * * * * * * * * * * * * * */

#include <USBComposite.h>
#include <EEPROM.h>

/*DEVICE INFO*/
const char ManufacturerName[] = "Homemade";
const char DeviceName[] = "HID Foot Mouse";
const char DeviceSerial[] = "00000000000000000001";

/*EEPROM*/
#define EEPROM_ADDR_VERSION_DATE 0x0
#define EEPROM_ADD_START_OF_DATA 0x13 //Data se sprema od ove adrese nadalje

/*Mouse*/
#define upButton PB12
#define downButton PB13
#define leftButton PB14
#define rightButton PB15
#define mouseButton PA8

/*Encoder*/
#define encA PA11
#define encB PA12
#define encSwitch PA15
#define statusLED PC13

boolean goingUp, goingDown = false;
boolean encSwitchValue, encSwitchOldValue = true;
boolean encSwitchState = false;

/* Za Mouse speed (0-127)*/
int counter = 0;     //Spremljena u EEPROM(0x14)

/*Milis*/
const long interval = 50;
unsigned long previousMillis = 0; 

/*Button State/time var*/
int upState, downState,leftState, rightState, mouseState = 0;   
const int short_press_time = 100;                        
unsigned long endUP_time, buildUP_time, hold_time;

/*HID Definiranje*/
USBHID HID;
//HIDKeyboard Keyboard(HID);
HIDMouse Mouse(HID); 

void setup() {
  /*Boota li se prvi put?*/
  if(first_boot_check()) EEPROM.write(0x14, counter);

  /*Čita iz memorije*/
  counter = EEPROM.read(0x14);

  /*Set Device info*/
  USBComposite.setManufacturerString(ManufacturerName);
  USBComposite.setProductString(DeviceName);
  USBComposite.setSerialString(DeviceSerial);
  
  HID.begin(HID_KEYBOARD_MOUSE);
  while(!USBComposite);

  /*Define pinova*/
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(mouseButton,INPUT);
  pinMode(encA, INPUT);
  pinMode(encB, INPUT);
  pinMode(encSwitch, INPUT); 
  pinMode(statusLED, OUTPUT);

  /*Pinu A encodera dodjeljenja interupt (ISR)*/
  attachInterrupt(digitalPinToInterrupt(encA), isr_enc_decode, FALLING); //Falling edge (HIGH -> LOW)
  
  endUP_time, buildUP_time, hold_time = 0; 
}

void loop() {
     unsigned long currentMillis = millis();
 
     if(currentMillis - previousMillis >= interval) {    
    /*State varijable*/
        readState();
        previousMillis = currentMillis;
     }
     if(checkState()) buildUP_time = millis();
     else endUP_time = millis();
     
    /*Racunanje udaljenosti*/ 
    int mouseRange = counter; //map(counter, 0, 16, 1, 16);
    int xDistance = (rightState - leftState);
    int yDistance = (downState - upState);
      
    /* poprimi li distance neku vrijednost pomakni mis*/
    if((xDistance != 0) || (yDistance != 0)){    
       hold_time = buildUP_time - endUP_time;     //koliko dugo traje pritisak?
       if(hold_time <= short_press_time ) Mouse.move(xDistance*3, yDistance*3, 0); //Kratak click pomakne mis za 3 piksela
       else Mouse.move(xDistance*mouseRange, yDistance*mouseRange, 0);
    }
    /*detektiranje ako je pritisnuta tipka misa*/
    if(mouseState == HIGH){
        if(!Mouse.isPressed(MOUSE_LEFT)) Mouse.press(MOUSE_LEFT);     
     }

    else{
        if(Mouse.isPressed(MOUSE_LEFT)) Mouse.release(MOUSE_LEFT);   
    }   
    
  
   /*Encoder*/
   encSwitchValue = digitalRead(encSwitch);
   if((encSwitchValue == LOW) && (encSwitchOldValue == HIGH)) encSwitchState = 1 - encSwitchState;
   encSwitchOldValue = encSwitchValue;

   /*Obrnuta logika (inace LOW)*/
   if(encSwitchState == HIGH){
      while(goingUp == 1){
          goingUp = 0;
          counter++;
          if(counter > 16) counter = 16;  
      }

      while(goingDown == 1){
          goingDown = 0;
          counter--;
          if(counter < 0) counter = 0;
      }
      
  digitalWrite(statusLED, HIGH);
  }
  
  else{
     digitalWrite(statusLED, LOW);
     EEPROM.write(0x14, counter);
  }
    
     
/*End of loop*/
} 



/*Encoder interupt (ISR) fja*/
void isr_enc_decode(){
  if(digitalRead(encA) == digitalRead(encB)) goingUp = 1;     //Smjer kazaljke na satu
  else goingDown = 1;                                         //Obrnuto od kazaljke na satu
}

/*Provjera prvoog bootanja*/
/*ovaj dio preuzet sa: http://www.nihamkin.com/how-to-detect-first-boot-after-burning-program-to-flash.html */
boolean first_boot_check(){
    const String version_date = __DATE__ __TIME__;
    uint16_t len = version_date.length();
    boolean is_ipl = false;

    for(unsigned int i = 0; i < len; i++) {
        int addr = EEPROM_ADDR_VERSION_DATE + i;

        if(EEPROM.read(addr) != version_date[i]) {
           EEPROM.write(addr, version_date[i]);
           is_ipl = true;
        }
    }
    return is_ipl;
}

void readState(){
    upState = digitalRead(upButton);
    downState = digitalRead(downButton);
    rightState = digitalRead(rightButton);
    leftState = digitalRead(leftButton);
    mouseState = digitalRead(mouseButton);
}

boolean checkState(){
  if((upState == HIGH) || (downState == HIGH) || (leftState == HIGH) || (rightState == HIGH)) return 1;
  else return 0;
}
