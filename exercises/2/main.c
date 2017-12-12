/************************************************************************************/
/*                                                                                  */
/*  Laboratorni uloha c. 2 z predmetu IMP                                           */
/*                                                                                  */
/*  Aplikace GPIO pro obsluhu tlacitek s vyuzitim preruseni a rizeni LED displeje   */
/*                                                                                  */
/*  Reseni vytvoril(a) a odevzdava: (Adrián Tóth, xtotha01)                         */
/*                                                                                  */
/************************************************************************************/

#include "MKL05Z4.h"

/* -------------------------------------------------------------------------------- *
 * (Makro)definice, deklarace atd. souvisejici s tlacitky                           *
 * -------------------------------------------------------------------------------- */
#define PB3_ISFR_MASK   0x08
#define PB4_ISFR_MASK   0x10
#define PB5_ISFR_MASK   0x20
#define PB6_ISFR_MASK   0x40
#define PB7_ISFR_MASK   0x80
#define PB_PCR_ISF_MASK PORT_PCR_ISF_MASK

/* -------------------------------------------------------------------------------- *
 * (Makro)definice, deklarace atd. souvisejici se segmentovym LED displejem         *
 * -------------------------------------------------------------------------------- */

/* Identifikace bloku LED segmentu na displeji */
#define DIG1_ID         1
#define DIG2_ID         2
#define DIG3_ID         4
#define DIG4_ID         8

/* Masky pro aktivaci bloku LED segmentu na displeji (aktivni v "1") */
#define DIG1_MASK       0x0800
#define DIG2_MASK       0x1000
#define DIG3_MASK       0x2000
#define DIG4_MASK       0x1000
#define DIG123_MASK     DIG1_MASK+DIG2_MASK+DIG3_MASK
#define DIG4_SHIFT      16

/* Masky vyvodu pro rizeni LED segmentu */
#define SEG_MASK        0x07F8
#define SEG_ALL_OFF     SEG_MASK
#define SEG_ALL_ON      ~SEG_MASK

/* Hodnoty pro zobrazeni informace na LED segmentech jednoho bloku (aktivni v "0") */
#define SEG_BIG_L       0x0E3F
#define SEG_BIG_A       0x0440
#define SEG_SMALL_B     0x0418
#define SEG_DP          0x03F8
#define SEG_2           0x0520
#define SEG_DIR_L       0x0600
#define SEG_DIR_R       0x0600
#define SEG_DIR_U       0x04E0
#define SEG_DIR_D       0x0518
#define SEG_DIR_C       0x05F8

#define NAHRADTE_TESTEM 1

/* -------------------------------------------------------------------------------- *
 * Funkce realizujici aktivni cekani                                                *
 * -------------------------------------------------------------------------------- */
void delay(uint64_t bound) {
    for(uint64_t i=0; i < bound; i++) { __NOP(); }
}

/* -------------------------------------------------------------------------------- *
 * Funkce pro inicializaci vyvodu MCU pro rizeni LED displeje                       *
 * -------------------------------------------------------------------------------- *
 *  Ilustrace k rozlozeni a znaceni LED segmentu v ramci jednoho bloku:
 *          A
 *          _
 *      F |   | B
 *          _
 *          G
 *      E | _ | C   .
 *          D       DP
 *
 *  Detaily k vyvodum MCU pro rizeni LED segmentu:
 *  Port:       PORTB       .                                PORTA
 *             =======      .                               =======
 *  Bity:       | 12 |      .   | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *  Vyznam:      DIG4       .    DIG3 DIG2 DIG1  DP   G   F   E   D   C   B   A   -   -   -
 *  Aktivita:   | ------- logicka 1 ---------- | --------------logicka 0 ------------------ |
 *                         (bloky)                             (segmenty)
 */
void init_display(void) {
    /* Nastav vyvody k LED displeji na GPIO */
    for(uint8_t i=3; i<=13; i++){
        PORTA->PCR[i] = ( PORT_PCR_MUX(0x01) );
    }
    PORTB->PCR[12] = ( PORT_PCR_MUX(0x01) );

    /* Nastav GPIO vyvody k LED displeji na vystupni */
    PTA->PDDR = GPIO_PDDR_PDD( DIG123_MASK + SEG_MASK );
    PTB->PDDR = GPIO_PDDR_PDD( DIG4_MASK );
}

