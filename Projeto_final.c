#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "include/ssd1306.h"
#include "include/font.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

// Definição de pinos e endereços
#define POTENCIOMETRO 26
#define LED_PWM 13
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_DISPLAY 0x3C
#define TECLA_A 5
#define TECLA_B 6
#define MIC_ADC 2
#define BUZZER_1 10
#define BUZZER_2 21
#define LIMITE_SOM 2600  // Ajuste para evitar falsos positivos
// Definição da duração das notas do alerta
#define TEMPO_NOTA 100  // Duração de cada nota em ms

// Inicializa o display
ssd1306_t display;

// Variável global para armazenar a opção selecionada
int opcao_menu = 0;  // 0 = LUMINOSIDADE, 1 = TESTE DE RUÍDO

void configurar_i2c() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void configurar_display() {
    ssd1306_init(&display, 128, 64, false, ENDERECO_DISPLAY, I2C_PORT);
    ssd1306_config(&display);
    sleep_ms(200);
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);
}

void configurar_adc() {
    adc_init();
    adc_gpio_init(POTENCIOMETRO);
    adc_gpio_init(MIC_ADC);
}

void configurar_botoes() {
    gpio_init(TECLA_A);
    gpio_init(TECLA_B);
    gpio_set_dir(TECLA_A, GPIO_IN);
    gpio_set_dir(TECLA_B, GPIO_IN);
    gpio_pull_up(TECLA_A);
    gpio_pull_up(TECLA_B);
}

void configurar_buzzer() {
    gpio_set_function(BUZZER_1, GPIO_FUNC_PWM); // Configura como PWM
    gpio_set_function(BUZZER_2, GPIO_FUNC_PWM);

    uint slice1 = pwm_gpio_to_slice_num(BUZZER_1);
    uint slice2 = pwm_gpio_to_slice_num(BUZZER_2);

    pwm_set_enabled(slice1, true);
    pwm_set_enabled(slice2, true);
}


bool verificar_entrada() {
    return gpio_get(TECLA_A) == 0;
}

bool verificar_saida() {
    return gpio_get(TECLA_B) == 0;
}

void exibir_menu() {
    ssd1306_fill(&display, false);
    int pos_y_opcao1 = 20;
    int pos_y_opcao2 = 40;
    int faixa_y = (opcao_menu == 0) ? pos_y_opcao1 : pos_y_opcao2;
    ssd1306_rect(&display, faixa_y, 10, 110, 15, true, false);
    ssd1306_draw_string(&display, "MENU", 50, 5);

    if (opcao_menu == 0) {
        ssd1306_draw_string_inverted(&display, "1 LUMINOSIDADE", 10, pos_y_opcao1);
        ssd1306_draw_string(&display, "2 RUÍDO", 10, pos_y_opcao2);
    } else {
        ssd1306_draw_string(&display, "1 LUMINOSIDADE", 10, pos_y_opcao1);
        ssd1306_draw_string_inverted(&display, "2 RUÍDO", 10, pos_y_opcao2);
    }

    ssd1306_send_data(&display);
}

void exibir_luminosidade(int brilho, int valor_adc) {
    char buffer[50];
    int luminosidade = (valor_adc * 100) / 4095;
    
    ssd1306_fill(&display, false);
    sprintf(buffer, "LUZ ATUAL %d%%", luminosidade);
    ssd1306_draw_string(&display, buffer, 10, 10);

    sprintf(buffer, "BRILHO %d", brilho);
    ssd1306_draw_string(&display, buffer, 10, 25);
    ssd1306_send_data(&display);
}

