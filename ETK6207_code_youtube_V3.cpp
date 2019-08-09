/****************************************************/
/* This is only one example of code structure       */
/* OFFCOURSE this code can be optimized, but        */
/* the idea is let it so simple to be easy catch    */
/* where can do changes and look to the results     */
/****************************************************/

/****************************************************/
/*    Panel of DVD reader LG Model DV392H-N         */
/****************************************************/

//set your clock speed
#define F_CPU 16000000UL
//these are the include files. They are outside the project folder
#include <avr/io.h>
//#include <iom1284p.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <IRremote.h>// Note: This library give-me troubles, 1.0.0 to Arduino Uno, 2.0.1 to 2560??? Not sure!!!

#define VFD_in 7// If 0 write LCD, if 1 read of LCD
#define VFD_clk 8 // if 0 is a command, if 1 is a data0
#define VFD_stb 9 // Must be pulsed to LCD fetch data of bus

#define AdjustPins    PIND // before is C, but I'm use port C to VFC Controle signals

int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

unsigned char DigitTo7SegEncoder(unsigned char digit, unsigned char common);

/*Global Variables Declarations*/
unsigned char day = 7;  // start at 7 because the VFD start the day on the left side and move to rigth... grid is reverse way
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char seconds = 0;
unsigned char milisec = 0;
unsigned char points = 0;

unsigned char secs;

unsigned char digit;
unsigned char number;
unsigned char numberA0;
unsigned char numberA1;
unsigned char numberB0;
unsigned char numberB1;
unsigned char numberC0;
unsigned char numberC1;
unsigned char numberD0;
unsigned char numberD1;
unsigned char numberE0;
unsigned char numberE1;
unsigned char numberF0;
unsigned char numberF1;

unsigned char grid;
unsigned char gridSegments = 0b00000001; // Here I define the number of GRIDs and Segments I'm using
                                         // On this chip (ETK6207) I use 5 grids. See the table on datasheet!

unsigned char wordA = 0;
unsigned char wordB = 0;

unsigned int k=0;
int upLED=-1; // Necessary with signal, because reach value -1

boolean flag=true;
boolean flgLED1 = true;
boolean flgLED2 = true;
boolean flgLED3 = true;
boolean flgLED4 = true;

boolean flgUpDown=true;

unsigned int segments[] ={
  //  Use 2 byte to do a digit of 7 segments, the second byte have the "g", 2 points...  // 
      0b00111111, 0b00000000,//0   
      0b00000110, 0b00000000,//1     
      0b01011011, 0b00001000,//2   
      0b01001111, 0b00001000,//3    
      0b01100110, 0b00001000,//4   
      0b01101101, 0b00001000,//5   
      0b01111101, 0b00001000,//6   
      0b00000111, 0b00000000,//7   
      0b01111111, 0b00001000,//8   
      0b01100111, 0b00001000,//9   
      0b00000000, 0b00000000,//10 Empty display
  };
void clear_VFD(void);
void InfraRed(void);


void etk6207_init(void){
  delayMicroseconds(200); //power_up delay
  // Note: Allways the first byte in the input data after the STB go to LOW is interpret as command!!!

  // Configure VFD display (grids)
  cmd_with_stb(gridSegments); // cmd 1// Command to define the number of grids and segments
  delayMicroseconds(1);
  // Write to memory display, increment address, normal operation
  cmd_with_stb(0b01000000);//(BIN(01000000));
  delayMicroseconds(1);
  // Address 00H - 15H ( total of 11*2Bytes=176 Bits)
  cmd_with_stb(0b11000000);//(BIN(01100110)); 
  delayMicroseconds(1);
  // set DIMM/PWM to value
  cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
  delayMicroseconds(1);
}

void cmd_without_stb(unsigned char a)
{
  // send without stb
  unsigned char transmit = 7; //define our transmit pin
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
   for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_in, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_in, LOW);
     }
    delayMicroseconds(5);
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(5);
   }
   //digitalWrite(VFD_clk, LOW);
}