/* -------------------------------------------------------------------------------- *
 * Funkce pro prevod kombinace ID bloku na masku pro aktivaci bloku z portu A, B    *
 * (dilci maska pro port A je v dolni, pro port B v horni polovine celkove masky)   *
 * -------------------------------------------------------------------------------- */
uint32_t activation_mask(uint8_t digit_id) {
    uint32_t digit_mask = 0;
    if(digit_id & DIG1_ID) digit_mask |= DIG1_MASK;
    if(digit_id & DIG2_ID) digit_mask |= DIG2_MASK;
    if(digit_id & DIG3_ID) digit_mask |= DIG3_MASK;
    if(digit_id & DIG4_ID) digit_mask |= (DIG4_MASK << DIG4_SHIFT);
    return digit_mask;
}

/* -------------------------------------------------------------------------------- *
 * Funkce pro zobrazeni informace (urcene obsahem value) na LED segmentech          *
 * v blocich displeje (urcenych maskou digit_mask)                                  *
 * -------------------------------------------------------------------------------- */
void display_msg(uint32_t digit_mask, uint32_t value) {
    PTA->PDOR = GPIO_PDOR_PDO(value & SEG_MASK);
    PTA->PDOR |= GPIO_PDOR_PDO(digit_mask & DIG123_MASK);
    PTB->PDOR |= GPIO_PDOR_PDO((digit_mask >> DIG4_SHIFT) & DIG4_MASK);
}

/* -------------------------------------------------------------------------------- *
 * Funkce pro smazani obsahu LED displeje                                           *
 * -------------------------------------------------------------------------------- */
void clear_display(void) {
    PTA->PDOR = GPIO_PDOR_PDO(SEG_MASK);
    PTA->PDOR &= GPIO_PDOR_PDO(~DIG1_MASK);
    PTA->PDOR &= GPIO_PDOR_PDO(~DIG2_MASK);
    PTA->PDOR &= GPIO_PDOR_PDO(~DIG3_MASK);
    PTB->PDOR &= GPIO_PDOR_PDO(~DIG4_MASK);
}

/* -------------------------------------------------------------------------------- *
 * Funkce pro vypis uvitaci zpravy na LED displej                                   *
 * -------------------------------------------------------------------------------- */
void display_welcome_msg(void) {
    uint64_t timeout=250000;
    for(uint8_t i=0; i<3; i++) {
        display_msg(activation_mask(DIG1_ID), SEG_BIG_L);
        delay(timeout);
        clear_display();
        display_msg(activation_mask(DIG2_ID), SEG_BIG_A);
        delay(timeout);
        clear_display();
        display_msg(activation_mask(DIG3_ID), SEG_SMALL_B);
        delay(timeout);
        clear_display();
        display_msg(activation_mask(DIG4_ID), SEG_2);
        delay(timeout);
        clear_display();
    }
}

/* -------------------------------------------------------------------------------- *
 * Obsluha preruseni (ISR) pro PORTB/tlacitka                                       *
 * -------------------------------------------------------------------------------- */
