// Author: Adrián Tóth
// Login:  xtotha01
// Date:   12.12.2017

//#################################################################################################
//#################################################################################################
//#################################################################################################

#include "MK60D10.h"
#include <string.h>

//#################################################################################################
//#################################################################################################
//#################################################################################################

#define BUFFER_SIZE 100
#define PRINT_NEWLINE() {SendStr(newline);}

// Mapping of LEDs and buttons to specific port pins:
// Note: only D9, SW3 and SW5 are used in this sample app

#define LED_D9  0x20 // Port B, bit 5
#define LED_D10 0x10 // Port B, bit 4
#define LED_D11 0x8  // Port B, bit 3
#define LED_D12 0x4  // Port B, bit 2

#define BTN_SW2 0x400     // Port E, bit 10
#define BTN_SW3 0x1000    // Port E, bit 12
#define BTN_SW4 0x8000000 // Port E, bit 27
#define BTN_SW5 0x4000000 // Port E, bit 26
#define BTN_SW6 0x800     // Port E, bit 11

//#################################################################################################
//#################################################################################################
//#################################################################################################

unsigned char buffer[BUFFER_SIZE];
unsigned char newline[3] = "\r\n";

unsigned char message_success[50] = "Success\n";
unsigned char message_error  [50] = "Error!\n";

//#################################################################################################
//#################################################################################################
//#################################################################################################

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

void ReceiveStr() {

    for (unsigned int i=0; i<BUFFER_SIZE; i++) {
        buffer[i]='\0';
    }

    unsigned char c;
    unsigned int i = 0;

    while (i < (BUFFER_SIZE - 1) ) {

        c = ReceiveCh();
        SendCh(c);

        if (c == '\r')
            break;

        buffer[i] = c;
        i++;
    }

    buffer[i] = '\0';
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

// MCU initialization
void MCUInit() {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK; // turning the watchdog off
}

// MCU pin initialization
void PinInit() {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    SIM->SCGC1 |= SIM_SCGC1_UART5_MASK;
    PORTE->PCR[8] = ( 0 | PORT_PCR_MUX(0x03) ); // UART0_TX
    PORTE->PCR[9] = ( 0 | PORT_PCR_MUX(0x03) ); // UART0_RX

    PORTA->PCR[4] = ( 0 | PORT_PCR_MUX(0x01) );
    PTA->PDDR = GPIO_PDDR_PDD( 0x0010 );
}

/* Inicializace UART - nastaveni prenosove rychlosti 115200Bd, 8 bitu, bez parity */
void UART5Init() {
    UART5->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);
    UART5->BDH = 0x00;
    UART5->BDL = 0x1A;      // Baud rate 115 200 Bd, 1 stop bit
    UART5->C4 = 0x0F;       // Oversampling ratio 16, match address mode disabled
    UART5->C1 = 0x00;       // 8 data bitu, bez parity
    UART5->C3 = 0x00;
    UART5->MA1 = 0x00;      // no match address (mode disabled in C4)
    UART5->MA2 = 0x00;      // no match address (mode disabled in C4)
    //UART5->S1 |= 0x1F;
    UART5->S2 |= 0xC0;
    UART5->C2 |= ( UART_C2_TE_MASK | UART_C2_RE_MASK );   // Zapnout vysilac i prijimac
}

void PortsInit() {
    /* Turn on all port clocks */
    SIM->SCGC5 = SIM_SCGC5_PORTA_MASK |
                 SIM_SCGC5_PORTB_MASK |
                 SIM_SCGC5_PORTC_MASK |
                 SIM_SCGC5_PORTD_MASK |
                 SIM_SCGC5_PORTE_MASK;

    /* Set corresponding PTB pins (connected to LED's) for GPIO functionality */
    PORTB->PCR[5] = PORT_PCR_MUX(0x01); // D9
    PORTB->PCR[4] = PORT_PCR_MUX(0x01); // D10
    PORTB->PCR[3] = PORT_PCR_MUX(0x01); // D11
    PORTB->PCR[2] = PORT_PCR_MUX(0x01); // D12

    PORTE->PCR[10] = PORT_PCR_MUX(0x01); // SW2
    PORTE->PCR[12] = PORT_PCR_MUX(0x01); // SW3
    PORTE->PCR[27] = PORT_PCR_MUX(0x01); // SW4
    PORTE->PCR[26] = PORT_PCR_MUX(0x01); // SW5
    PORTE->PCR[11] = PORT_PCR_MUX(0x01); // SW6

    /* Change corresponding PTB port pins as outputs */
    PTB->PDDR = GPIO_PDDR_PDD( 0x3C );
    PTB->PDOR |= GPIO_PDOR_PDO( 0x3C); // turn all LEDs OFF
}

int main() {

    // initialization
    MCUInit();
    PortsInit();
    PinInit();
    UART5Init();

    while (1) {

        SendStr("=======\r\n");

        ReceiveStr();
        PRINT_NEWLINE();

        SendStr(buffer);
        PRINT_NEWLINE();

    }

    return 0;
}
