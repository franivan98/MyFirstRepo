#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "lpc17xx_gpio.h"   /* GPIO handling */
#include "lpc17xx_pinsel.h" /* Pin function selection */
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"

/* Pin Definitions */
#define GREEN_LED   ((uint32_t)(1 << 20)) /* P0.22 connected to LED */
#define YELLOW_LED ((uint32_t)(1 << 21))  /* P0.0 connected to input */
#define RED_LED ((uint32_t)(1 << 22))  /* P0.0 connected to input */
#define ADC_INPUT ((uint32_t)(1<<2))

#define SECOND 10000 //1 second in microseconds

/* GPIO Direction Definitions */
#define INPUT  0
#define OUTPUT 1

/* Boolean Values */
#define TRUE  1
#define FALSE 0

#define ADC_FREQ 100000 //200khz

#define GREEN_TEMP 40
#define YELLOW_TEMP 70
#define RED_TEMP 20

//variable adc
static unsigned int adc_read_value = 0; //lo usamos para la lectura del adc

/**
 * @brief Initialize the GPIO peripheral
 *
 */
void configure_port(void)
{
    PINSEL_CFG_Type led_pin_cfg; /* Create a variable to store the configuration of the pin */

    /* We need to configure the struct with the desired configuration */
    led_pin_cfg.Portnum = PINSEL_PORT_0;           /* The port number is 0 */
    led_pin_cfg.Pinnum = PINSEL_PIN_20;            /* The pin number is 22 */
    led_pin_cfg.Funcnum = PINSEL_FUNC_0;           /* The function number is 0 */
    led_pin_cfg.Pinmode = PINSEL_PINMODE_PULLUP;   /* The pin mode is pull-up */
    led_pin_cfg.OpenDrain = PINSEL_PINMODE_NORMAL; /* The pin is in the normal mode */

    //GREEN_LED
    /* Configure the pin */
    PINSEL_ConfigPin(&led_pin_cfg);

    //YELLOW_LED
    led_pin_cfg.Pinnum = PINSEL_PIN_21;
    PINSEL_ConfigPin(&led_pin_cfg);

    //RED_LED
    led_pin_cfg.Pinnum = PINSEL_PIN_22;
    PINSEL_ConfigPin(&led_pin_cfg);

    //ADC_INPUT
    led_pin_cfg.Pinnum=PINSEL_PIN_2;
    led_pin_cfg.Funcnum = PINSEL_FUNC_1;
    PINSEL_ConfigPin(&led_pin_cfg);

    /* Set the pins as input or output */
    GPIO_SetDir(PINSEL_PORT_0, GREEN_LED | YELLOW_LED | RED_LED , OUTPUT); 
}

/*
* Se modifico el inicio de la conversion. se inicia cuando exista match con el timer. (linea 80)
*/

void configure_adc(void){
	ADC_Init(LPC_ADC, ADC_FREQ); //Inicializacion. Referencias con adc y frecuencia de conversion.
	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_7, ENABLE); //configuramos canal. le pasamos el adc, el canal y el enable

    ADC_StartCmd(LPC_ADC,ADC_START_ON_MAT01); //Inicia la conversion con el timer-
	ADC_IntConfig(LPC_ADC, ADC_CHANNEL_7, ENABLE); //Habilitamos las interrupciones por adc.
}

/*
* Se modifico linea 99, la configuracion de la interrupcion.
*/
void configure_timer_and_match(void){
    TIM_TIMERCFG_Type timer_cfg_struct;
    timer_cfg_struct.PrescaleOption = TIM_PRESCALE_USVAL; //prescale in microsecond value
    timer_cfg_struct.PrescaleValue = 100; //prescale value. cada 100 us incrementa el contador hasta q encuentre el match value

    TIM_Init(LPC_TIM0,TIM_TIMER_MODE, &timer_cfg_struct);

    //esta parte no la entiendo mucho.  
    TIM_MATCHCFG_Type match_cfg_struct;
    match_cfg_struct.MatchChannel = 0;
    match_cfg_struct.IntOnMatch = DISABLE; //No la necesitamos
    match_cfg_struct.StopOnMatch = DISABLE;
    match_cfg_struct.ResetOnMatch = ENABLE; //reseteamos el temporizador
    match_cfg_struct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    match_cfg_struct.MatchValue=(uint32_t)(60*SECOND);

    TIM_ConfigMatch(LPC_TIM0, &match_cfg_struct);

}

void start_timer(void){
    TIM_Cmd(LPC_TIM0, ENABLE); //Habilitamos el timer
    NVIC_EnableIRQ(TIMER0_IRQn); //Habilitamos la interrupcion
}

//-----------------------------------Interrupt Handler Functions-----------------------------------
void TMR0_IRQHandler(void){
    //Ya no hace falta
}

void ADC_IRQHandler(void){
    // la interrupcion se "deshabilita" cuando lo leo
    //tomo el valor 7 veces o mas, y saco el promedio
    NVIC_DisableIRQ(ADC_IRQn); //deshabilitamos la interrupcion. no c, el profe lo saco y lo volvio a poner xd
    for(unsigned int lecture=0; lecture<8;++lecture){
        while(!(ADC_ChannelGetStatus(LPC_ADC, lecture, ADC_DATA_DONE))); //esperamos a que termine la conversion
        adc_read_value+=ADC_ChannelGetValue(LPC_ADC, ADC_CHANNEL_7);
        ADC_StartCmd(LPC_ADC, ADC_START_NOW);
    }
    adc_read_value/=8;
    turn_on_led();
    NVIC_EnableIRQ(ADC_IRQn); //habilitamos la interrupcion
}

void turn_on_led(){
    //
    if(adc_read_value<=GREEN_TEMP){
        GPIO_SetValue(PINSEL_PORT_0, GREEN_LED);
        GPIO_ClearValue(PINSEL_PORT_0, YELLOW_LED);
        GPIO_ClearValue(PINSEL_PORT_0, RED_LED);
    }
    else{
        if(adc_read_value<=YELLOW_TEMP){
            GPIO_ClearValue(PINSEL_PORT_0, GREEN_LED);
            GPIO_SetValue(PINSEL_PORT_0, YELLOW_LED);
            GPIO_ClearValue(PINSEL_PORT_0, RED_LED);
        }
        else{
            GPIO_ClearValue(PINSEL_PORT_0, GREEN_LED);
            GPIO_ClearValue(PINSEL_PORT_0, YELLOW_LED);
            GPIO_SetValue(PINSEL_PORT_0, RED_LED);
        }
    }
}
/**
 * @brief Main function.
 * Initializes the system and toggles the LED based on the input pin state.
 */
int main(void)
{
    SystemInit(); /* Initialize the system clock (default: 100 MHz) */
    configure_port();
    configure_adc();
    configure_timer_and_match();
    start_timer();


    while (TRUE)
    {
       __asm("nop");
    }

    return 0; /* Program should never reach this point */
}
