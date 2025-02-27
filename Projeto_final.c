// ===============================
// Inclusão de Bibliotecas
// ===============================

// Bibliotecas padrão do C
#include <stdio.h>       // Biblioteca para entrada/saída padrão (printf, scanf, etc.)
#include <math.h>        // Biblioteca para funções matemáticas (log, pow, etc.)

// Bibliotecas do Raspberry Pi Pico
#include "pico/stdlib.h"    // Funções padrão para GPIO, tempo e comunicação serial
#include "hardware/i2c.h"    // Biblioteca para comunicação via protocolo I2C
#include "hardware/adc.h"    // Biblioteca para leitura de valores analógicos (ADC)
#include "hardware/clocks.h" // Biblioteca para gerenciamento de clocks
#include "hardware/pwm.h"    // Biblioteca para controle de saída PWM

// Bibliotecas para controle do display OLED
#include "include/ssd1306.h" // Controlador do display OLED SSD1306
#include "include/font.h"    // Biblioteca de fontes para renderização de texto no display

// ===============================
// Definição de Pinos e Parâmetros
// ===============================

// Sensores e atuadores conectados aos pinos GPIO do Raspberry Pi Pico
#define POTENCIOMETRO 26  // Entrada ADC para medir a posição do potenciômetro
#define LED_PWM 13        // Saída PWM para controlar o brilho de um LED

// Configuração do barramento I2C (usado pelo display OLED)
#define I2C_PORT i2c1     // Porta I2C utilizada (i2c1)
#define I2C_SDA 14        // Pino SDA para comunicação I2C
#define I2C_SCL 15        // Pino SCL para comunicação I2C
#define ENDERECO_DISPLAY 0x3C // Endereço I2C do display OLED SSD1306

// Pinos dos botões de controle
#define TECLA_A 5  // Botão para avançar/interagir com o menu
#define TECLA_B 6  // Botão para voltar/sair do menu

// Pino do microfone usado para medir nível de som
#define MIC_ADC 2  // Entrada ADC para capturar o sinal do microfone

// Pinos dos buzzers usados para emitir sons de alerta
#define BUZZER_1 10 // Buzzer 1 controlado por PWM
#define BUZZER_2 21 // Buzzer 2 controlado por PWM

// Definição do limite de ruído para ativação do alerta
#define LIMITE_SOM 2600  // Valor do ADC acima do qual o alerta sonoro será ativado

// Tempo de duração das notas musicais emitidas pelo buzzer (em milissegundos)
#define TEMPO_NOTA 100  // Define que cada nota sonora dura 100ms

// ===============================
// Variáveis Globais
// ===============================

// Objeto para armazenar o estado do display OLED
ssd1306_t display;

// Variável global para armazenar a opção atualmente selecionada no menu
int opcao_menu = 0;  // 0 = Modo de Luminosidade, 1 = Modo de Teste de Ruído

// ===============================
// Funções de Configuração
// ===============================

// Configura a interface I2C para comunicação com o display OLED
void configurar_i2c() {
    i2c_init(I2C_PORT, 400 * 1000); // Inicializa a comunicação I2C com frequência de 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define o pino SDA como função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define o pino SCL como função I2C
    gpio_pull_up(I2C_SDA); // Habilita resistor de pull-up no pino SDA
    gpio_pull_up(I2C_SCL); // Habilita resistor de pull-up no pino SCL
}

// Inicializa e configura o display OLED SSD1306 via I2C
void configurar_display() {
    ssd1306_init(&display, 128, 64, false, ENDERECO_DISPLAY, I2C_PORT); // Configura o display para resolução 128x64
    ssd1306_config(&display); // Aplica configurações padrão ao display
    sleep_ms(200); // Pequeno atraso para garantir a inicialização correta do display
    ssd1306_fill(&display, false); // Limpa a tela do display
    ssd1306_send_data(&display); // Atualiza o display com a tela limpa
}

// Configura o ADC para leitura dos sensores analógicos (potenciômetro e microfone)
void configurar_adc() {
    adc_init(); // Inicializa o conversor analógico-digital (ADC)
    adc_gpio_init(POTENCIOMETRO); // Habilita leitura do potenciômetro no pino ADC
    adc_gpio_init(MIC_ADC); // Habilita leitura do microfone no pino ADC
}

// Configura os botões como entradas digitais com pull-up ativado
void configurar_botoes() {
    gpio_init(TECLA_A); // Inicializa o pino do botão A
    gpio_init(TECLA_B); // Inicializa o pino do botão B
    gpio_set_dir(TECLA_A, GPIO_IN); // Define o botão A como entrada
    gpio_set_dir(TECLA_B, GPIO_IN); // Define o botão B como entrada
    gpio_pull_up(TECLA_A); // Ativa resistor pull-up interno para o botão A
    gpio_pull_up(TECLA_B); // Ativa resistor pull-up interno para o botão B
}

