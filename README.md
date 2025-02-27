# Monitor de Luminosidade e Ruído com Raspberry Pi Pico  

Este projeto implementa um sistema de monitoramento de **luminosidade** e **nível de ruído** utilizando um **Raspberry Pi Pico**. O usuário pode alternar entre os modos de operação utilizando botões físicos, e os dados são exibidos em um display **OLED SSD1306** via **I2C**. O sistema também possui um **buzzer** para alertas de ruído alto.

---

## 📌 Funcionalidades  

- **Modo Luminosidade:**  
  - Mede a luminosidade ambiente usando um potenciômetro.  
  - Ajusta o brilho de um LED PWM proporcionalmente.  
  - Exibe os valores no **display OLED**.  

- **Modo Ruído:**  
  - Mede o nível de som via **microfone** conectado ao ADC.  
  - Exibe a intensidade do som em **decibéis (dB)** no display.  
  - Ativa um **alerta sonoro** no buzzer caso o ruído ultrapasse o limite.  

- **Interface com botões físicos:**  
  - Botão **A** troca entre os modos.  
  - Botão **B** sai do modo ativo e retorna ao menu principal.  

---

## 🖥️ Hardware Utilizado  

| Componente        | Função |
|------------------|--------|
| Raspberry Pi Pico | Microcontrolador principal |
| Display OLED SSD1306 | Exibição de informações |
| Potenciômetro (ADC) | Controle de luminosidade |
| Microfone (ADC) | Medição do nível de som |
| LED PWM | Indicador de luminosidade |
| Buzzer (PWM) | Alerta sonoro de ruído alto |
| Botões físicos | Controle de menu |

---

## 🔌 Conexões  

### 📡 **I2C (Display OLED SSD1306)**  

| Pino Pico | Componente |
|-----------|-----------|
| **GP14** | SDA (Display) |
| **GP15** | SCL (Display) |

### 🎛 **Potenciômetro (Luminosidade)**  

| Pino Pico | Função |
|-----------|--------|
| **GP26 (ADC0)** | Sinal analógico |

### 🎤 **Microfone (Medição de ruído)**  

| Pino Pico | Função |
|-----------|--------|
| **GP28 (ADC2)** | Entrada analógica |

### 🔊 **Buzzer (Alerta Sonoro)**  

| Pino Pico | Função |
|-----------|--------|
| **GP10** | PWM do buzzer 1 |
| **GP21** | PWM do buzzer 2 |

### 💡 **LED PWM (Indicador de brilho)**  

| Pino Pico | Função |
|-----------|--------|
| **GP13** | Controle de brilho (PWM) |

### 🔘 **Botões**  

| Pino Pico | Botão |
|-----------|--------|
| **GP5** | Botão A (Selecionar) |
| **GP6** | Botão B (Voltar) |

---

## 🛠️ Como Compilar e Executar  

### **1️⃣ Configurar o Ambiente**  

Certifique-se de ter instalado:  
- **Raspberry Pi Pico SDK**  
- **Compilador ARM (arm-none-eabi-gcc)**  
- **CMake e Ninja**  

### **2️⃣ Compilar o Código**  

```sh
mkdir build
cd build
cmake ..
make
```

### **3️⃣ Carregar no Raspberry Pi Pico**  

- Pressione e segure o **BOOTSEL** no Raspberry Pi Pico.  
- Conecte o cabo USB ao PC.  
- Arraste o arquivo `monitor.uf2` gerado para a unidade **RPI-RP2**.  
- O código será carregado automaticamente.  

---

## 📜 Funcionamento  

### 🔷 **1. Inicialização**  
- O display é inicializado.  
- O menu principal é exibido.  

### 🔷 **2. Alternância entre modos**  
- Pressione **Botão A** para alternar entre:  
  - **Modo Luminosidade**  
  - **Modo Ruído**  
- Pressione **Botão B** para voltar ao menu.  

### 🔷 **3. Modo Luminosidade**  
- Mede o valor do potenciômetro via **ADC**.  
- Ajusta a intensidade do LED via **PWM**.  
- Exibe o brilho e luminosidade no **display OLED**.  

### 🔷 **4. Modo Ruído**  
- Mede o nível de ruído via microfone.  
- Converte o sinal em **dB** e exibe no **display OLED**.  
- Se ultrapassar o limite, toca um alerta no **buzzer**.  


## 🏆 Melhorias Futuras  

- [ ] Adicionar interface gráfica mais elaborada.  
- [ ] Implementar ajustes de limite de ruído pelo usuário.  
- [ ] Salvar configurações na memória flash do Pico.  
