#include "MK60D10.h"
#include <string.h>

/* Macros for bit-level registers manipulation */
#define GPIO_PIN_MASK   0x1Fu
#define GPIO_PIN(x)     (((1)<<(x & GPIO_PIN_MASK)))

/* Constants specifying delay loop duration */
#define DELAY_T 200000

/* Mapping of LEDs and buttons to specific port pins: */
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

int pressed_up = 0, pressed_down = 0;
unsigned int compare = 0x200;
unsigned char c;                       

unsigned char prompt[50] = "\r\nZadajte login:\r\n"; 
unsigned char corrl[50] = "xtotha01";  
unsigned char login[50];                

unsigned char message_success[50] = "\r\nSuccess\r\n";
unsigned char message_error[50]   = "\r\nError!\r\n";

//#################################################################################################
//#################################################################################################
//#################################################################################################

// Delay
void delay(long long bound) {
    for(long long i=0; i<bound; i++);
}

// Send character
void SendCh(char ch)  {
    while( !(UART5->S1 & UART_S1_TDRE_MASK) && !(UART5->S1 & UART_S1_TC_MASK) );
    UART5->D = ch;
}

// Send string
void SendStr(char *s)  {
    unsigned int i = 0;
    while (s[i]!='\0') {
        SendCh(s[i++]);
    }
}

// Receive character
char ReceiveCh(void) {
    while( !(UART5->S1 & UART_S1_RDRF_MASK) );
    return UART5->D;
}

/* Inicializace pinu pro vysilani a prijem pres UART - RX a TX */
void PinInit(void) {
    //SIM->SOPT2 |= SIM_SOPT2_UART0SRC(0x01);
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;         // Zapnout hodiny pro PORTA a PORTB
    SIM->SCGC1 |= SIM_SCGC1_UART5_MASK;

    PORTE->PCR[8] = ( 0 | PORT_PCR_MUX(0x03) ); // UART0_TX
    PORTE->PCR[9] = ( 0 | PORT_PCR_MUX(0x03) ); // UART0_RX

    PORTB->PCR[13] = ( 0|PORT_PCR_MUX(0x01) );  // Beeper (PTB13)
    PTB->PDDR = GPIO_PDDR_PDD( 0x2000 );        // "1" znamena, ze pin bude vystupni
}

/* Inicializace UART - nastaveni prenosove rychlosti 115200Bd, 8 bitu, bez parity */
void UART5Init(void) {
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


/* Pipnuti pres bzucak na PTB13 - generuje 500 period obdelnikoveho signalu */
void beep(void) {
int q;
    for (q=0; q<500; q++) {
        PTB->PDOR = GPIO_PDOR_PDO(0x2000);
        delay(500);
        PTB->PDOR = GPIO_PDOR_PDO(0x0000);
        delay(500);
    }
}

/* Initialize the MCU - basic clock settings, turning the watchdog off */
void MCUInit(void)  {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
}

void PortsInit(void)
{
    /* Turn on all port clocks */
    SIM->SCGC5 = SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

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

void LPTMR0_IRQHandler(void)
{
    // Set new compare value set by up/down buttons
    LPTMR0_CMR = compare; // !! the CMR reg. may only be changed while TCF == 1
    LPTMR0_CSR |=  LPTMR_CSR_TCF_MASK; // writing 1 to TCF tclear the flag
    GPIOB_PDOR ^= LED_D9; // invert D9 state
}

void LPTMR0Init(int count)
{
    SIM_SCGC5 |= SIM_SCGC5_LPTIMER_MASK;

    LPTMR0_CSR &= ~LPTMR_CSR_TEN_MASK;     // Turn OFF LPTMR to perform setup

    LPTMR0_PSR = ( LPTMR_PSR_PRESCALE(0) // 0000 is div 2
                 | LPTMR_PSR_PBYP_MASK  // LPO feeds directly to LPT
                 | LPTMR_PSR_PCS(1)) ; // use the choice of clock

    LPTMR0_CMR = count;  // Set compare value

    LPTMR0_CSR =(  LPTMR_CSR_TCF_MASK   // Clear any pending interrupt (now)
                 | LPTMR_CSR_TIE_MASK   // LPT interrupt enabled
                );

    NVIC_EnableIRQ(LPTMR0_IRQn);

    LPTMR0_CSR |= LPTMR_CSR_TEN_MASK;   // Turn ON LPTMR0 and start counting
}

int main(void) {

    MCUInit();
    PortsInit();
    PinInit();
    UART5Init();

    LPTMR0Init(compare);

    while (1) {

        SendStr(prompt);    // Vyslani vyzvy k zadani loginu

        for(unsigned int n=0; n<8; n++) {
            c = ReceiveCh();
            SendCh(c);        // Prijaty znak se hned vysle - echo linky
            login[n]=c;       // Postupne se uklada do pole
        }


        if(strcmp(login, corrl) == 0) {
            beep();
            SendStr(message_success);
        }
        else {
            SendStr(message_error);
        }







        // pressing the up button decreases the compare value,
        // i.e. the compare event will occur more often;
        // testing pressed_up avoids uncontrolled modification
        // of compare if the button is hold.
        if (!pressed_up && !(GPIOE_PDIR & BTN_SW5))
        {
            pressed_up = 1;
            compare -= 0x40;
        }
        else if (GPIOE_PDIR & BTN_SW5) pressed_up = 0;
        // pressing the down button increases the compare value,
        // i.e. the compare event will occur less often;
        if (!pressed_down && !(GPIOE_PDIR & BTN_SW3))
        {
            pressed_down = 1;
            compare += 0x40;
        }
        else if (GPIOE_PDIR & BTN_SW3) pressed_down = 0;
        // some limits - in order to keep the LED blinking reasonable
        if (compare < 0x40) compare = 0x40;
        if (compare > 0x400) compare = 0x400;
    }

    return 0;
}