void cmd_with_stb(unsigned char a)
{
  // send with stb
  unsigned char transmit = 7; //define our transmit pin
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(1);
   for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
     digitalWrite(VFD_clk, LOW);
     delayMicroseconds(1);
     if (data & mask){ // if bitwise AND resolves to true
        digitalWrite(VFD_in, HIGH);
     }
     else{ //if bitwise and resolves to false
       digitalWrite(VFD_in, LOW);
     }
    digitalWrite(VFD_clk, HIGH);
    delayMicroseconds(1);
   }
   digitalWrite(VFD_stb, HIGH);
   delayMicroseconds(1);
}

void test_VFD(void){
  /* 
  Here do a test for all segments of 6 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data. The cycle for do a increment of 3 bytes on 
  the variable "i" on each test cycle of FOR.
  */
  // to test 6 grids is 6*3=18, the 8 gird result in 8*3=24.
 
  clear_VFD();
      
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(gridSegments); // cmd 1 // 4 Grids & 14 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
        
         for (int i = 0; i < 10 ; i=i+2){ // test base to 16 segm and 6 grids
         cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
         cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
         }
    
      //cmd_without_stb(0b00000001); // cmd 1// Command to define the number of grids and segments
      //cmd_with_stb((0b10001000) | 7); //cmd 4
      digitalWrite(VFD_stb, HIGH);
      delay(1);
      delay(200);  
}

void test_Segments_Panel_ETK6207(void){
  clear_VFD();
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(gridSegments); // cmd 1 // 5 Grids & 14 Segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
            for (int n=0; n < 10; n=n+2){
                       for (int i = 0; i < 16 ; i=i+2){ // test base to 16 segm and 6 grids
                          digitalWrite(VFD_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000000 | n); //cmd 3 wich define the start address (00H to 15H)
                            if(i<8){
                              cmd_without_stb(0b00000001 << i); //
                              cmd_without_stb(0b00000000);
                            }
                              else{
                              cmd_without_stb(0b00000000);
                              cmd_without_stb(0b00000001 << (i-8)); //
                              }
                            digitalWrite(VFD_stb, HIGH);
                           cmd_with_stb((0b10001000) | 7); //cmd 4
                          delay(100);
                       }
             }
}

void test_VFD_chkGrids(void){
  /* 
  Here do a test for all segments of 5 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data. The cycle for do a increment of 3 bytes on 
  the variable "i" on each test cycle of FOR.
  */
  // to test 6 grids is 6*3=18, the 8 grid result in 8*3=24.
 
  clear_VFD();
      
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(gridSegments); // cmd 1// Command to define the number of grids and segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
             for (int i = 0; i < 6 ; i++){ // test base to 15 segm and 7 grids
             cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
             cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
             
             }
          digitalWrite(VFD_stb, HIGH);
          delayMicroseconds(1);
      //cmd_without_stb(0b00000001); // cmd 1// Command to define the number of grids and segments
      //cmd_with_stb((0b10001000) | 7); //cmd 4
      cmd_with_stb((0b10001000) | 7); //cmd 4
      
        delay(1);
        delay(100);
}

void test_VFD_grid(void){
  clear_VFD();
      //
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      cmd_with_stb(gridSegments); // cmd 1// Command to define the number of grids and segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      //
      cmd_with_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
      for (int i = 0; i < 12 ; i=i+2){ // test base to 15 segm and 6 grids
        digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
        cmd_without_stb((0b11000000) | i);
      
             cmd_without_stb(segments[i]); // Data to fill table 6*16 = 96 bits
             cmd_without_stb(segments[i+1]); // Data to fill table 6*16 = 96 bits
             digitalWrite(VFD_stb, HIGH);
             cmd_with_stb((0b10001000) | 7); //cmd 4
      
        delay(1);
        delay(70);
        digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
        cmd_without_stb((0b11000000) | i);
             cmd_without_stb(0b00000000); // Data to fill table 6*16 = 96 bits
             cmd_without_stb(0b00000000); // Data to fill table 6*16 = 96 bits
             digitalWrite(VFD_stb, HIGH);
             cmd_with_stb((0b10001000) | 7); //cmd 4
      
        delay(1);
        delay(70);
      }
}