// Configura os buzzers para emissão de som usando PWM
void configurar_buzzer() {
    gpio_set_function(BUZZER_1, GPIO_FUNC_PWM); // Define o buzzer 1 como saída PWM
    gpio_set_function(BUZZER_2, GPIO_FUNC_PWM); // Define o buzzer 2 como saída PWM

    uint slice1 = pwm_gpio_to_slice_num(BUZZER_1); // Obtém o slice PWM do buzzer 1
    uint slice2 = pwm_gpio_to_slice_num(BUZZER_2); // Obtém o slice PWM do buzzer 2

    pwm_set_enabled(slice1, true); // Habilita PWM no buzzer 1
    pwm_set_enabled(slice2, true); // Habilita PWM no buzzer 2
}

// ===============================
// Funções de Entrada
// ===============================

// Verifica se o botão A foi pressionado
bool verificar_entrada() {
    return gpio_get(TECLA_A) == 0; // Retorna verdadeiro se o botão A estiver pressionado
}

// Verifica se o botão B foi pressionado
bool verificar_saida() {
    return gpio_get(TECLA_B) == 0; // Retorna verdadeiro se o botão B estiver pressionado
}

// ===============================
// Funções de Exibição no Display OLED
// ===============================

// Exibe o menu de opções no display OLED
void exibir_menu() {
    ssd1306_fill(&display, false); // Limpa o display antes de desenhar o menu

    int pos_y_opcao1 = 20; // Posição vertical da opção "LUMINOSIDADE"
    int pos_y_opcao2 = 40; // Posição vertical da opção "RUÍDO"

    // Determina a posição da seleção (marca visual que indica qual opção está selecionada)
    int faixa_y = (opcao_menu == 0) ? pos_y_opcao1 : pos_y_opcao2;
    
    // Desenha um retângulo ao redor da opção selecionada para destacá-la
    ssd1306_rect(&display, faixa_y, 10, 110, 15, true, false);

    // Exibe o título do menu
    ssd1306_draw_string(&display, "MENU", 50, 5);

    // Exibe as opções do menu, destacando a selecionada
    if (opcao_menu == 0) {
/*         ssd1306_draw_string_inverted(&display, "1 LUMINOSIDADE", 10, pos_y_opcao1);
 */        ssd1306_draw_string(&display, "2 RUIDO", 10, pos_y_opcao2);
    } else {
        ssd1306_draw_string(&display, "1 LUMINOSIDADE", 10, pos_y_opcao1);
/*         ssd1306_draw_string_inverted(&display, "2 RUIDO", 10, pos_y_opcao2);
 */    }

    ssd1306_send_data(&display); // Atualiza o display com as novas informações
}

// ===============================
// Funções para Controle da Luminosidade e Exibição no Display OLED
// ===============================

// Exibe no display OLED o nível de luminosidade medido e o brilho ajustado do LED
void exibir_luminosidade(int brilho, int valor_adc) {
    char buffer[50]; // Buffer para armazenar strings formatadas

    // Converte a leitura do ADC (0-4095) para um valor percentual (0-100%)
    int luminosidade = (valor_adc * 100) / 4095;

    // Limpa a tela do display antes de exibir novos dados
    ssd1306_fill(&display, false);

    // Exibe a luminosidade medida em porcentagem
    sprintf(buffer, "LUZ ATUAL %d%%", luminosidade);
    ssd1306_draw_string(&display, buffer, 10, 10);

    // Exibe o nível de brilho do LED (controlado via PWM)
    sprintf(buffer, "BRILHO %d", brilho);
    ssd1306_draw_string(&display, buffer, 10, 25);

    // Atualiza o display com as novas informações
    ssd1306_send_data(&display);
}

// ===============================
// Funções para Emissão de Sons e Alertas
// ===============================

// Gera um tom sonoro no buzzer por um determinado tempo e volume
void play_tone(uint buzzer, int frequency, int duration, int volume) {
    // Se a frequência for 0, apenas aguarda o tempo especificado sem emitir som
    if (frequency == 0) {
        sleep_ms(duration);
        return;
    }

    // Obtém o slice PWM correspondente ao pino do buzzer
    uint slice = pwm_gpio_to_slice_num(buzzer);

    // Define a taxa de divisão do clock para ajuste da frequência PWM
    pwm_set_clkdiv(slice, 4.0f);

    // Calcula o valor de wrap para gerar a frequência desejada
    uint32_t wrap = clock_get_hz(clk_sys) / (frequency * 4);
    pwm_set_wrap(slice, wrap);

    // Define o duty cycle do PWM com base no volume desejado (escala de 0 a 100)
    uint16_t duty_cycle = (wrap * volume) / 100;
    pwm_set_gpio_level(buzzer, duty_cycle);
    
    // Ativa o PWM para gerar o som
    pwm_set_enabled(slice, true);

    // Aguarda o tempo da nota antes de desligar o som
    sleep_ms(duration);
    
    // Desativa o PWM (silencia o buzzer)
    pwm_set_enabled(slice, false);
    sleep_ms(50); // Pequeno delay para evitar ruídos indesejados entre notas
}

