
#define GASPORTOPEN HIGH
#define GASPORTCLOSED LOW


int toggle=0;
int tally = 0;
int pin = 0; // this is for the pin number corresponding to LEDs
String num = "0123456789"; //this is used to change sting input into int

char command[32]="";
 
void toggleall(void){
 
  for (int i = 2; i <=11; i++) { // 
    digitalWrite(i,HIGH);
    delay(100);
  }
  for (int i = 11; i >=2; i--) { // 
    digitalWrite(i,LOW);
    delay(100);

  }
  
  
 
}//end toggle all 
 
 
void setup() {
  // start serial port at 9600 bps:
  Serial.begin(9600);
 // Serial.println("work already");
 
  for (int i = 2; i <=11; i++) { // for-loop to make pins 2-11 output pins
    pinMode(i, OUTPUT);   

  }
  
  for (int i=0; i < 3; i++) toggleall();
  
}

void loop() {
  

 
  int setpin=0;
 
 
  if (Serial.available() >= 32) {
      
        Serial.readBytes(command,32);
     //   Serial.print(command);
       // Serial.println(":from recv ");
        
        if (command[0] == 7 && command[1] == 7 && command[2] == 7 && command[3] == 7) {
          bool pinnum = true;
          bool pincomplete=false;
          for (int i=4; i < 29; i++){
              if (pinnum){
                setpin = (int)command[i];
                pinnum = false;
              } else {
                if (command[i] == 1) digitalWrite(setpin+1,LOW); 
                if (command[i] == 100) digitalWrite(setpin+1,HIGH);
                pinnum=true;
              }//else if value
          }//end for each byte
        }//end if good command toggleall();
        
       
  }//end while serial avail
  delay(100);
 // Serial.print("end of loop:received=");
 // Serial.println(command);
}//end loop





