// PIC12F683 Configuration Bit Settings

// 複数のLEDを使ったイルミネーション
//点滅モード、点滅速度を、赤外線リモコンで変更
//                                      @2021.12.14

// CONFIG
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select bit (MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown Out Detect (BOR enabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)

#include <xc.h>
#define _XTAL_FREQ 8000000
#define IRsig GP5
#define MAX 50
#define TIME0INIT 0xC2   //0x9C
#define LED_OFF 0b00000111
#define LED_ON  0b00000000
unsigned char rcv_data[4];
unsigned char RecieveControll(void);
unsigned char IRrecieve(void);
unsigned char getLeaderCode(void);
unsigned char getDataCode(void);
unsigned char existNextInput(void);
unsigned char writeEEPROM(void);
unsigned char getByteCode(unsigned char);
//void lightLED(int);
void lightLED2(int);
unsigned char GPstate;
unsigned char pulseCnt,stepCnt;
unsigned char mode;
unsigned char mode0array[6]={4,0,2,0,1,0};
unsigned char mode1array[14]={5,0,4,0,6,0,7,0,2,0,3,0,1,0};
unsigned char mode2array[12]={4,6,7,0,2,3,7,0,1,5,7,0};
unsigned char MODE0LENGTH=6;
unsigned char MODE1LENGTH=14;
unsigned char MODE2LENGTH=12;
unsigned char gp4_flag;

unsigned char *p_array[3];
unsigned char *length[3];
unsigned char modeCnt;
unsigned char interval;

//受信処理
unsigned char IRrecieve(void){
    //リーダーコード受信処理
    if(!getLeaderCode()){
        //異常終了では、受信確認用のLEDを３回点滅
        lightLED2(1);
        //falseを返して抜ける
        return 0;
    }
    //データコード受信処理
    if(!getDataCode()){
        lightLED2(2);
        return 0;
    }
    //受光終了を確認
    if(!existNextInput()){
        lightLED2(3);
        return 0;
    }
    //EEPROM書き込み
    /*
    __delay_ms(50);
    if(!writeEEPROM()){
        lightLED2(9);
        return 0;
    }
     */ 
    //正常終了で、trueを返す
    return 1;
}
//リーダーコード受信処理
unsigned char getLeaderCode(){
    while(IRsig);
    TMR1L=0;
    TMR1H=0;
    TMR1ON=1;
    
    while(!IRsig);
    TMR1ON=0;
    
    if((TMR1H>=0x3E)&&(TMR1H<=0x4E)){
        return 1;
    }else{
        return 0;
    }
}
//データコード受信処理。データは４バイト。
unsigned char  getDataCode(){
    for(unsigned char i=0;i<4;i++){
        if(!getByteCode(i)){
            return 0;
        }
    }
    return 1;
}
//受光終了を確認
unsigned char existNextInput(void){
   // while(RA0);  //ストップビットの確認はしない。    
    while(!IRsig);
    TMR1L=0;
    TMR1H=0;
    TMR1ON=1;
    
    while(IRsig){
        if(TMR1H>0x10){
            TMR1ON=0;
            return 1;
        }
    }
    TMR1ON=0;
    return 0;
}

//データビットを１バイト分受信し、配列[index]に格納
unsigned char getByteCode(unsigned char index){
    rcv_data[index]=0;
    while(IRsig);
    for(int i=0;i<8;i++){
        while(!IRsig);
        TMR1L=0;
        TMR1H=0;
        TMR1ON=1;
        while(IRsig);
        TMR1ON=0;
        if(TMR1H>=0x08){
            rcv_data[index] |=(0b00000001<<i);
        }
    }
    return 1;
}
//動作デバッグ用のLED点灯処理
/*
void lightLED(int n){
    GPstate=LED_OFF;
    GPIO=GPstate;
    for(int i=0;i<n;i++){
        __delay_ms(500);
        GPstate=LED_ON;
        GPIO=GPstate;
        __delay_ms(500);
        GPstate=LED_OFF;
        GPIO=GPstate;     
    }
}
*/
//動作デバッグ用のLED点灯処理
void lightLED2(int n){
    GPstate=LED_OFF;
    unsigned char GREEN=0b0000010;
    GPIO=GPstate;
    for(int i=0;i<n;i++){
        __delay_ms(500);
        GPstate=GREEN;
        GPIO=GPstate;
        __delay_ms(500);
        GPstate=LED_OFF;
        GPIO=GPstate;     
    }
}

void changeMode(){
    unsigned char code=rcv_data[2];
    if(code==0x02){
        mode++;
        if(mode>=3){
            mode=0;
        }
    }else if(code==0x04){   //+ ボタン
        if(interval<201){  //最大250
            interval+=MAX;
        }
    }else if(code==0x0C){   //- ボタン
        if(interval>MAX){  //最小50
            interval-=MAX;
        }
    }
}
void changeStep(void){
    stepCnt++;
    if(stepCnt>= *length[mode]){
        stepCnt=0;
    }
    GPstate =LED_OFF;
    unsigned char v=*(p_array[mode]+stepCnt);
    GPstate &= (~v);
    //gp4_flag = 1-gp4_flag;
    if(gp4_flag==1){
        GPstate |= 0b00010000; //GP4=1
        gp4_flag=0;
    }else{
        GPstate &= 0b11101111; //GP4=0
        gp4_flag=1;
    }
}
//割り込み処理
void __interrupt() isr(void){
    if(T0IF){   //タイマー割り込み
        pulseCnt++;
        if(pulseCnt>=interval){
            changeStep();
            GPIO=GPstate;
            pulseCnt=0;
        }          
     
        TMR0=TIME0INIT;
        T0IF=0;
        return;
    }else if(GPIF){ //状態変化割り込み
        GIE=0;      //割り込み停止
        if(IRrecieve()){        //
            changeMode();       //リモコンボタンの押下の処理
        }else{
            //lightLED(5);
        }
        GPIF=0;
        GIE=1;      //割り込み再開
    }
}
void init(void){
    OSCCON=0b01110000;
    TRISIO=0b00100000;  //GP5 入力
    ANSEL=0;            //全てデジタル
    INTCON=0b00101000;
    IOC=0b00100000;
    OPTION_REG=0b00000111;
    
    GPstate=LED_OFF;    //LED点灯用ビットデータ
    GPIO=GPstate;
    pulseCnt=0;
    stepCnt=0;
    mode=0;
    interval=MAX;
    p_array[0]=mode0array;
    p_array[1]=mode1array;
    p_array[2]=mode2array;
    length[0]=&MODE0LENGTH;
    length[1]=&MODE1LENGTH;
    length[2]=&MODE2LENGTH;
    gp4_flag=0;

    TMR0=TIME0INIT;
}
void main(void){
    init();
    GIE=1;
    while (1){       
    }
}
