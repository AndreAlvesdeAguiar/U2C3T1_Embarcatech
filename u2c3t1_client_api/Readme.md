# Placa BITDOGLAB com Microcontrolador Raspberry Pi Pico W

Este projeto implementa a comunicação IoT utilizando a placa **Bitdoglab** com o microcontrolador **Raspberry Pi Pico W**. Ele realiza a leitura de sensores como botões, joystick e sensor de temperatura, além de enviar os dados coletados para um servidor via conexão Wi-Fi utilizando o protocolo TCP.

---

## 📋 Funcionalidades

- Leitura dos estados de dois botões conectados à placa.
- Leitura das coordenadas X e Y de um joystick analógico.
- Geração de uma rosa dos ventos com base nas coordenadas do joystick.
- Leitura da temperatura por meio do sensor interno do Raspberry Pi Pico W.
- Envio dos dados coletados em formato JSON para um servidor específico.
- Indicação do status de conexão utilizando LEDs (vermelho, verde e azul).

---

## 🗂️ Estrutura do Código

### **Componentes Utilizados**
- **Botões:** Detecta o estado (pressionado ou solto) dos botões conectados aos pinos GPIO 5 e 6.
- **Joystick:** Lê as coordenadas X (pino GPIO 27) e Y (pino GPIO 26) usando o ADC.
- **Sensor de Temperatura:** Utiliza o sensor interno do Raspberry Pi Pico W.
- **LEDs RGB:** Indica o status das operações:
  - **Vermelho:** Indica falha na conexão Wi-Fi.
  - **Azul:** Pisca ao enviar dados ao servidor.
  - **Verde:** Indica operação normal.

---

## 🚀 Como Configurar

### **1. Pré-requisitos**
- **Hardware:**
  - Placa Bitdoglab
  - Raspberry Pi Pico W
  - Botões e joystick conectados aos pinos correspondentes
  - LEDs RGB conectados aos pinos GPIO 13 (vermelho), 11 (verde) e 12 (azul)
- **Software:**
  - SDK do Raspberry Pi Pico configurado em sua máquina
  - Compilador C (como GCC)
  - Biblioteca **pico-sdk** e **lwIP** para comunicação TCP
  - Conexão Wi-Fi com acesso ao servidor

### **2. Configuração do Código**
- Atualize as seguintes constantes no código para suas configurações de Wi-Fi e servidor:
  ```c
  #define WIFI_SSID "nome_wifi"       // Nome da sua rede Wi-Fi
  #define WIFI_PASS "senha_wifi"     // Senha da sua rede Wi-Fi
  #define SERVER_IP "endereço_do_servidor" // IP do servidor
  #define SERVER_PORT 3000           // Porta do servidor
  #define API_ENDPOINT ""            // Endpoint da API
  ```

### **3. Compilação e Upload**
1. Compile o código utilizando o SDK do Raspberry Pi Pico.
2. Carregue o firmware gerado na Raspberry Pi Pico W.

---

## 🛠️ Funcionalidades

### **1. Monitoramento dos Sensores**
- Leitura dos botões:
  - **Botão 1 (GPIO 5):** Estado "pressionado" ou "solto".
  - **Botão 2 (GPIO 6):** Estado "pressionado" ou "solto".
- Leitura do joystick:
  - Coordenadas **X** (GPIO 27) e **Y** (GPIO 26).
  - Geração de uma **rosa dos ventos** com base nas coordenadas:
    - Norte, Sul, Leste, Oeste, Nordeste, Sudeste, Noroeste, Sudoeste ou Centro.
- Leitura da temperatura:
  - Utiliza o sensor interno do Raspberry Pi Pico W.
  - Retorna a temperatura em graus Celsius.

### **2. Envio de Dados para o Servidor**
- Os dados coletados são enviados para o servidor em formato JSON, conforme o exemplo abaixo:
  ```json
  {
    "button1": "pressionado",
    "button2": "solto",
    "joystickX": 2048,
    "joystickY": 1024,
    "rosa_dos_ventos": "Nordeste",
    "temperature": 25.67
  }
  ```
- O envio ocorre a cada 1 segundo, utilizando o protocolo TCP.

### **3. Indicação com LEDs**
- O LED **vermelho** acende quando a conexão Wi-Fi falha.
- O LED **azul** pisca toda vez que os dados são enviados ao servidor.
- O LED **verde** indica que o dispositivo está funcionando normalmente.

---

## 🌐 Referências de Pinos

| Componente         | Pino GPIO |
|---------------------|-----------|
| Botão 1            | GPIO 5    |
| Botão 2            | GPIO 6    |
| Joystick X         | GPIO 27   |
| Joystick Y         | GPIO 26   |
| LED Vermelho       | GPIO 13   |
| LED Verde          | GPIO 11   |
| LED Azul           | GPIO 12   |

---

## 🔧 Melhorias Futuais
- Adicionar suporte a mais sensores (como umidade ou luminosidade).
- Criar uma interface web para visualizar os dados em tempo real.
- Adicionar uma funcionalidade de reconexão automática em caso de falha no Wi-Fi.

---

## 🤝 Contribuições
Contribuições são bem-vindas! Caso tenha sugestões ou melhorias, sinta-se à vontade para abrir issues ou enviar pull requests.

---

## 🧑‍🏫 Créditos
Este projeto foi desenvolvido como parte de práticas IoT utilizando a placa **Bitdoglab** com o **Raspberry Pi Pico W**.