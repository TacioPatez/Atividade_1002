#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"     
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define IS_RGBW false

#define BUTTON_A 5

#define VRY_PIN 26
#define VRX_PIN 27
#define JOYB_PIN 22

#define LED_G 11
#define LED_B 12
#define LED_R 13

static volatile bool led_pwm = true;

// Armazena o tempo do último evento (em microssegundos)
static volatile uint32_t last_time = 0; 

//  Funções de Interrupção
static void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if(current_time - last_time > 200000) { // 200 ms de debouncing
        if(gpio == JOYB_PIN){
            last_time = current_time;
            gpio_put(LED_G, !gpio_get(LED_G));
        } else {
            led_pwm = !led_pwm;
            pwm_set_gpio_level(LED_R, 0);
            pwm_set_gpio_level(LED_B, 0);
        }
    }
}

uint pwm_init_gpio(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, 4096);
    
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

int main()
{
    stdio_init_all();

    adc_init();

    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);

    pwm_init_gpio(LED_B);
    pwm_init_gpio(LED_R);

    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);

    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(JOYB_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    uint16_t vrx_value = 0;
    uint16_t vry_value = 0;

    while (true) {
        if(led_pwm){
            adc_select_input(0);
            vrx_value = abs(adc_read()-2047)*2;
            adc_select_input(1);
            vry_value = abs(adc_read()-2047)*2;
            pwm_set_gpio_level(LED_R, vrx_value);
            pwm_set_gpio_level(LED_B, vry_value);
        }
        
        sleep_ms(1000);
    }
}
