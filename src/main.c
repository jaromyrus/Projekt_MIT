#include <main.h>
#include <stm/stm8s.h>
#include <stm/stm8s_i2c.h>
#include <swi2c.h>

/*Essential Definitons*/
#define DISPLAY_DATA_PORT GPIOC

#define DISPLAY_A_PIN GPIO_PIN_5
#define DISPLAY_A_PORT GPIOD
#define DISPLAY_B_PIN GPIO_PIN_0
#define DISPLAY_B_PORT GPIOE

#define BTN_PIN GPIO_PIN_4
#define BTN_PORT GPIOE

#define ENCODER_CLK_PIN GPIO_PIN_3
#define ENCODER_CLK_PORT GPIOG

#define ENCODER_DT_PIN GPIO_PIN_2
#define ENCODER_DT_PORT GPIOG

uint8_t nums[10] = {0b11101110, 0b00100010, 0b10111100, 0b10110110, 0b01110010, 0b11010110, 0b11011110, 0b10100010, 0b11111111, 0b11110110}; // Number defined by port value

#define ATH10_I2C_ADDRESS 0x38 << 1 // ATH10 I2C address (replace with actual address if different)

/*INTERRUPT_HANDLER(TIM2_UPD_OVF_IRQHandler, ITC_IRQ_TIM2_OVF){
    TIM2_ClearFlag(TIM2_FLAG_UPDATE);
    printf("Hello World");
}*/
uint8_t low_temp = 0;
uint8_t high_temp = 99;
uint8_t current_temp = 23;
void setup(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); // Set CLK

    /*Init Displays*/

    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DISPLAY_DATA_PORT, GPIO_PIN_7, GPIO_MODE_OUT_PP_LOW_SLOW);

    GPIO_Init(DISPLAY_A_PORT, DISPLAY_A_PIN, GPIO_MODE_OUT_PP_LOW_SLOW); // CC gate
    GPIO_Init(DISPLAY_B_PORT, DISPLAY_B_PIN, GPIO_MODE_OUT_PP_LOW_SLOW); // CC gate

    GPIO_Init(BTN_PORT, BTN_PIN, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(ENCODER_CLK_PORT, ENCODER_CLK_PIN, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(ENCODER_DT_PORT, ENCODER_DT_PIN, GPIO_MODE_IN_PU_NO_IT);

    init_uart1();
    init_time();
}
void init_ath10()
{
    swi2c_init();
}
uint8_t state = 0; // 0->temp 1->low 2->high

uint8_t old_btn_state = 1; // old button state

void main_task(void)
{
    if (GPIO_ReadInputPin(BTN_PORT, BTN_PIN))
    { // check button state
        if (old_btn_state != GPIO_ReadInputPin(BTN_PORT, BTN_PIN))
        {                                       // compare two states, decide if button in pressed for fir time since release
            state = state >= 2 ? 0 : state + 1; // state change
        }
    }
    old_btn_state = GPIO_ReadInputPin(BTN_PORT, BTN_PIN); // copy button state
    uint8_t number = 0;                                   // currently displayed number
    if (state == 0)
    { // this tree selects the displayed value
        number = current_temp;
    }
    else if (state == 1)
    {
        number = low_temp;
    }
    else if (state == 2)
    {
        number = high_temp;
    }
    uint8_t units = number % 10;                   // getting units
    uint8_t tens = (number - units) / 10;          // gettings tens
    GPIO_WriteLow(DISPLAY_B_PORT, DISPLAY_B_PIN);  // Turn off CC of units
    DISPLAY_DATA_PORT->ODR = nums[units]; // Write tens on display
    GPIO_WriteHigh(DISPLAY_A_PORT, DISPLAY_A_PIN); // Turn on CC of tens
    delay_ms(1);                        // eye delay to show result on display
    GPIO_WriteLow(DISPLAY_A_PORT, DISPLAY_A_PIN);  // Turn off CC of tens
    DISPLAY_DATA_PORT->ODR = nums[tens];          // Write units on display
    GPIO_WriteHigh(DISPLAY_B_PORT, DISPLAY_B_PIN); // Turn on CC of units
    delay_ms(1);                                  // eye delay to show result on display
}
void encoder_action(int8_t value) //encoder function
{
    if (state == 1)
    {
        if (((int)low_temp) + value > 99)
            low_temp = 99;
        else if (((int)low_temp) + value < 0)
            low_temp = 0;
        else
            low_temp += value;
    }
    else if (state == 2)
    {
        if (((int)high_temp) + value > 99)
            high_temp = 99;
        else if (((int)high_temp) + value < 0)
            high_temp = 0;
        else
            high_temp += value;
    }
}
void setup_pwm() //ventilator inicialized by PWM function
{
    TIM2_TimeBaseInit(TIM2_PRESCALER_1, 640);
    TIM2_OC1Init(
        TIM2_OCMODE_PWM1,
        TIM2_OUTPUTSTATE_ENABLE,
        3000,
        TIM2_OCPOLARITY_HIGH);
    TIM2_OC1PreloadConfig(ENABLE);
    TIM2_Cmd(ENABLE);
}
int main(void) // main loop
{
    setup();
    init_ath10();
    setup_pwm();
    uint32_t timestamp = milis();
    uint8_t old_clk_state = 0;
    uint8_t buffer[6];
    printf("%d\n", swi2c_test_slave(ATH10_I2C_ADDRESS));
    bool readed = FALSE;
    TIM2_SetCompare1(640);
    delay_ms(1000);
    while (1)
    {
        uint8_t clk_state = GPIO_ReadInputPin(ENCODER_CLK_PORT, ENCODER_CLK_PIN);
        if (clk_state != old_clk_state && clk_state == 0)
        {
            if (GPIO_ReadInputPin(ENCODER_DT_PORT, ENCODER_DT_PIN) != clk_state)
                encoder_action(1);
            else
                encoder_action(-1);
        }
        old_clk_state = clk_state;
        main_task();
        if (milis() - timestamp > 1000)
        {
            //delay_ms(100);
            swi2c_read_buf(ATH10_I2C_ADDRESS, 0x00, buffer, 6);
            uint32_t raw = (((uint32_t)buffer[3] & 15) << 16) | ((uint32_t)buffer[4] << 8) | buffer[5];
            current_temp = (raw * 200 / 1048576) - 50;
            uint16_t duty = 0;
            if(current_temp >= high_temp) duty = 640;
            else if(current_temp <= low_temp) duty = 0;
            else{
                duty = 140 * (current_temp - low_temp) / (high_temp - low_temp) + 500;
            }
            TIM2_SetCompare1(duty);
            printf("%d, %d, %d\n", low_temp, high_temp, current_temp);
            timestamp = milis();
            readed = FALSE;
        } else if(milis() - timestamp < 900 && milis() - timestamp > 890){
            if(!readed){
                readed = TRUE;
                swi2c_write_buf(ATH10_I2C_ADDRESS, 0xAC, NULL, 0);
            }
        }
    }
    return 0;
}

void rx_action(void) // will not compile without whis event definition
{
    char c = UART1_ReceiveData8();
}

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)
{
}
