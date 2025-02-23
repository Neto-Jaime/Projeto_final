#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "include/ssd1306.h"
#include "include/font.h"
#include "hardware/pwm.h"


// Definição de pinos e endereços
#define POTENCIOMETRO 26   // Pino ADC para simular o LDR
#define LED_PWM 13         // Pino do LED controlado por PWM
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_DISPLAY 0x3C
#define TECLA_A 5
#define TECLA_B 6

// Inicializa o display
ssd1306_t display;

bool tarefa_luminancia_ativa = false;

void configurar_pwm(uint pino) {
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pino);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0);
    pwm_init(slice, &config, true);
}

void configurar_adc() {
    adc_init();
    adc_gpio_init(POTENCIOMETRO);
}

void configurar_display() {
    i2c_init(i2c1, 400 * 1000); // Inicializa o I2C com a velocidade de 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&display, 128, 64, false, ENDERECO_DISPLAY, i2c1); // Inicializa o display OLED
    ssd1306_fill(&display, false); // Limpa a tela
    ssd1306_send_data(&display); // Envia os dados para o display
}

void exibir_menu() {
    ssd1306_fill(&display, false);
    ssd1306_draw_string(&display, "MENU", 50, 5);
    ssd1306_draw_string(&display, "1 LUMINOSIDADE", 10, 20);
    ssd1306_draw_string(&display, "2 SAIR", 10, 40);
    ssd1306_send_data(&display);
}

void exibir_luminosidade(int valor_simulado, int brilho, int valor_adc) {
    char buffer[50];
    int luminosidade = (valor_adc * 100) / 4095;

    ssd1306_fill(&display, false);
    sprintf(buffer, "LUZ ATUAL %d%%", luminosidade);
    ssd1306_draw_string(&display, buffer, 10, 10);

    sprintf(buffer, "BRILHO %d", brilho);
    ssd1306_draw_string(&display, buffer, 10, 20);
    ssd1306_send_data(&display);
}

void debug_uart(int valor_adc, int brilho) {
    printf("Leitura ADC: %d\n", valor_adc);  // Envia a leitura do ADC para o UART
    printf("Valor PWM: %d\n", brilho);   // Envia o valor do PWM para o UART
}

void configurar_botoes() {
    gpio_init(TECLA_A);
    gpio_init(TECLA_B);
    gpio_set_dir(TECLA_A, GPIO_IN);
    gpio_set_dir(TECLA_B, GPIO_IN);
    gpio_pull_up(TECLA_A);
    gpio_pull_up(TECLA_B);
}

bool verificar_entrada() {
    if (gpio_get(TECLA_A) == 0) {
        return true;  // Tarefa de controle de luminosidade selecionada
    } else if (gpio_get(TECLA_B) == 0) {
        return false; // Sair do menu
    }
    return false; // Nenhuma tecla pressionada
}

int main() {
    stdio_init_all(); // Inicializa a comunicação serial
    configurar_pwm(LED_PWM); // Configura o PWM para o LED
    configurar_adc(); // Configura o ADC para ler o potenciômetro
    configurar_display(); // Configura o display OLED
    configurar_botoes(); // Configura os botões de entrada
    
    exibir_menu(); // Exibe o menu inicial

    while (1) {
        if (verificar_entrada()) {
            tarefa_luminancia_ativa = true;
            while (tarefa_luminancia_ativa) {
                int leitura_adc = adc_read();  // Lê o valor do potenciômetro
                int brilho = 255 - (leitura_adc * 255 / 4095);  // Converte para PWM (0-255), ajustando para brilho inverso

                pwm_set_gpio_level(LED_PWM, brilho); // Ajusta o brilho do LED baseado no valor do potenciômetro
                exibir_luminosidade(brilho, brilho, leitura_adc); // Exibe no display a luminosidade simulada, o brilho e o valor do ADC
                debug_uart(leitura_adc, brilho); // Exibe o debug via UART

                if (verificar_entrada()) {
                    tarefa_luminancia_ativa = false;  // Sai do controle de luminosidade
                    exibir_menu(); // Exibe o menu novamente
                }

                sleep_ms(100); // Aguarda 100ms antes de atualizar
            }
        }
        sleep_ms(100); // Aguarda 100ms antes de verificar novamente
    }
}