// Função para tocar uma nota no buzzer
void play_tone(uint buzzer, int frequency, int duration, int volume) {
    if (frequency == 0) {
        sleep_ms(duration);
        return;
    }

    uint slice = pwm_gpio_to_slice_num(buzzer);
    pwm_set_clkdiv(slice, 4.0f);

    uint32_t wrap = clock_get_hz(clk_sys) / (frequency * 4);
    pwm_set_wrap(slice, wrap);

    uint16_t duty_cycle = (wrap * volume) / 100;
    pwm_set_gpio_level(buzzer, duty_cycle);
    pwm_set_enabled(slice, true);

    sleep_ms(duration);
    
    pwm_set_enabled(slice, false);
    sleep_ms(50);
}

// Função para emitir um alerta sonoro quando o ruído ultrapassar o limite
void emitir_som_alerta(uint buzzer) {
    const int frequencias[] = {1000, 1200, 800}; // Frequências alternadas para um som de alerta
    const int duracoes[] = {TEMPO_NOTA, TEMPO_NOTA, TEMPO_NOTA};

    for (int i = 0; i < 3; i++) {
        play_tone(buzzer, frequencias[i], duracoes[i], 80);
    }
}

void exibir_indicador_som(uint16_t mic_value) {
    static int tempo_alerta = 0; // Mantém o alerta ativo por alguns ciclos
    char buffer[50];
    ssd1306_fill(&display, false);

    // Conversão do ADC para dB (aproximado)
    float voltage = (mic_value / 4095.0) * 3.3;
    float dB = 20 * log10(voltage / 0.006);
    float limite_dB = 55;

    sprintf(buffer, "MIN %.1f dB", limite_dB);
    ssd1306_draw_string(&display, buffer, 10, 10);

    sprintf(buffer, "ATUAL %.1f dB", dB);
    ssd1306_draw_string(&display, buffer, 10, 25);

    // Se ultrapassar o limite, ativa o alerta e reinicia o tempo
    if (mic_value > LIMITE_SOM) {  
        tempo_alerta = 15;  // Mantém alerta por 15 ciclos (~1.5 segundos)
        emitir_som_alerta(BUZZER_1);
    }

    // Exibe alerta se o tempo ainda não zerou
    if (tempo_alerta > 0) {
        ssd1306_draw_string(&display, "ALERTA", 30, 40);
        ssd1306_draw_string(&display, "SOM ALTO", 30, 50);
        tempo_alerta--;  // Decrementa o contador do alerta
    }

    ssd1306_send_data(&display);
}


int main() {
    stdio_init_all();
    configurar_i2c();
    configurar_display();
    configurar_botoes();
    configurar_adc();
    configurar_buzzer();

    exibir_menu();

    while (1) {
        adc_select_input(0);
        int leitura_adc = adc_read();
        int nova_opcao = (leitura_adc > 2048) ? 1 : 0;

        if (nova_opcao != opcao_menu) {
            opcao_menu = nova_opcao;
            exibir_menu();
        }

        if (verificar_entrada()) {  
            if (opcao_menu == 0) {  // LUMINOSIDADE
                bool tarefa_luminancia_ativa = true;
                while (tarefa_luminancia_ativa) {
                    adc_select_input(0);
                    int leitura_adc = adc_read();
                    int brilho = 255 - (leitura_adc * 255 / 4095);
                    pwm_set_gpio_level(LED_PWM, brilho);
                    exibir_luminosidade(brilho, leitura_adc);

                    if (verificar_saida()) {
                        tarefa_luminancia_ativa = false;
                        exibir_menu();
                    }
                    sleep_ms(100);
                }
            } else if (opcao_menu == 1) {  // TESTE DE RUÍDO
                bool tarefa_som_ativa = true;
                while (tarefa_som_ativa) {
                    adc_select_input(MIC_ADC);
                    uint16_t mic_value = adc_read();
                    exibir_indicador_som(mic_value);

                    if (verificar_saida()) {
                        tarefa_som_ativa = false;
                        gpio_put(BUZZER_1, 0);
                        gpio_put(BUZZER_2, 0);
                        exibir_menu();
                    }
                    sleep_ms(100);
                }
            }
        }
        sleep_ms(100);
    }
}
