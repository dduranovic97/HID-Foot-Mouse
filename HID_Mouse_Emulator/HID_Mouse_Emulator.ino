/* * * * * * * * * * * * * * * * * * */
/*          HID Foot Mouse           */
/*        by Daniel Đuranović        */
/*           Kolovoz 2021.           */
/* * * * * * * * * * * * * * * * * * */

#include <USBComposite.h>
#include <EEPROM.h>

/*DEVICE INFO*/
const char ManufacturerName[] = "Dani's Thinkering";
const char DeviceName[] = "HID Foot Mouse";
const char DeviceSerial[] = "00000000000000000001";

/*EEPROM*/
#define EEPROM_ADDR_VERSION_DATE 0x0
#define EEPROM_ADD_START_OF_DATA 0x13 //Data se sprema od ove adrese nadalje

/*Mouse Pin Definition*/
#define upButton PB12
#define downButton PB13
#define leftButton PB11
#define rightButton PB10
#define mouseButton PB14

/*Encoder Definition*/
#define encA PA1
#define encB PA2
#define encSwitch PA0
#define statusLED PA3

int seqA, seqB;
boolean right, left;
boolean encSwitchValue;
boolean  encSwitchOldValue = true;
boolean encSwitchState;

/* Za Mouse speed (0-127)*/
int counter = 1;     //Spremljena u EEPROM(0x14)

/*Milis*/
const long interval = 100;
unsigned long previousMillis; 

/*Button State/time var*/
int upState, downState,leftState, rightState, mouseState = 0;   
const int short_press_time = 100; //WIP                  
unsigned long endUP_time, buildUP_time, hold_time; //WIP

/*HID Definition*/
USBHID HID;
//HIDKeyboard Keyboard(HID);
HIDMouse Mouse(HID); 

/*SETUP*/
void setup() {
  
  /*Boota li se prvi put?*/
  if(first_boot_check()) EEPROM.write(0x14, counter);

  /*Čita iz memorije*/
  counter = EEPROM.read(0x14);

  /*Set Device info*/
  USBComposite.setManufacturerString(ManufacturerName);
  USBComposite.setProductString(DeviceName);
  USBComposite.setSerialString(DeviceSerial);

  /*Begin HID*/
  HID.begin(HID_KEYBOARD_MOUSE);
  while(!USBComposite);

  /*Define pinova*/
  pinMode(upButton, INPUT);
  pinMode(downButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(mouseButton,INPUT);


  /*Define Encoder Pins*/
  pinMode(encSwitch, INPUT_PULLUP);
  pinMode(encA, INPUT_PULLUP);
  pinMode(encB, INPUT_PULLUP);
  pinMode(statusLED, OUTPUT);

  /*Attach Interupt to encoder pins*/
  attachInterrupt(digitalPinToInterrupt(encSwitch), ISR, CHANGE); //ISR-> Definirana interupt fja, CHANGE mode -> svaki put kad pin promjeni stanje
  attachInterrupt(digitalPinToInterrupt(encA), ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encB), ISR, CHANGE);
  
  /*Status variables*/ 
  encSwitchState = false;


  /*Time variables*/
  endUP_time, buildUP_time, hold_time = 0; 
  previousMillis = 0;

  delay(1000); //Da se ne triggera interupt cim se spoji na USB Port
}

/*LOOP*/
void loop() {
  
     unsigned long currentMillis = millis();
 
     if(currentMillis - previousMillis >= interval) {    
    /*State varijable*/
        readState();
        previousMillis = currentMillis;
     }

     /*Click press timing - WIP*/
     if(checkState()) buildUP_time = millis();
     else endUP_time = millis();
          
    /*Racunanje udaljenosti*/ 
    int mouseRange = map(counter, 1, 64, 1, 10);
    int xDistance = (rightState - leftState);
    int yDistance = (downState - upState);

      
    /* poprimi li distance neku vrijednost pomakni mis*/
    if((xDistance != 0) || (yDistance != 0)){    
        Mouse.move(xDistance*mouseRange, yDistance*mouseRange, 0);
       } 
    
    /*detektiranje ako je pritisnuta lijevi klik misa*/
    if(mouseState == HIGH){
        if(!Mouse.isPressed(MOUSE_LEFT)) Mouse.press(MOUSE_LEFT);     
     }

    else{
        if(Mouse.isPressed(MOUSE_LEFT)) Mouse.release(MOUSE_LEFT);   
    }   
    
  
   /*Encoder*/
   if((encSwitchValue == true) && (encSwitchOldValue == false)){
    encSwitchState = 1 - encSwitchState;
   }
   encSwitchOldValue = encSwitchValue;

   if(encSwitchState == HIGH){
     if(right == true){
          right = false; //Sprijecava da se counter poveca u beskonacnost jednim okretom
          counter++; 
          if(counter > 64) counter = 64; 
      }

      else if(left == true){
          left = false; //Sprijecava da se counter smanjuje u beskonacnost jednim okretom
          counter--;
          if(counter < 1) counter = 1;  
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
void ISR(){
  

// Interupt triggeran preko switch-a
  if (!digitalRead(encSwitch)) {
    
    encSwitchValue = true;
    left, right = false;
  }

// Interupt triggeran preko encA ili encB
  else {
    
    // Read A and B signals
    boolean A_val = digitalRead(encA);
    boolean B_val = digitalRead(encB);
    
    // Record the A and B signals in seperate sequences
    seqA <<= 1;
    seqA |= A_val;
    
    seqB <<= 1;
    seqB |= B_val;
    
    // Mask the MSB four bits
    seqA &= 0b00001111;
    seqB &= 0b00001111;
    
    // Compare the recorded sequence with the expected sequence
    if (seqA == 0b00001001 && seqB == 0b00000011) {
      left = true;
      right = false;
      }
     
    else if (seqA == 0b00000011 && seqB == 0b00001001) {
      right = true;
      left = false;
      }

      encSwitchValue = false;  
  }

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

/*Gleda jeli koji taster pritisnut (samo je kompaktniji kod)*/
void readState(){
    upState = digitalRead(upButton); 
    downState = digitalRead(downButton);
    rightState = digitalRead(rightButton);
    leftState = digitalRead(leftButton);
    mouseState = digitalRead(mouseButton);
}

/*Ako je bilo koji state aktivan, vraca true*/
boolean checkState(){
  if((upState == HIGH) || (downState == HIGH) || (leftState == HIGH) || (rightState == HIGH)){ 
    return 1;
  }
  else{ 
    return 0;
  }
}
