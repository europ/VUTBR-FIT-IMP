// Author: Adrián Tóth
// Login:  xtotha01
// Date:   12.12.2017

//#################################################################################################
//#################################################################################################
//#################################################################################################

#include "MK60D10.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

//#################################################################################################
//#################################################################################################
//#################################################################################################

#define PRINT_TSR() { \
    seconds_tmp = RTC_TSR; \
    sprintf(buffer, "TSR = %d", seconds_tmp); \
    SendStr(buffer); \
    SendStr("\r\n"); \
    time_convert(&seconds_tmp, buffer); \
    SendStr("TSR = "); \
    SendStr(buffer); \
    SendStr("\r\n"); \
}

#define PRINT_BUFF() { \
    SendStr("BUFFER = "); \
    SendStr(buffer); \
    SendStr("\r\n"); \
}

#define DEBUG() { \
    SendStr("================================================\r\n"); \
    sprintf(buffer, "SONG   = %d", song); \
    SendStr(buffer); \
    SendStr("\r\n"); \
    sprintf(buffer, "LIGHT  = %d", light); \
    SendStr(buffer); \
    SendStr("\r\n"); \
    sprintf(buffer, "DELAY  = %d", delay); \
    SendStr(buffer); \
    SendStr("\r\n"); \
    sprintf(buffer, "REPEAT = %d", repeat_count); \
    SendStr(buffer); \
    SendStr("\r\n"); \
    SendStr("================================================\r\n"); \
}

#define PRINT_NEWLINE() { \
    SendStr("\r\n"); \
}

#define PRINT(msg) { \
    SendStr(msg); \
    SendStr("\r\n"); \
}

#define TIME_LOAD_ERROR(msg) { \
    SendStr(msg); \
    SendStr("\r\n"); \
    return false; \
}

#define BUFFER_SIZE 101

#define LED_D9  0x20 // Port B, bit 5
#define LED_D10 0x10 // Port B, bit 4
#define LED_D11 0x8  // Port B, bit 3
#define LED_D12 0x4  // Port B, bit 2

//#################################################################################################
//#################################################################################################
//#################################################################################################

enum State {
    INITIALIZATION,  // 1
    SELECTION_MUSIC, // 2
    SELECTION_LIGHT, // 3
    REPEAT,          // 4
    DELAY,           // 5
    SET_ALARM,       // 6
    ACTIVE           // 7
};

char buffer[BUFFER_SIZE];
unsigned int seconds_init;
unsigned int seconds_alarm;
unsigned int seconds_tmp;

int song;
int light;
int repeat_count;
int delay;

//#################################################################################################
//#################################################################################################
//#################################################################################################

// Delay
void Delay(unsigned long long int bound) {
    for(unsigned long long int i=0; i<bound; i++);
}

// Beeper
void Beep() {
    for (unsigned int q=0; q<500; q++) {
        PTA->PDOR = GPIO_PDOR_PDO(0x0010);
        Delay(500);
        PTA->PDOR = GPIO_PDOR_PDO(0x0000);
        Delay(500);
    }
}

// Send character
void SendCh(char c) {
    while( !(UART5->S1 & UART_S1_TDRE_MASK) && !(UART5->S1 & UART_S1_TC_MASK) );
    UART5->D = c;
}

// Send string
void SendStr(char* s) {
    unsigned int i = 0;
    while (s[i] != '\0') {
        SendCh(s[i++]);
    }
}

// Receive character
unsigned char ReceiveCh() {
    while( !(UART5->S1 & UART_S1_RDRF_MASK) );
    return UART5->D;
}

// Receive string
void ReceiveStr() {

    unsigned char c;
    unsigned int i = 0;

    for (unsigned int i=0; i<BUFFER_SIZE; i++) {
        buffer[i]='\0';
    }

    while (i < (BUFFER_SIZE - 1) ) { // receive 100 chars

        c = ReceiveCh();
        SendCh(c);

        if (c == '\r') {
            break;
        }

        buffer[i] = c;
        i++;
    }

    buffer[i] = '\0'; // char 101 is \0

    PRINT_NEWLINE();
}

