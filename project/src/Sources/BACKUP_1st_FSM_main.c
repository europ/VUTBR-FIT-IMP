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

#define PRINT_NEWLINE() { \
    SendStr("\r\n"); \
}

#define PRINT(msg) { \
    SendStr(msg); \
    SendStr("\r\n"); \
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
    INITIALIZATION,
    ACTIVE
};

unsigned long long int seconds;
char time_buffer[BUFFER_SIZE];
char buffer[BUFFER_SIZE];

//#################################################################################################
//#################################################################################################
//#################################################################################################

// Convert time FROM seconds TO buffer
void time_convert() {
    time_t tmp = seconds;
    struct tm ts = *localtime(&tmp);

    for (unsigned int i=0; i<BUFFER_SIZE; i++) {
        time_buffer[i]='\0';
    }

    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &ts);
}

// Convert time FROM buffer TO seconds
bool time_load() {
    int retval;
    struct tm t;

    retval = strlen(time_buffer);
    if (retval < 14 || 19 < retval) return false;

    retval = sscanf(time_buffer, "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon , &t.tm_mday, &t.tm_hour, &t.tm_min , &t.tm_sec);
    if (retval != 6) return false;

    if (t.tm_year < 1970 || 2038 < t.tm_year) return false;
    if (t.tm_mon  < 1    || 12   < t.tm_mon ) return false;
    if (t.tm_mday < 1    || 31   < t.tm_mday) return false;
    if (t.tm_hour < 0    || 23   < t.tm_hour) return false;
    if (t.tm_min  < 0    || 59   < t.tm_min ) return false;
    if (t.tm_sec  < 0    || 59   < t.tm_sec ) return false;

    t.tm_year = t.tm_year - 1900;
    seconds = (long)mktime(&t);

    return true;
}

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

void RTC_IRQHandler() {
    if(RTC_SR & RTC_SR_TAF_MASK) {
        RTC_TAR = 0;
        Beep();
        SendStr("=======ALARM=======\r\n");
        SendStr("Time Alarm Flag\r\n");
    }
/*
    if(RTC_SR & RTC_SR_TOF_MASK) {
        SendStr("Time Overflow Flag\r\n");
    }
    if(RTC_SR & RTC_SR_TIF_MASK) {
        SendStr("Time Invalid Flag\r\n");
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

    RTC_TSR = 10U;
    RTC_TAR = 0U;
    //RTC_TAR = RTC_TSR+3U;

    RTC_IER |= RTC_IER_TAIE_MASK;

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);

    RTC_SR |= RTC_SR_TCE_MASK; // turn ON RTC
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

int main() {

    MCUInit();
    PortsInit();
    UART5Init();
    RTCInit();

    Delay(500);

    int S = INITIALIZATION;
    bool retval;
    PRINT("Please, initialize the machine with the current date in format \"YYYY-MM-DD HH:MM:SS\".");

    while (1) {

        switch(S) {
            //=================================================================
            case INITIALIZATION:

                ReceiveStr();

                strcpy(time_buffer,buffer);
                retval = time_load();

                if (retval == true) {
                    PRINT("Initialization was successful.");
                    PRINT_NEWLINE();
                    S = ACTIVE;
                }
                else {
                    PRINT("Something went wrong, please, repeat the initialization process.");
                    PRINT("Initialize the machine with the current date in format \"YYYY-MM-DD HH:MM:SS\"!");
                }

                break;
            //=================================================================
            case ACTIVE:

                ReceiveStr();
                PRINT(buffer);

                // Music
                if(strcmp(buffer,"B1")==0) {
                    Music(1);
                }
                else if(strcmp(buffer,"B2")==0) {
                    Music(2);
                }
                else if(strcmp(buffer,"B3")==0) {
                    Music(3);
                }

                // Lights
                if(strcmp(buffer,"L1")==0) {
                    Lights(1);
                }
                else if(strcmp(buffer,"L2")==0) {
                    Lights(2);
                }
                else if(strcmp(buffer,"L3")==0) {
                    Lights(3);
                }

                break;
            //=================================================================
            default:
                break;
        }

    }

    return 0;
}