void clear_VFD(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 3 bytes = 36 registers
  */
      for (int n=0; n < 10; n=n+2){  // 5 grids, this means 10 bytes of memory, advance 2 by 2!
        cmd_with_stb(gridSegments); // cmd 1// Command to define the number of grids and segments
        cmd_with_stb(0b01000000); //   cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b11000000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table of 6 grids * 15 segm = 80 bits on the table
            cmd_without_stb(0b00000000);
            //
            //cmd_with_stb((0b10001000) | 7); //cmd 4
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(100);
     }
}

/******************************************************************/
/************************** Update Clock **************************/
/******************************************************************/
void send_update_clock(void)
{
  if (secs >=60){
    secs =0;
    minutes++;
  }
  if (minutes >=60){
    minutes =0;
    hours++;
  }
  if (hours >=24){
    hours =0;
  }
    //*************************************************************
    DigitTo7SegEncoder(secs%10);
    //Serial.println(secs, DEC);
    numberA0=segments[number];
    numberA1=segments[number+1];
    DigitTo7SegEncoder(secs/10);
    //Serial.println(secs, DEC);
    numberB0=segments[number];
    numberB1=segments[number+1];
    SegTo32Bits();
    //*************************************************************
    DigitTo7SegEncoder(minutes%10);
    numberC0=segments[number];
    numberC1=segments[number+1];
    DigitTo7SegEncoder(minutes/10);
    numberD0=segments[number];
    numberD1=segments[number+1];
    SegTo32Bits();
    //**************************************************************
    DigitTo7SegEncoder(hours%10);
    numberE0=segments[number];
    numberE1=segments[number+1];
    DigitTo7SegEncoder(hours/10);
    numberF0=segments[number];
    numberF1=segments[number+1];
    SegTo32Bits();
    //**************************************************************
    /*
     * // Only to debug!!!
    Serial.println(numberA0);
    Serial.println(numberA1);
    Serial.println(numberB0);
    Serial.println(numberB1);
    Serial.println("Tst");
     */
}