void PORTB_IRQHandler(void) {
    int64_t timeout_debounce = 50000;
    int64_t timeout_display = 100000;

    /* !!!!!!!!!!!!!! */
    /* Pockejte na odezneni zakmitu od kontaktu tlacitka */
    delay(timeout_debounce);

    /* !!!!!!!!!!!!!! */
    /* Doprogramujte vetveni obsluhy preruseni pro kazde stisknute tlacitko
     * (nize uvedena sablona predpoklada stisk jednoho tlacitka v case):
     * - kazdou vetev zahajte testem, zda preruseni bylo vyvolano stiskem daneho tlacitka
     *   (viz registry Portu B: PCR (priznak ISF) a/nebo ISFR),
     * - provedte odezvu na stisk tlacitka (predprogramovano - neni potreba menit),
     * - vynulujte priznak preruseni vyvolaneho stiskem tlacitka
     *   (viz registry Portu B: PCR (priznak ISF) a/nebo ISFR).
     */

    if (PORTB->ISFR & PB3_ISFR_MASK) { /* tlacitko NAHORU (Up) */
        /* zacatek odezvy */
        display_msg(activation_mask(DIG2_ID + DIG3_ID), SEG_DIR_U);
        delay(timeout_display);
        clear_display();
        /* konec odezvy */
        /* Vynulujte priznak */
        PORTB->PCR[3] = PORTB->PCR[3] | PORT_PCR_ISF(1);
    }
    else if (PORTB->ISFR & PB4_ISFR_MASK) { /* tlacitko PROSTREDNI (Central) */
        /* zacatek odezvy */
        display_msg(activation_mask(DIG1_ID + DIG4_ID), SEG_DIR_C);
        delay(timeout_display);
        clear_display();
        display_msg(activation_mask(DIG2_ID), SEG_DIR_C);
        delay(timeout_display);
        clear_display();
        display_msg(activation_mask(DIG3_ID), SEG_DIR_C);
        delay(timeout_display);
        clear_display();
        /* konec odezvy */

        /* Vynulujte priznak */
        PORTB->PCR[4] = PORTB->PCR[4] | PORT_PCR_ISF(1);
    }
    else if (PORTB->ISFR & PB5_ISFR_MASK) { /* tlacitko DOLEVA (Left) */
        /* zacatek odezvy */
        display_msg(activation_mask(DIG1_ID), SEG_DIR_L);
        delay(timeout_display);
        clear_display();
        /* konec odezvy */

        /* Vynulujte priznak */
        PORTB->PCR[5] = PORTB->PCR[5] | PORT_PCR_ISF(1);
    }
    else if (PORTB->ISFR & PB6_ISFR_MASK) { /* tlacitko DOLU (Down) */
        /* zacatek odezvy */
        display_msg(activation_mask(DIG2_ID + DIG3_ID), SEG_DIR_D);
        delay(timeout_display);
        clear_display();
        /* konec odezvy */

        /* Vynulujte priznak */
        PORTB->PCR[6] = PORTB->PCR[6] | PORT_PCR_ISF(1);
    }
    else if (PORTB->ISFR & PB7_ISFR_MASK) { /* tlacitko DOPRAVA (Right) */
        /* Zacatek odezvy */
        display_msg(activation_mask(DIG4_ID), SEG_DIR_R);
        delay(timeout_display);
        clear_display();
        /* konec odezvy */

        /* Vynulujte priznak */
        PORTB->PCR[7] = PORTB->PCR[7] | PORT_PCR_ISF(1);
    }
    else { }
}

/* -------------------------------------------------------------------------------- *
 * Inicializace hardware                                                            *
 * -------------------------------------------------------------------------------- */
void init_hardware(void) {
    uint8_t i;

    MCG->C4 |= (MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01));     /* Nastav hodinovy podsystem */
    SIM->CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    SIM->COPC = SIM_COPC_COPT(0x00);                            /* Vypni WatchDog */
    SIM->SCGC5 = (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK); /* Aktivuj hodiny pro PORTA, PORTB */

    init_display();
    display_welcome_msg();

    /* !!!!!!!!!!!!!!!!!!!!
     * Nastavte PCR registr kazdeho vyvodu Portu B s pripojenym tlacitkem takto:
     * - Nulujte priznak ISF (Interrupt Status Flag)
     * - Nastavte citlivost preruseni na sestupnou hranu (viz priznak IRQC, Interrupt Configuration)
     * - Nastavte vyvod do rezimu GPIO (viz priznak MUX, Pin Mux Control)
     * - Povolte interni Pull rezistor (viz priznak PE, Pull Enable)
     * - Zvolte Pull-Up rezistor (viz priznak PS, Pull Select)
     */

    for (int i = 3; i<8; i++) {
        PORTB->PCR[i] = PORT_PCR_PE(0x1) | PORT_PCR_PS(0x1) | PORT_PCR_ISF(0x1) | PORT_PCR_MUX(0x1) | PORT_PCR_IRQC(0xA);
    }

    /* Pro dalsi kroky je nutno zjistit cislo (x) pro IRQ Portu B (IRQx) */

    /* !!!!!!!!!!!!!! */
    /* Zruste/nulujte (pripadne) neobslouzene pozadavky z IRQx
     * (viz NVIC registr ICPR popr. funkce NVIC_ClearPendingIRQ(IRQn_Type IRQn))
     */
    NVIC_ClearPendingIRQ(31);

    /* !!!!!!!!!!!!!!!!! */
    /* Povolte preruseni z IRQx
     * (viz NVIC registr ISER popr. funkce NVIC_EnableIRQ(IRQn_Type IRQn))
     */
    NVIC_EnableIRQ(31);
}

/* -------------------------------------------------------------------------------- *
 * Hlavni funkce                                                                    *
 * -------------------------------------------------------------------------------- */
int main(void) {
    init_hardware();

    for(;;){ ; }    /* zamezeni navratu z main(), napr. vstupem do nekonecneho cyklu */
    return 0;
}
