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

#define DISPLAY_Y 63 // pixels usaveis Y
#define DISPLAY_X 127 // pixels usaveis X

static volatile bool led_pwm = true;

ssd1306_t ssd; // Inicializa a estrutura do display
bool cor = true; // Borda do display

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
            cor = !cor;
            // Atualiza o conteúdo do display com animações
            ssd1306_fill(&ssd, !cor); // Limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
            ssd1306_send_data(&ssd); // Atualiza o display
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

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 3, 3, 63, 34, cor, !cor); // Desenha um retângulo
    ssd1306_draw_char(&ssd, 'O', 63, 31);
    ssd1306_send_data(&ssd);

    uint16_t vrx_value = 0;
    uint16_t vry_value = 0;

    uint16_t point_x = 63;
    uint16_t point_y = 31;

    while (true) {
        if(led_pwm){
            adc_select_input(0);
            vrx_value = adc_read();
            adc_select_input(1);
            vry_value = adc_read();
            pwm_set_gpio_level(LED_R, abs(vrx_value-2047)*2);
            pwm_set_gpio_level(LED_B, abs(vry_value-2047)*2);
            
            point_x = DISPLAY_X - 0.031 * vrx_value;
            point_y = DISPLAY_Y - 0.014 * vry_value;

            // afastar das bordas
            if(point_x > 100) {
                point_x -= 10;
            } else if(point_x < 20) {
                point_x += 10;
            }
            if(point_y > 50) {
                point_y -= 10;
            } else if(point_y < 20) {
                point_y += 10;
            }

            ssd1306_fill(&ssd, !cor);
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);
            ssd1306_draw_char(&ssd, 'O', point_x, point_y);
            ssd1306_send_data(&ssd);
        }
        sleep_ms(100);
    }
}