void SegTo32Bits(){
  //Serial.println(number,HEX);
  //
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      cmd_with_stb(gridSegments); // cmd 1// Command to define the number of grids and segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(10);
        cmd_without_stb((0b11000000) | 0x00); //cmd 3 wich define the start address (00H to 15H)
          // Here you can adjuste which grid represent the values of clock
          // each grid use 2 bytes of memory registers
          //
          //cmd_without_stb(numberA0);      // seconds unit
          //cmd_without_stb(numberA1);      //
          //cmd_without_stb(numberB0);      // seconds dozens  // dozens
          //cmd_without_stb(numberB1);      //
          
          // Grid 1
          cmd_without_stb(numberA0);     // Hours dozens
          cmd_without_stb(numberA1);     // Only to used with digits drawed by 2 bytes.
          
          // Grid 2
          cmd_without_stb(numberB0);      // Hours Units
          cmd_without_stb(numberB1);      // Only to used with digits drawed by 2 bytes.
          
          // Grid 3
              if(flag==false){
              cmd_without_stb(0b00000000); // space between digits (2 points???) 
              cmd_without_stb(0b00001000); // I'm use the score, but you can use the dot or 2 dot's
              }
              else{
              cmd_without_stb(0b00000000);  // space between digits (2 points???)
              cmd_without_stb(0b00000000);
              }      
          
          //Grid 4
          cmd_without_stb(numberC0);      // Minutes  dozens
          cmd_without_stb(numberC1);      // Only to used with digits drawed by 2 bytes.
          
          // Grid 5
          cmd_without_stb(numberD0);      // Minutes Units
          cmd_without_stb(numberD1);      // Only to used with digits drawed by 2 bytes.

      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delayMicroseconds(1);
}
/***************************** This identify the array[] position to the number ****************************/
void DigitTo7SegEncoder( unsigned char digit)
{
  switch(digit)
  {
    //Is important the number here, because if only use a byte, use from 0~9, case use 2 bytes use from 0~18!!!!
    case 0:   number=0;     break;  // if remove the LongX, need put here the segments[x]
    case 1:   number=2;     break;
    case 2:   number=4;     break;
    case 3:   number=6;     break;
    case 4:   number=8;     break;
    case 5:   number=10;     break;
    case 6:   number=12;     break;
    case 7:   number=14;     break;
    case 8:   number=16;     break;
    case 9:   number=18;     break;
  }
} 
/************************************** Depending of Buttons to adjust time ******************************/
void adjustHMS(){
 // Important is necessary put a pull-up resistor to the VCC(+5VDC) to this pins (3, 4, 5)
 // if dont want adjust of the time comment the call of function on the loop
  /* Reset Seconds to 00 Pin number 3 Switch to GND*/
    if((AdjustPins & 0x08) == 0 )
    {
      _delay_ms(200);
      secs=00;
    }
    
    /* Set Minutes when SegCntrl Pin 4 Switch is Pressed*/
    if((AdjustPins & 0x10) == 0 )
    {
      _delay_ms(200);
      if(minutes < 59)
      minutes++;
      else
      minutes = 0;
    }
    /* Set Hours when SegCntrl Pin 5 Switch is Pressed*/
    if((AdjustPins & 0x20) == 0 )
    {
      _delay_ms(200);
      if(hours < 23)
      hours++;
      else
      hours = 0;
    }
}
/********************************* Send 7 segments Only to test ********************************************/
void send7segm(){
  // This block is very important, it explain how solve the difference 
  // between segments from grids and segments wich start 0 or 1.
  
      cmd_with_stb(gridSegments); // cmd 1// Command to define the number of grids and segments
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
        // This block need to take the shift to left on the second "Display" of the Grid
        // because the firt digit of 7 segm start at 1, the second digit of the same grid
        // start at 2.
          cmd_without_stb(segments[k]);     // seconds unit
          cmd_without_stb(segments[k]);     // seconds dozens
          cmd_without_stb(segments[k]);     // minutes units
          cmd_without_stb(segments[k]);     // minutes dozens
          cmd_without_stb(segments[k]);     // hours units
          cmd_without_stb(segments[k]);     // hours dozens
          cmd_without_stb(segments[k]);     // hours third digit not used
          cmd_without_stb(segments[k]);     // hours dozens
          cmd_without_stb(segments[k]);     // hours third digit not used
          cmd_without_stb(segments[k]);     // hours dozens
          //cmd_without_stb(segments[k]);   // hours third digit not used
          //cmd_without_stb(segments[k]);   // hours dozens
      digitalWrite(VFD_stb, HIGH);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(1);
      delay(200);  
}
/*************************** Read Buttons **************************************/
void readButtons(){
//Take special attention to the initialize digital pin LED_BUILTIN as an output.
//
int ledPin = 13;   // LED connected to digital pin 13, this is only to monitorize the buttons read process!
int inPin = 7;     // pushbutton connected to digital pin 7
int val = 0;       // variable to store the read value
int dataIn=0;

byte array[8] = {0,0,0,0,0,0,0,0};
byte together = 0;

unsigned char receive = 7; //define our transmit pin
unsigned char data = 0; //value to transmit, binary 10101010
unsigned char mask = 1; //our bitmask

array[0] = 1;

unsigned char btn1 = 0x41;

      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(2);
      cmd_without_stb(0b01000010); // cmd 2 //Read Keys;Normal operation; Set pulse as 1/16
      // cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
      // send without stb
  
  pinMode(7, INPUT);  // Important this point! Here I'm changing the direction of the pin to INPUT data.
  delayMicroseconds(2);
  //PORTD != B01010100; // this will set only the pins you want and leave the rest alone at
  //their current value (0 or 1), be careful setting an input pin though as you may turn 
  //on or off the pull up resistor  
  //This don't send the strobe signal, to be used in burst data send
         for (int z = 0; z < 5; z++){
             //for (mask=00000001; mask > 0; mask <<= 1) { //iterate through bit mask
                   for (int h =8; h > 0; h--) {
                      digitalWrite(VFD_clk, HIGH);  // Remember wich the read data happen when the clk go from LOW to HIGH! Reverse from write data to out.
                      delayMicroseconds(2);
                     val = digitalRead(inPin);
                      //digitalWrite(ledPin, val);    // sets the LED to the button's value
                           if (val & mask){ // if bitwise AND resolves to true
                             //Serial.print(val);
                            //data =data | (1 << mask);
                            array[h] = 1;
                           }
                           else{ //if bitwise and resolves to false
                            //Serial.print(val);
                           // data = data | (1 << mask);
                           array[h] = 0;
                           }
                    digitalWrite(VFD_clk, LOW);
                    delayMicroseconds(2);
                   } 
             
              Serial.print(z);  // All the lines of print is only used to debug, comment it, please!
              Serial.print(" - " );
                        
                                  for (int bits = 7 ; bits > -1; bits--) {
                                      Serial.print(array[bits]);
                                   }
                        
                        if (z==0){
                          if(array[7] == 1){
                           hours++;
                          }
                        }
                          if (z==0){
                            if(array[4] == 1){
                             hours--;
                            }
                          }
                          if (z==0){
                            if(array[6] == 1){
                             minutes++;
                            }
                        }
                        if (z==0){
                          if(array[3] == 1){
                           minutes--;
                          }
                        }
                        if (z==1){
                          if(array[3] == 1){
                           hours = 0;
                           minutes = 0;
                           secs = 0;
                          }
                        }
                         
                        if (z==2){
                            if(array[4] == 1){
                             
                          }
                        }
                        
                        if (z==3){
                           if(array[7] == 1){
                            
                            }
                          }                        
                  Serial.println();
          }  // End of "for" of "z"
      Serial.println();  // This line is only used to debug, please comment it!

 digitalWrite(VFD_stb, HIGH);
 delayMicroseconds(2);
 cmd_with_stb((0b10001000) | 7); //cmd 4
 delayMicroseconds(2);
 pinMode(7, OUTPUT);  // Important this point!  // Important this point! Here I'm changing the direction of the pin to OUTPUT data.
 delay(1); 
}
/*************************************** Active ports of Barr LED's ******************************/
 void bar(int s, boolean flgUpDown){
  Serial.print("s: ");
  Serial.println(s, DEC);
          if (flgUpDown==true){  // The flag define if is Up or Down and wich switch will go be used!!!
                    switch (s){
                        case 0: PORTD |= ((1 << PIND2)); break; // On LED connecte PORTD pin 2 //Moviment Up
                        case 1: PORTD |= ((1 << PIND3)); break;
                        case 2: PORTD |= ((1 << PIND4)); break;
                        case 3: PORTD |= ((1 << PIND5)); break;
                        case 4: PORTD |= ((1 << PIND6)); break;
                        case 5: PORTC |= ((1 << PINC0)); break;
                        case 6: PORTC |= ((1 << PINC1)); break;
                        case 7: PORTC |= ((1 << PINC2)); break;
                        case 8: PORTC |= ((1 << PINC3)); break;
                        case 9: PORTC |= ((1 << PINC4)); break;
                    }
          }
          else{
                        switch (s){
                        case 0: PORTD &= (~(1 << PIND2)); break; // Off LED connected PORTD pin 2 //Moviment Down
                        case 1: PORTD &= (~(1 << PIND3)); break;
                        case 2: PORTD &= (~(1 << PIND4)); break;
                        case 3: PORTD &= (~(1 << PIND5)); break;
                        case 4: PORTD &= (~(1 << PIND6)); break;
                        case 5: PORTC &= (~(1 << PINC0)); break;
                        case 6: PORTC &= (~(1 << PINC1)); break;
                        case 7: PORTC &= (~(1 << PINC2)); break;
                        case 8: PORTC &= (~(1 << PINC3)); break;
                        case 9: PORTC &= (~(1 << PINC4)); break;
                      }
             }
 }
