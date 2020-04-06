

  /*--------------------------------|
   * This code will enable you to   |
   * determine maximum power output |
   * of an audio power amplifier    |
   * using any arduino board. Here  |
   * I am using arduino pro mini,   |
   * PD8554 or commonly known as    |
   * nokia 5110 lcd, resistor & cap.|
   *                                |
   *   TX  1         - RAW          |
   *   RX  0         - GND          |
   *   GND -         - RST          |    10k
   *   GND -         - VCC          | --/\/\/\--|
   *   N/A 2        A3 N/A          |           |
   *   RST 3        A2 N/A          |           |   47u             200k
   *   CE  4        A1 AUDIO INPUT  |-----------|---||---|----|----/\/\/\-----<input
   *   DC  5        A0 LCD button   |           |        |    |
   *   DIN 6        13 N/A          |           |        |    |
   *   CLK 7        12 N/A          |           /        /    |
   *   N/A 8        11 N/A          |           \        \    |
   *   N/A 9        10 N/A          |     10k   /   4.7k /    = 224
   *       A4 A5 A6 A7              |           \        \    |          |----<gnd
   *                                |           |        |    |          |
   * -------------------------------|          gnd      gnd  gnd        gnd
   */
  // this is a free software and can be used at ur own risk, Cris Villa
  //***************Libraries*****************
  #include <LiquidCrystal.h>


  // lcd device devlaration
  //lcd(rs, en, d4, d5, d6, d7)
  // RS = 8
  // EN = 9
  // D4 = 4
  // D5 = 5
  // D6 = 6
  // D7 = 7
  LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // designated pins for LCD

  //define some values for lcd menu
  #define btnRIGHT  0
  #define btnUP     1
  #define btnDOWN   2
  #define btnLEFT   3
  #define btnSELECT 4
  #define btnNONE   5
  #define baklytpin 10

  // custom char
  char tmp[16];
  byte char1[8] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18};
  byte char2[8] = {0x1B,0x1B,0x1B,0x1B,0x1B,0x1B,0x1B,0x1B};
  byte char3[8] = {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03};
  char *menu[] = {"Main Page","Settings","Device Info"};
  char *set_menu[] = {"Load Impedance","LCD backlight","Exit"};
  char c = ' ';
  byte lcd_menu=0;
  bool buttonWasReleased=false,adjustment=false,display_init=false;
  int main_menu=0, selecteditem=0, cursor = 0;


  // ***************some variables********************
  int setvalue[] = {4,50};
  int maxvalue[] = {8,250};
  int minvalue[] = {2,0};
  long timer = 0;
  int half_sec, last_sec, sec, lcd_timer;
  int scope[80];
  float out_vol, last_out_vol, last_out_cur, out_cur, Rload, max_pow=0.0;
  float vp,pp;
  
  // *********Read buttons*********
  int read_buttons(){
    int adc_key_in = analogRead(A0);  // read analog value from lcd shield button
    if(adc_key_in > 1000) return btnNONE;
    
    // for version V1.1 use these threshold
    if(adc_key_in < 50) return btnRIGHT;    // 50
    if (adc_key_in < 250) return btnUP;     // 250
    if (adc_key_in < 450) return btnDOWN;   // 450
    if (adc_key_in < 650) return btnLEFT;   // 650
    if (adc_key_in < 850) return btnSELECT; // 850

    // For V1.0 comment the other threshold and use the one below:
    /*
     if (adc_key_in < 50) return btnRIGHT;
     if (adc_key_in < 195) return btnUP;
     if (adc_key_in < 380) return btnDOWN;
     if (adc_key_in < 555) return btnLEFT;
     if (adc_key_in < 790) return btnSELECT;
    */
     return btnNONE; // when all others fail, return this.
  }

  
  void setup() {
    Serial.begin(9600);         // serial monitor used to change parameters
    // Initialize device
    lcd.begin(16, 2);           // initialize LCD
    lcd.createChar(0,char1);    // our custom characters
    lcd.createChar(1,char2);
    lcd.createChar(2,char3);
    lcd.setCursor(0, 0);
    lcd.print("P.A. Power Meter");
    lcd.setCursor(0, 1);
    lcd.print("-By: Cris Villa-");
    pinMode(baklytpin,OUTPUT);
    pinMode(A0,INPUT);
    pinMode(A1,INPUT);
    analogWrite(baklytpin,setvalue[1]);
    delay(1000);
  }

  void loop() {
  // main code here
  if(millis()-timer >=250){   // check for timing 1000 = 1 second, half sec = 500, quarter of a sec = 250
    half_sec++;               // every (half/quarter)seconds increment ur timer
    timer = millis();         // initialize ur timer equals to arduino timing
  }
  if(half_sec>=120) half_sec=0;
  get_serial();
  get_sample();
  lcd_display();
   if(last_sec != half_sec){
     last_sec = half_sec;
   }
  }   // loop ends here

  void get_sample(){
    static float max_data=0, min_data=2000, total_diff=0;
    max_data=0;                                             // need to initialized data every cycle(it seems that my static declaration didn't work)
    min_data=2000;
    total_diff=0;
    for(int i=0;i<79;i++){                                  // sampling input signal, 80 samples taken
      // read analog signal
      scope[i] = analogRead(A1);                            // read one sample at a time
      out_vol = out_vol + scope[i];                         // just add it up(totalizer)
      max_data = max(max_data,scope[i]);                    // store maximum value
      min_data = min(min_data,scope[i]);                    // store minimum value
      delay(1);                                             // rest for a while
    }
    out_vol = out_vol / 80;                                 // average of total samples
    total_diff = (max_data - min_data) * 216 / 1023;        // analog to digital conversion, "216vmax" is the ratio of resistors, this is
                                                            // a convertion of analog to digital and is mapped to 1023(the actual ratio was 217, removed 1 for accuracy issue)
 
    vp = total_diff / 2.0;                         // try to get Vout peak
    pp = (vp * vp) / (2.0 * setvalue[0]);         // why 2? look for Electronics and ckt theories by Boleystad chapter 16
    max_pow = max(max_pow,pp);                         // all theories and example were presented there. Also u may watch here: "https://www.youtube.com/watch?v=7Svlaaz8lrk&t"
  }  // sample reading ends here

  void get_serial(){
    while(Serial.available()){                      // if serial data available
      switch(Serial.read()){                        // with this u have access to change
        case '4':setvalue[0] = 4;                // load impedance to desired value
               Serial.print("Load impedance = ");
               Serial.print(setvalue[0]);
               Serial.println(" ohms");
               break;
        case '8':setvalue[0] = 8;
               Serial.print("Load impedance = ");
               Serial.print(setvalue[0]);
               Serial.println(" ohms");
               break;
      }
    }  // en of serial com
  }

  void lcd_display(){

    lcd_menu = read_buttons();
    switch(lcd_menu){
      case btnUP:
          if(buttonWasReleased){
            buttonWasReleased=false;
            switch(main_menu){
              case 0:
                 main_menu = 1;
                 selecteditem = 0;
                 adjustment = false;
                 break;
              case 1:
                 selecteditem--;
                 cursor--;
                 if(cursor < 0) cursor = 0;
                 if(selecteditem < 0){
                  selecteditem = 2;
                  cursor = 1;
                 }
                 break;
              case 3:
                 if(adjustment){
                  if(setvalue[selecteditem]<maxvalue[selecteditem]){
                    if(selecteditem == 0) setvalue[selecteditem]+=2;
                    else setvalue[selecteditem]+=25;
                  }
                  else setvalue[selecteditem]=maxvalue[selecteditem];
                 }
                 else{
                  selecteditem--;
                  cursor--;
                  if(cursor < 0) cursor=0;
                  if(selecteditem < 0){
                    selecteditem = 2;
                    cursor = 1;
                  }
                 }
                 break;
            }  // switch main_menu ends here
          }  // buttonWasReleased ends here
          break;
      case btnDOWN:
          if(buttonWasReleased){
            buttonWasReleased=false;
            switch(main_menu){
              case 0:
                 main_menu = 1;
                 selecteditem = 0;
                 adjustment = false;
                 break;
              case 1:
                 selecteditem++;
                 cursor++;
                 if(cursor > 1) cursor = 1;
                 if(selecteditem > 2){
                  selecteditem = 0;
                  cursor = 0;
                 }
                 break;
              case 3:
                 if(adjustment){
                  if(setvalue[selecteditem]>minvalue[selecteditem]){
                    if(selecteditem == 0) setvalue[selecteditem]-=2;
                    else setvalue[selecteditem]-=25;
                  }
                  else setvalue[selecteditem]=minvalue[selecteditem];
                 }
                 else{
                  selecteditem++;
                  cursor++;
                  if(cursor > 1) cursor = 1;
                  if(selecteditem > 2){
                    selecteditem = 0;
                    cursor = 0;
                  }
                 }
                 break;
            }  // switch main_menu ends here
          }
          break;
      case btnSELECT:
          if(buttonWasReleased){
            buttonWasReleased=false;
            switch(main_menu){
              case 0:
                 main_menu = 1;
                 selecteditem = 0;
                 adjustment = false;
                 break;
              case 1:
                 switch(selecteditem){
                  case 0:
                      main_menu = 0;
                      selecteditem = 0;
                      break;
                  case 1:
                      main_menu = 3;
                      selecteditem = 0;
                      cursor = 0;
                      break;
                  case 2:
                      main_menu = 2;
                      break;
                 }
                 break;
              case 2:
                 main_menu = 1;
                 break;
              case 3:
                 if(selecteditem == 2){
                  main_menu = 1;
                  selecteditem = 1;
                 }
                 else adjustment = !adjustment;
                 break;
            }  // switch main_menu ends here
          }
          break;
      case btnNONE:
          if(!buttonWasReleased){
            buttonWasReleased=true;
            display_init=false;
            lcd.clear();
            lcd_timer = 0;
          }
          switch(main_menu){
            case 0:
                if(!display_init){
                  display_init=true;
                }
                else{
                  if(last_sec != half_sec){
                    print_txt(dtostrf(pp,6,1,tmp),0,0);
                    lcd.print("W");
                    print_txt(dtostrf(max_pow,6,1,tmp),9,0);
                    lcd.print("W");
                    print_txt(dtostrf(vp,5,1,tmp),0,1);
                    lcd.print("V");
                    lcd.setCursor(6,1);
                    static double a = 0, b = 0, c = 0, d = 0;
                    d = map(vp,0,116,0,20);
                    a = d / 2;
                    if(a>=1){
                      for(int i=1;i<a;i++){
                        lcd.write(byte (1));
                        b = i;
                      }
                      c = a - b;
                    }
                    if(c == 1) lcd.write(byte (1));
                    else lcd.write(byte (0));
                    for(int i=b;i<18;i++){
                      lcd.print(" ");
                    }
                  }
                }
                break;
            case 1:
                if(!display_init){
                  for(int i=0;i<2;i++){
                    print_txt(menu[selecteditem-cursor+i],1,i);
                  }
                  print_txt(">",0,cursor);
                  display_init = true;
                }
                break;
            case 2:
                if(!display_init){
                  print_txt("P.A. POWER METER",0,0);
                  print_txt("Rev. 1.0",3,1);
                  display_init = true;
                }
                break;
            case 3:
                if(!display_init){
                  if(adjustment){
                    print_txt(set_menu[selecteditem],0,0);
                    print_txt(dtostrf(setvalue[selecteditem],3,0,tmp),13,0);
                    lcd.setCursor(0,1);
                    static double a = 0, b = 0, c = 0, d = 0;
                    d = map(setvalue[selecteditem],minvalue[selecteditem],maxvalue[selecteditem],0,32);
                    a = d / 2;
                    if(a>=1){
                      for(int i=1;i<a;i++){
                        lcd.write(byte (1));
                        b = i;
                      }
                      c = a - b;
                    }
                    if(c == 1) lcd.write(byte (1));
                    else lcd.write(byte (0));
                    for(int i=b;i<18;i++){
                      lcd.print(" ");
                    }
                    if(selecteditem == 1) analogWrite(baklytpin,setvalue[1]);
                  }
                  else{
                    for(int i=0;i<2;i++){
                      print_txt(set_menu[selecteditem-cursor+i],1,i);
                    }
                    print_txt(">",0,cursor);
                  }
                  display_init = true;
                }
                break;
          }  // switch main_menu ends here
          if(main_menu!=0||selecteditem!=0||adjustment){
            if(lcd_timer<60){
              if(last_sec != half_sec) lcd_timer++;
            }
            else{
              lcd_timer = 0;
              main_menu = 0;
              selecteditem = 0;
              cursor = 0;
              lcd.clear();
            }
          }
          break;  //btnNONE
    }  // lcd_menu ends here
  }

  void print_txt(char temp[16], int x, int y){ // char to display, x,y location
    lcd.setCursor(x,y);
    lcd.print(temp);
  }
