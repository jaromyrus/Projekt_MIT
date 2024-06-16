#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_clk.h"

// Inicializace hodin
void CLK_Config(void) {
    CLK_DeInit();
    CLK_HSECmd(DISABLE);
    CLK_LSICmd(DISABLE);
    CLK_HSICmd(ENABLE);
    while (CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == RESET);
    CLK_ClockSwitchCmd(ENABLE);
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
    CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
}

// Inicializace GPIO
void GPIO_Config(void) {
    GPIO_DeInit(GPIOB);
    GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);    // PIR senzor
    GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW); // Bzučák
    GPIO_Init(GPIOB, GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_SLOW); // LED dioda
}

// Funkce pro zpoždění
void delay_ms(uint16_t ms) {
    for(uint16_t i = 0; i < ms; i++) {
        for(uint16_t j = 0; j < 1600; j++) {
            __asm__("nop");
        }
    }
}

int main(void) {
    // Inicializace systému
    CLK_Config();
    GPIO_Config();

    while (1) {
        // Čtení stavu PIR senzoru
        if (GPIO_ReadInputPin(GPIOB, GPIO_PIN_4) == RESET) {
            // Pokud je detekován pohyb
            GPIO_WriteHigh(GPIOB, GPIO_PIN_5); // Aktivace bzučáku
            GPIO_WriteHigh(GPIOB, GPIO_PIN_6); // Aktivace LED
        } else {
            // Pokud není detekován pohyb
            GPIO_WriteLow(GPIOB, GPIO_PIN_5);  // Deaktivace bzučáku
            GPIO_WriteLow(GPIOB, GPIO_PIN_6);  // Deaktivace LED
        }

        // Zpoždění pro stabilizaci čtení
        delay_ms(100);
    }
}

void rx_action(void) {
    char c = UART1_ReceiveData8();
}

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6) {
    // Můžete přidat svůj kód pro přerušení
}