/********************************* Decision of code IR received *******************************/
void InfraRed(void){
  // IR zone
  // Note 1: Case use a different protocol of IR, active the "Serial Monitor" to see the number 
  // you receive from your command and adapte the code to this values!
  // Note 2: This is one way to do this without changing the pins of: (data in: 7), (clock: 8) and (Strobe: 9)
  // Of course the code could be simpler here using another pin range!
      //"results" is used as pointer( *results )
         
            if (irrecv.decode(&results)) {
              Serial.println(results.value, HEX);
              irrecv.resume(); // Receive the next value
              }
        
              switch(results.value){
              //0
              case 0x54C: digitalWrite(2, HIGH); break;
              case 0xD4C: digitalWrite(2, LOW); break;
              //1
              case 0x543: digitalWrite(3, HIGH); break;
              case 0xD43: digitalWrite(3, LOW); break;
              //2
              case 0x544: digitalWrite(4, HIGH); break;
              case 0xD44: digitalWrite(4, LOW); break;
              //3
              case 0x545: digitalWrite(5, HIGH); break;
              case 0xD45: digitalWrite(5, LOW); break;
              //4
              case 0x546: digitalWrite(6, HIGH); break;
              case 0xD46: digitalWrite(6, LOW); break;
              //5
              case 0x547: digitalWrite(14, HIGH); break;
              case 0xD47: digitalWrite(14, LOW); break;
              //6
              case 0x548: digitalWrite(15, HIGH); break;
              case 0xD48: digitalWrite(15, LOW); break;
              //7
              case 0x549: digitalWrite(16, HIGH); break;
              case 0xD49: digitalWrite(16, LOW); break;
              //8
              case 0x54A: digitalWrite(17, HIGH); break;
              case 0xD4A: digitalWrite(17, LOW); break;
              //9
              case 0x54B: digitalWrite(18, HIGH); break;
              case 0xD4B: digitalWrite(18, LOW); break;
 
              // Here I change the pins D2~D6 pin by pin to avoid collision with pins D7. Please define other pins to Din, Clk, Stb if necessary!
              // I let two options to switch the pin status to OFF, one with pin change, other with a cycle FOR
              // Also is possible and more easy, apply a shift to the first 5 bits(PortD), and other shift to the second group of 5 bits(PortC)
              //
              //case 0x0554: PORTC=B00000000; PORTD &= (~(1 << PIND2) & ~(1 << PIND3) & ~(1 << PIND4) & ~(1 << PIND5) & ~(1 << PIND6)); break;
              //case 0x0D54: PORTC=B00000000; PORTD &= (~(1 << PIND2) & ~(1 << PIND3) & ~(1 << PIND4) & ~(1 << PIND5) & ~(1 << PIND6)); break;
              case 0x0554:
              // turn all the LEDs off:
                  for (int thisPin = 2; thisPin < 7; thisPin++) {
                   digitalWrite(thisPin, LOW);
                  }  
                  for (int thisPin = 14; thisPin < 19; thisPin++) {
                   digitalWrite(thisPin, LOW);
                  } upLED=-1;break;
              case 0x0D54:
              // turn all the LEDs off:
                  for (int thisPin = 2; thisPin < 7; thisPin++) {
                   digitalWrite(thisPin, LOW);
                  }  
                  for (int thisPin = 14; thisPin < 19; thisPin++) {
                   digitalWrite(thisPin, LOW);
                  }upLED=-1; break; 

              case 0x54D: flgUpDown=true; upLED++; bar(upLED, flgUpDown);  if(upLED >9){upLED=9;}   break;
              case 0xD4D: flgUpDown=true; upLED++; bar(upLED, flgUpDown);  if(upLED >9){upLED=9;}  break;

              case 0x551: flgUpDown=false;  if(upLED >-1){ bar(upLED, flgUpDown);} else{if(upLED <0){upLED=0;}} upLED--;   break;
              case 0xD51: flgUpDown=false;  if(upLED >-1){ bar(upLED, flgUpDown);} else{if(upLED <0){upLED=0;}} upLED--;   break;
              }
              results.value=0x00000000;
              Serial.println(upLED, DEC);  //Please, comment the Print lines, they was only to debug!            
}
/****************************************** Setup of Arduino Uno ****************************************/
void setup() {
// put your setup code here, to run once:

// initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

// Initialize of IR reception
// In case the interrupt driver crashes on setup, give a clue
// to the user what's going on.
  Serial.println("Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Enabled IRin");
 
  seconds = 0x00;
  minutes =0x00;
  hours = 0x00;

  /*CS12  CS11 CS10 DESCRIPTION
  0        0     0  Timer/Counter1 Disabled 
  0        0     1  No Prescaling
  0        1     0  Clock / 8
  0        1     1  Clock / 64
  1        0     0  Clock / 256
  1        0     1  Clock / 1024
  1        1     0  External clock source on T1 pin, Clock on Falling edge
  1        1     1  External clock source on T1 pin, Clock on rising edge
 */
  // initialize timer1 
  cli();           // disable all interrupts
  // initialize timer1 
  //noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;// This initialisations is very important, to have sure the trigger take place!!!
  TCNT1  = 0;
  // Use 62499 to generate a cycle of 1 sex 2 X 0.5 Secs (16MHz / (2*256*(1+62449) = 0.5
  OCR1A = 62498;            // compare match register 16MHz/256/2Hz
  //OCR1A = 500; // only to use in test, increment seconds to fast!
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= ((1 << CS12) | (0 << CS11) | (0 << CS10));    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
 
// Note: this counts is done to a Arduino 1 with Atmega 328... Is possible you need adjust
// a little the value 62499 upper or lower if the clock have a delay or adavnce on hours.

CLKPR=(0x80);

//Notes as set the PORT's
//DDRD is the direction register for Port D (Arduino digital pins 0-7). 
//DDRD = B11111110;  // sets Arduino pins 1 to 7 as outputs, pin 0 as input
//DDRD = DDRD | B11111100;  // this is safer as it sets pins 2 to 7 as outputs
                            // without changing the value of pins 0 & 1, which are RX & TX
// IMPORTANT: from pin 0 to 7 is port D, from pin 8 to 13 is port B
                             
//PORTD is the register for the state of the outputs. For example                            
//PORTD = B10101000; // sets digital pins 7,5,3 HIGH                           
//PORTD != B01010100; // this will set only the pins you want and leave the rest alone at
//their current value (0 or 1), be careful setting an input pin though as you may turn 
//on or off the pull up resistor  
DDRD = DDRD | B11111100;  //Sets pins 2 to 7 as outputs without changing the value of pins 0 & 1, which are RX & TX
PORTD !=0x40;
DDRB =B11110111; // Pin 11 to input, receive the IR signal
PORTB =0x00;  // Don't active Pull Up resistor's
DDRC = 0xFF;
PORTC = 0x00;

etk6207_init();

//only here I active the enable of interrupts to allow run the test of VFD
//interrupts();             // enable all interrupts
sei();
}
/******************************************** RUN ************************************************/
void loop() {
// You can comment untill while cycle to avoid the test running.
clear_VFD();
test_Segments_Panel_ETK6207();
//
test_VFD();
clear_VFD();
//
// Can use this cycle to teste all segments of VFD
       for(int h=0; h < 10; h++){
       k=h;
       send7segm();
       delay(60);
       }
//
  clear_VFD();
//
      while(1){
        send_update_clock();
        delay(100);
        readButtons();
        delay(100);
        InfraRed();
        delay(100);
      }
}
/*************************************** Interrupt ***********************************************/
ISR(TIMER1_COMPA_vect)   {  //This is the interrupt request
                            // https://sites.google.com/site/qeewiki/books/avr-guide/timers-on-the-atmega328
      secs++;
      flag = !flag; 
} 