// Convert time FROM unsigned int TO char
void time_convert(unsigned int* src, char* dest) {
    time_t tmp = *src;
    struct tm ts = *localtime(&tmp);
    for (unsigned int i=0; i<BUFFER_SIZE; i++) {
        dest[i]='\0';
    }
    strftime(dest, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", &ts);
}

// Convert time FROM char TO unsigned int
bool time_load(char* src, unsigned int* dest) {
    int retval;
    struct tm t;

    retval = strlen(src);
    if (retval < 14 || 19 < retval) TIME_LOAD_ERROR("Wrong length!");

    retval = sscanf(src, "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon , &t.tm_mday, &t.tm_hour, &t.tm_min , &t.tm_sec);
    if (retval != 6) TIME_LOAD_ERROR("Wrong format!");

    // date YYYY-MM-DD
    if (t.tm_year < 1970 || 2038 < t.tm_year) TIME_LOAD_ERROR("Wrong year!");
    if (t.tm_mon  < 1    || 12   < t.tm_mon ) TIME_LOAD_ERROR("Wrong month!");

    // JAN,MAR,MAY,JUL,AUG,OCT,DEC
    if (t.tm_mon==1 || t.tm_mon==3 || t.tm_mon==5 || t.tm_mon==7 || t.tm_mon==8 || t.tm_mon==10 || t.tm_mon==12) {
        if (t.tm_mday < 1 || 31 < t.tm_mday) TIME_LOAD_ERROR("Wrong day!");
    }
    // APR,JUN,SEP,NOV
    if (t.tm_mon==4 || t.tm_mon==6 || t.tm_mon==9 || t.tm_mon==11) {
        if (t.tm_mday < 1 || 30 < t.tm_mday) TIME_LOAD_ERROR("Wrong day!");
    }
    // FEB
    if (t.tm_mon==2) {
        if (t.tm_mday < 1 || 29 < t.tm_mday) TIME_LOAD_ERROR("Wrong day!");
    }

    // time HH:MM:SS
    if (t.tm_hour < 0    || 23   < t.tm_hour) TIME_LOAD_ERROR("Wrong hour!");
    if (t.tm_min  < 0    || 59   < t.tm_min ) TIME_LOAD_ERROR("Wrong minute!");
    if (t.tm_sec  < 0    || 59   < t.tm_sec ) TIME_LOAD_ERROR("Wrong second!");

    t.tm_year  = t.tm_year - 1900;
    t.tm_mon   = t.tm_mon - 1;
    t.tm_isdst = -1;

    time_t tmp;
    tmp = mktime(&t);
    *dest = (unsigned int)tmp;

    return true;
}

// MCU initialization
void MCUInit() {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK; // turn off watchdog
}

// UART initialization
void UART5Init() {
    UART5->C2  &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);
    UART5->BDH =  0x00;
    UART5->BDL =  0x1A; // Baud rate 115 200 Bd, 1 stop bit
    UART5->C4  =  0x0F; // Oversampling ratio 16, match address mode disabled
    UART5->C1  =  0x00; // 8 data bitu, bez parity
    UART5->C3  =  0x00;
    UART5->MA1 =  0x00; // no match address (mode disabled in C4)
    UART5->MA2 =  0x00; // no match address (mode disabled in C4)
    UART5->S2  |= 0xC0;
    UART5->C2  |= ( UART_C2_TE_MASK | UART_C2_RE_MASK ); // Zapnout vysilac i prijimac
}

// Port initialization
void PortsInit() {

    // Enable CLOCKs for: UART5, RTC, PORT-A, PORT-B, PORT-E
    SIM->SCGC1 = SIM_SCGC1_UART5_MASK;
    SIM->SCGC5 = SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;
    SIM->SCGC6 = SIM_SCGC6_RTC_MASK;

    // PORT A
    PORTA->PCR[4] = PORT_PCR_MUX(0x01);

    // PORT B
    PORTB->PCR[5] = PORT_PCR_MUX(0x01); // D9  LED
    PORTB->PCR[4] = PORT_PCR_MUX(0x01); // D10 LED
    PORTB->PCR[3] = PORT_PCR_MUX(0x01); // D11 LED
    PORTB->PCR[2] = PORT_PCR_MUX(0x01); // D12 LED

    // PORT E
    PORTE->PCR[8]  = PORT_PCR_MUX(0x03); // UART0_TX
    PORTE->PCR[9]  = PORT_PCR_MUX(0x03); // UART0_RX
    PORTE->PCR[10] = PORT_PCR_MUX(0x01); // SW2
    PORTE->PCR[12] = PORT_PCR_MUX(0x01); // SW3
    PORTE->PCR[27] = PORT_PCR_MUX(0x01); // SW4
    PORTE->PCR[26] = PORT_PCR_MUX(0x01); // SW5
    PORTE->PCR[11] = PORT_PCR_MUX(0x01); // SW6

    // set ports as output
    PTA->PDDR =  GPIO_PDDR_PDD(0x0010);
    PTB->PDDR =  GPIO_PDDR_PDD(0x3C);
    PTB->PDOR |= GPIO_PDOR_PDO(0x3C); // turn all LEDs OFF
}

void Lights(int idx) {
    if (idx == 1) {
        for(unsigned int i=0; i<20; i++) { // all leds flashing

            PTB->PDOR &= ~GPIO_PDOR_PDO(0x3C);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
            Delay(300000);

        }
    }
    else if (idx == 2) {
        for(unsigned int i=0; i<10; i++) { // leds are one by one flashing

            GPIOB_PDOR ^= LED_D12;
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

            GPIOB_PDOR ^= LED_D11;
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

            GPIOB_PDOR ^= LED_D10;
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

            GPIOB_PDOR ^= LED_D9;
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

        }
    }
    else if (idx == 3) {
        for(unsigned int i=0; i<10; i++) { // flashing two edges leds and two middle

            GPIOB_PDOR ^= (LED_D12 | LED_D9);
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
            Delay(300000);

            GPIOB_PDOR ^= (LED_D10 | LED_D11);
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            Delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
            Delay(300000);

        }
    }
}

void Music(int idx) {
    if (idx == 1) { // interrupted beeping -> 15times beep+pause
        for(unsigned int i=0; i<15; i++) {
            Beep();
            Delay(70000);
        }
    }
    else if (idx == 2) { // non interrupted beeping -> one long beeeeeeeeeeeeep
        for(unsigned int i=0; i<20; i++) {
            Beep();
        }
    }
    else if (idx == 3) { // buzzer -> (pi-pi-pi-pi) (silence) (pi-pi-pi-pi) (silence) (pi-pi-pi-pi)
        for(unsigned int i=0; i<3; i++) {

            Beep();
            Delay(90000);
            Beep();
            Delay(90000);
            Beep();
            Delay(90000);
            Beep();

            Delay(2000000); // propably 1 second silence
        }
    }
}

void RTC_IRQHandler() {
    if(RTC_SR & RTC_SR_TAF_MASK) {
        //SendStr("Time Alarm Flag\r\n");
        SendStr("== ALARM (wait for finish) ==\r\n");
        Music(song);
        Lights(light);
        if (repeat_count > 0) {
            repeat_count--;
            RTC_TAR += delay;
        }
        else {
            RTC_TAR = 0;
        }
    }
    /*

    if(RTC_SR & RTC_SR_TOF_MASK) {
        //SendStr("Time Overflow Flag\r\n");
    }

    if(RTC_SR & RTC_SR_TIF_MASK) {
        //SendStr("Time Invalid Flag\r\n");
    }

    */
}

void RTCInit () {

    RTC_CR |= RTC_CR_SWR_MASK;  // SWR = 1, reset all RTC's registers
    RTC_CR &= ~RTC_CR_SWR_MASK; // SWR = 0

    RTC_TCR = 0x0000; // reset CIR and TCR

    RTC_CR |= RTC_CR_OSCE_MASK; // enable 32.768 kHz oscillator

    Delay(0x600000);

    RTC_SR &= ~RTC_SR_TCE_MASK; // turn OFF RTC

    RTC_TSR = 0x00000000; // MIN value in 32bit register
    RTC_TAR = 0xFFFFFFFF; // MAX value in 32bit register

    RTC_IER |= RTC_IER_TAIE_MASK;

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);

    RTC_SR |= RTC_SR_TCE_MASK; // turn ON RTC
}

