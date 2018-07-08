

/* Controller for NEC p404 screen. 
Pointer version**** richard moores 4/2018 - richard @ thelimen dot com 
v0.5
Header is fixed at 7 bytes: byte 0, Start Of Header = 0x01, last 2 bytes give length of message in ascii
Message section bookended with STX, ETX.(0x2, 0x3)
Penultimate byte checksum : xor of byte 2(0x30) through to ETX
EOM = 0xd
*/


//******* NECP404 Command Strings *********//

const char setHDMI1[]=         {
                               0x01,0x30,0x41,0x30,0x45,0x30,0x38,             // header+message length incl. STX & ETX
                               0x02,0x30,0x30,0x36,0x30,0x31,0x31,0x03,        // message incl. STX + ETX
                               0x0b,0x0d                                       // checksum, EOM 
                               };                                                               
const char setHDMI2[]=         {
                               0x01,0x30,0x41,0x30,0x45,0x30,0x38,
                               0x02,0x30,0x30,0x36,0x30,0x31,0x32,0x03,
                               0x08,0x0d
                               };                       
const char setOPT[]=           {
                               0x01,0x30,0x41,0x30,0x45,0x30,0x38,
                               0x02,0x30,0x30,0x36,0x30,0x30,0x44,0x03,
                               0x7f,0x0d
                               };
const char getInput[]=         {
                               0x01,0x30,0x41,0x30,0x43,0x30,0x36,
                               0x02,0x30,0x30,0x36,0x30,0x03,
                               0x03,0x0d
                               };
const char readPowerState[]=  {
                               0x01,0x30,0x41,0x30,0x41,0x30,0x36,
                               0x02,0x30,0x31,0x44,0x36,0x03,
                               0x74,0x0d
                               };
const char powerON[]=          {
                               0x01,0x30,0x41,0x30,0x41,0x30,0x43,
                               0x02,0x43,0x32,0x30,0x33,0x44,0x36,
                               0x30,0x30,0x30,0x31,0x03,
                               0x73,0x0d
                               };
                               
//******* Monitor Replies *********//

const char isHDMI_1[]=        {
                               0x01,0x30,0x30,0x41,0x44,0x31,0x32, // header - 18 byte reply (hex 12,in ascii,bytes 6+7)
                               0x02,0x30,0x30,0x30,0x30,0x36,0x30,
                               0x30,0x30,0x30,0x30,0x38,0x38,
                               0x30,0x30,0x31,0x31,0x03,
                               0x01,0x0d
                               };
                               
const char powerIsOFF[]=       {
                               0x01,0x30,0x30,0x41,0x42,0x31,0x32, // 18 byte reply
                               0x02,0x30,0x32,0x30,0x30,0x44,0xd6, // power mode code
                               0x30,0x30,0x30,0x30,0x30,0x34,      // 4 modes total
                               0x30,0x30,0x30,0x34,0x03,           // mode 4 (off)
                               0x71,0x0d
                               };
                               
const char powerIsON[]=        {
                               0x01,0x30,0x30,0x41,0x42,0x31,0x32, // 18 byte reply
                               0x02,0x30,0x32,0x30,0x30,0x44,0xd6, // power mode code
                               0x30,0x30,0x30,0x30,0x30,0x31,      // 4 modes total
                               0x30,0x30,0x30,0x34,0x03,           // mode 1 (on)
                               0x74,0x0d
                               };
//==================================================
#define EOM 0xd
#define DEBUG 0
                             
char              mBuf[32];                   //input buffer for replies from monitor

const int         buttonPin=2;
const int         ledPin=4;

volatile boolean  bPressed, bLockout;         //set during ISR -  needs to be in RAM
int               ind = 0;                    //an index for reading characters from the serial buffer
unsigned long t = millis();
       
//==================================================

void setup() {
  
pinMode(ledPin, OUTPUT);
digitalWrite(ledPin, LOW);

pinMode(buttonPin, INPUT_PULLUP);
attachInterrupt((digitalPinToInterrupt(buttonPin)),
                                      ISRbuttonPress,
                                      RISING);
bPressed = false;
bLockout = false;


Serial3.begin(9600);                          //serial to RS232 adapter port
if (DEBUG) {
  Serial.begin(9600);
  Serial.println("running setup");
  }
 
}
//==================================================

void loop() {

  if (bPressed){
                detachInterrupt(buttonPin);             //disable the button
                digitalWrite(ledPin, HIGH);
                          
                t = millis();                
                            
                memset(mBuf, 0, sizeof mBuf);
                send_Request(getInput);                 //get the current input                           
                          
                delay(100);
                                                     
                getReply();                            //read monitors reply from buffer
                if (DEBUG) Serial.write(mBuf);
                            
                if (parseReply(isHDMI_1, sizeof (isHDMI_1))){
                                                             if (DEBUG) Serial.println("command matched input");                         
                                                             delay(50);
                                                             send_Request(setOPT);
                                                             delay(100);
                                                             getReply();
                                                                      
                            } else {                          
                                    delay(30);
                                    send_Request(setHDMI1);
                                    delay(100);
                                    getReply();
                            }
                            
                            delay(40);                            
                            send_Request(readPowerState);
                            delay(100);
                            if (Serial3.available()){
                                                     getReply();
                            }
                            
                            switch (parseReply(powerIsON, sizeof powerIsON)) {

                                 case 0:
                                        delay(40);
                                        send_Request(powerON);
                                        delay(100);
                                        getReply();
                                 case 1:
                                        break;

                                 default:
                                 ;
                            }
                                            
  }
 if (millis() - t > 4000 && bLockout) {
                                       attachInterrupt((digitalPinToInterrupt(buttonPin)),
                                       ISRbuttonPress,
                                       RISING);
                                       bLockout = false;
                                       digitalWrite(ledPin, LOW);  
                                       } 
  bPressed = false;
    
}
//==================================================
int parseReply(const char *_pMonitorReply, size_t _size){

          if (memcmp(_pMonitorReply, mBuf, _size) == 0){
                                                         if (DEBUG){
                                                                    Serial.write("command matches state, swapping inputs ");
                                                                    Serial.flush();
                                                                    }
                                                        return 1;
                                                      } else return 0;                   
}
//==================================================

void getReply(){
   
   ind = 0;  
    
   if (Serial3.available()){                               
                            while ( ind < sizeof mBuf ) {                                       
                                                        mBuf[ind++] = Serial3.read();                          
                            }
                                
    if (DEBUG){
               for (int i=0;i<ind;)
               {
               Serial.write(mBuf[i++]);
               }    
               Serial.flush();
    }
   }
}
//==================================================
void send_Request(const char* _pCommandString){
  
  if (Serial3.availableForWrite()){ 
                                   Serial3.write(_pCommandString);
                                   Serial3.flush();
                                
    if (DEBUG){
      Serial.print("Sent:");
      Serial.write(_pCommandString);
      Serial.println();
    }
  }
}
//==================================================

void ISRbuttonPress(){
                               detachInterrupt(buttonPin);
                               PORTD_ISFR = CORE_PIN2_BITMASK;
                               bLockout = true;
                               bPressed = true;  
                                             
}
//==================================================

 