// Emite um alerta sonoro quando o ruído captado pelo microfone ultrapassa o limite definido
void emitir_som_alerta(uint buzzer) {
    // Sequência de frequências para criar um som de alerta distinto
    const int frequencias[] = {1000, 1200, 800};
    const int duracoes[] = {TEMPO_NOTA, TEMPO_NOTA, TEMPO_NOTA};

    // Toca as três notas da sequência de alerta
    for (int i = 0; i < 3; i++) {
        play_tone(buzzer, frequencias[i], duracoes[i], 80);
    }
}

// ===============================
// Função para Monitoramento de Ruído e Exibição no Display OLED
// ===============================

// Exibe o nível de som captado pelo microfone e ativa alerta caso o volume seja alto
void exibir_indicador_som(uint16_t mic_value) {
    static int tempo_alerta = 0; // Variável que mantém o alerta ativo por um tempo determinado
    char buffer[50]; // Buffer para strings de exibição
    ssd1306_fill(&display, false); // Limpa o display antes de atualizar as informações

    // Conversão da leitura do ADC (0-4095) para uma estimativa de nível em decibéis (dB)
    float voltage = (mic_value / 4095.0) * 3.3; // Converte para tensão (0 a 3.3V)
    float dB = 20 * log10(voltage / 0.006); // Aproximação do nível de som em decibéis
    float limite_dB = 55; // Limite mínimo de som considerado "alto"

    // Exibe no display o limite estabelecido
    sprintf(buffer, "MIN %.1f dB", limite_dB);
    ssd1306_draw_string(&display, buffer, 10, 10);

    // Exibe o nível de som captado pelo microfone
    sprintf(buffer, "ATUAL %.1f dB", dB);
    ssd1306_draw_string(&display, buffer, 10, 25);

    // Se o som ultrapassar o limite, ativa o alerta e reinicia o contador do tempo do alerta
    if (mic_value > LIMITE_SOM) {  
        tempo_alerta = 15; // Mantém o alerta ativo por 15 ciclos (~1.5 segundos)
        emitir_som_alerta(BUZZER_1); // Emite som de alerta no buzzer
    }

    // Se o alerta estiver ativo, exibe mensagem de alerta no display
    if (tempo_alerta > 0) {
        ssd1306_draw_string(&display, "ALERTA", 30, 40);
        ssd1306_draw_string(&display, "SOM ALTO", 30, 50);
        tempo_alerta--; // Reduz o tempo restante do alerta
    }

    // Atualiza o display OLED com as novas informações
    ssd1306_send_data(&display);
}

// ===============================
// Função Principal (Loop do Programa)
// ===============================

int main() {
    // Inicializa a comunicação serial para debug
    stdio_init_all();

    // Configura os periféricos do Raspberry Pi Pico
    configurar_i2c();
    configurar_display();
    configurar_botoes();
    configurar_adc();
    configurar_buzzer();

    // Exibe o menu inicial no display
    exibir_menu();

    // Loop infinito do programa
    while (1) {
        // Lê o valor do joystick/potenciômetro
        adc_select_input(0);
        int leitura_adc = adc_read();
    
        // Definição da zona morta (para evitar mudanças involuntárias)
        int limite_superior = 3000; // Se o ADC passar desse valor, seleciona "LUMINOSIDADE"
        int limite_inferior = 1000; // Se o ADC for menor que isso, seleciona "RUÍDO"
    
        int nova_opcao = opcao_menu; // Mantém a opção atual por padrão
    
        // Apenas altera a opção se o joystick ultrapassar os limites estabelecidos
        if (leitura_adc > limite_superior) {
            nova_opcao = 0; // Seleciona "LUMINOSIDADE"
        } else if (leitura_adc < limite_inferior) {
            nova_opcao = 1; // Seleciona "RUÍDO"
        }
    
        // Se a opção realmente mudou, atualiza o menu no display
        if (nova_opcao != opcao_menu) {
            opcao_menu = nova_opcao;
            exibir_menu();
        }
    
        // Verifica se o botão A foi pressionado para entrar no modo selecionado
        if (verificar_entrada()) {  
            if (opcao_menu == 0) {  // MODO: LUMINOSIDADE
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
            } else if (opcao_menu == 1) {  // MODO: TESTE DE RUÍDO
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