int main() {

    MCUInit();
    PortsInit();
    UART5Init();
    RTCInit();

    Delay(500);

    int S = INITIALIZATION;
    bool retval;

    while (1) {

        switch(S) {
            //=================================================================
            case INITIALIZATION:

                PRINT("Enter date & time in format \"YYYY-MM-DD HH:MM:SS\".");

                SendStr("Input: ");
                ReceiveStr();

                retval = time_load(buffer, &seconds_init);
                if (retval == true) {
                    RTC_SR &= ~RTC_SR_TCE_MASK; // turn OFF RTC
                    RTC_TSR = seconds_init;
                    RTC_SR |= RTC_SR_TCE_MASK; // turn ON RTC
                    PRINT("Success.");
                    PRINT("============================");
                    S = SELECTION_MUSIC;
                }
                else {
                    PRINT("Please, repeat the initialization process.");
                }

            break;
            //=================================================================
            case SELECTION_MUSIC:

                PRINT("Choose music, type \"[1-3]\".");
                PRINT("You can preview music, type \"B[1-3]\".");

                SendStr("Input: ");
                ReceiveStr();

                if(strcmp(buffer,"B1")==0) {
                    Music(1);
                }
                else if(strcmp(buffer,"B2")==0) {
                    Music(2);
                }
                else if(strcmp(buffer,"B3")==0) {
                    Music(3);
                }
                else if(strcmp(buffer,"1")==0) {
                    song = 1;
                    PRINT("Success.");
                    PRINT("============================");
                    S = SELECTION_LIGHT;
                }
                else if(strcmp(buffer,"2")==0) {
                    song = 2;
                    PRINT("Success.");
                    PRINT("============================");
                    S = SELECTION_LIGHT;
                }
                else if(strcmp(buffer,"3")==0) {
                    song = 3;
                    PRINT("Success.");
                    PRINT("============================");
                    S = SELECTION_LIGHT;
                }

            break;
            //=================================================================
            case SELECTION_LIGHT:

                PRINT("Choose light, type \"[1-3]\".");
                PRINT("You can preview light, type \"L[1-3]\".");

                SendStr("Input: ");
                ReceiveStr();

                if(strcmp(buffer,"L1")==0) {
                    Lights(1);
                }
                else if(strcmp(buffer,"L2")==0) {
                    Lights(2);
                }
                else if(strcmp(buffer,"L3")==0) {
                    Lights(3);
                }
                else if(strcmp(buffer,"1")==0) {
                    light = 1;
                    PRINT("Success.");
                    PRINT("============================");
                    S = REPEAT;
                }
                else if(strcmp(buffer,"2")==0) {
                    light = 2;
                    PRINT("Success.");
                    PRINT("============================");
                    S = REPEAT;
                }
                else if(strcmp(buffer,"3")==0) {
                    light = 3;
                    PRINT("Success.");
                    PRINT("============================");
                    S = REPEAT;
                }

            break;
            //=================================================================
            case REPEAT:

                PRINT("Enter count of alarm repetition (MIN=1, MAX=5).");
                PRINT("Enter 0 for no repetition.");

                SendStr("Input: ");
                ReceiveStr();

                retval = sscanf(buffer,"%d", &repeat_count);
                if (retval == 1) {
                    if (repeat_count < 0 || 5 < repeat_count) {
                        PRINT("Wrong count!");
                    }
                    else {
                        PRINT("Success.");
                        PRINT("============================");
                        S = DELAY;
                    }
                }
                else {
                    PRINT("Wrong count!");
                }

            break;
            //=================================================================
            case DELAY:

                PRINT("Enter delay in seconds between alarm repetition (MIN=30, MAX=300).");

                SendStr("Input: ");
                ReceiveStr();

                retval = sscanf(buffer,"%d", &delay);
                if (retval == 1) {
                    if (delay < 30 || 300 < delay) {
                        PRINT("Wrong delay!");
                    }
                    else {
                        PRINT("Success.");
                        PRINT("============================");
                        S = SET_ALARM;
                    }
                }
                else {
                    PRINT("Wrong delay!");
                }

            break;
            //=================================================================
            case SET_ALARM:

                PRINT("Enter ALARM date & time in format \"YYYY-MM-DD HH:MM:SS\".");

                SendStr("Current date & time: ");
                seconds_tmp = RTC_TSR;
                time_convert(&seconds_tmp, buffer);
                PRINT(buffer);

                SendStr("Input: ");
                ReceiveStr();

                retval = time_load(buffer, &seconds_alarm);
                if ((retval == true) && (RTC_TSR < seconds_alarm)) {
                    RTC_TAR = seconds_alarm;
                    PRINT("Success.");
                    PRINT("============================");
                    S = ACTIVE;
                }
                else {
                    PRINT("Please, repeat the ALARM initialization process.");
                }

            break;
            //=================================================================
            case ACTIVE:

                SendStr("Current date & time: ");
                seconds_tmp = RTC_TSR;
                time_convert(&seconds_tmp, buffer);
                PRINT(buffer);

                SendStr("Alarm   date & time: ");
                seconds_tmp = RTC_TAR;
                if (seconds_tmp == 0) {
                    PRINT("OFF");
                }
                else {
                    time_convert(&seconds_tmp, buffer);
                    PRINT(buffer);
                }

                SendStr("Input: ");
                ReceiveStr();

                if(strcmp(buffer,"poweroff")==0) {
                    while(1);
                }
                else if(strcmp(buffer,"reboot")==0) {
                    S = INITIALIZATION;
                }
                else if(strcmp(buffer,"stop")==0) {
                    RTC_TAR = 0;
                    PRINT("Alarm disabled.");
                }
                else if(strcmp(buffer,"help")==0) {
                    PRINT("Commands: [ poweroff / reboot / stop / help ].");
                }
                else {
                    ;
                }

                PRINT("===============");

            break;
            //=================================================================
            default:
            break;
        }


    }

    return 0;
}
