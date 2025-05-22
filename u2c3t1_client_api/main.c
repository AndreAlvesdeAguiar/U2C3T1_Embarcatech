#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/adc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// 

#include "hardware/i2c.h"
#include "inc/ssd1306.h"

#define I2C_SDA 14
#define I2C_SCL 15

// gpios
#define Button1 5
#define Button2 6

#define JOYSTICK_X 27
#define JOYSTICK_Y 26

#define LedRed  13
#define LedGreen  11
#define LedBlue 12

// dados do wifi
#define WIFI_SSID "AGUIA 2.4G"
#define WIFI_PASS "Leticia150789"
#define SERVER_IP "192.168.15.13" // IP do computador ipv4
#define SERVER_PORT 8080    // Porta do computador
#define API_ENDPOINT "/update" // Endpoint da API


char temperatura_mensagem[20] = "Temperatura: N/A";

int x = 0, y = 0; // Joystick X e Y inicializados

typedef struct { 
    struct tcp_pcb *pcb;
    char *data;
    u16_t len;
    bool connected;
} tcp_connection_t;

char oled_linha1[22] = "Wi-Fi: Conectando...";
char oled_linha2[22] = "";
char oled_linha3[22] = "";
char oled_linha4[22] = "";

struct render_area frame_area;
uint8_t ssd_buffer[ssd1306_buffer_length];

void atualizar_oled() {
    memset(ssd_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd_buffer, 0, 0, oled_linha1);
    ssd1306_draw_string(ssd_buffer, 0, 16, oled_linha2);
    ssd1306_draw_string(ssd_buffer, 0, 32, oled_linha3);
    ssd1306_draw_string(ssd_buffer, 0, 48, oled_linha4);
    render_on_display(ssd_buffer, &frame_area);
}
// Leitura da temperatura em Celsius
float ler_temperatura() {
    adc_select_input(4);
    uint16_t raw = adc_read();
    float voltagem = raw * 3.3f / 4095.0f;
    float temp_celsius = 27.0f - (voltagem - 0.706f) / 0.001721f;

    return (temp_celsius < -10.0f || temp_celsius > 100.0f) ? NAN : temp_celsius;
}

const char* gerar_rosa_dos_ventos(int x, int y) {
    if (x > 3000 && y > 3000) return "Nordeste";
    if (x > 3000 && y < 1000) return "Sudeste";
    if (x < 1000 && y > 3000) return "Noroeste";
    if (x < 1000 && y < 1000) return "Sudoeste";
    if (x > 3000) return "Leste";
    if (x < 1000) return "Oeste";
    if (y > 3000) return "Norte";
    if (y < 1000) return "Sul";
    return "Centro";
}


// função de callback -> recepção TCP
static err_t tcp_recv_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (!p) { // conexão fechada
        tcp_close(pcb);
        tcp_connection_t *conn = (tcp_connection_t *)arg;
        mem_free(conn->data);
        mem_free(conn);
        return ERR_OK;
    }
    
    printf("Resposta: %.*s\n", p->len, (char*)p->payload);
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

    // callback de envio TCP
static err_t tcp_sent_cb(void *arg, struct tcp_pcb *pcb, u16_t len) {
    printf("Dados enviados (%d bytes)\n", len);
    return ERR_OK;
}

    // callback de erro TCP
void tcp_error_cb(void *arg, err_t err) {
    printf("Erro TCP: %d\n", err);
}

    // callback de conexão TCP
static err_t tcp_connected_cb(void *arg, struct tcp_pcb *pcb, err_t err) {
    tcp_connection_t *conn = (tcp_connection_t *)arg;
    if (err != ERR_OK) { // erro na conexão
        printf("Erro na conexão: %d\n", err);
        tcp_abort(pcb);
        mem_free(conn->data);
        mem_free(conn);
        return err;
    }

    conn->connected = true; //conectado

    tcp_recv(pcb, tcp_recv_cb); // função de callback de recebimento
    tcp_sent(pcb, tcp_sent_cb); //

    err_t wr_err = tcp_write(pcb, conn->data, conn->len, TCP_WRITE_FLAG_COPY);
    if (wr_err != ERR_OK) { // erro ao escrever dados 
        printf("Erro ao escrever dados TCP: %d\n", wr_err);
        tcp_abort(pcb);
        mem_free(conn->data);
        mem_free(conn);
        return wr_err;
    }

    tcp_output(pcb);
    return ERR_OK;
}

void piscar_leds(); 
// Envia os dados em formato JSON para a API (Nest.js/Node.js)
void enviar_dados_api() {

    char json[512];
    char iso_date[40];

    bool b1 = !gpio_get(Button1);
    bool b2 = !gpio_get(Button2);

    float temp = ler_temperatura();

    adc_select_input(1); // Joystick eixo x adc 1
    x = adc_read();     
    adc_select_input(0); // Joystick eixo y adc 0
    y = adc_read();
    
    if(x<0 || x>4095) x=0; // Limita o valor de x entre 0 e 4095
    if(y<0 || y>4095) y=0;  // Limita o valor de y entre 0 e 4095
    
    const char *rosa_dos_ventos = gerar_rosa_dos_ventos(x, y);
    
    // Formata JSON 
    snprintf(json, sizeof(json),
        "{"
        "\"button1\": \"%s\", "
        "\"button2\": \"%s\", "
        "\"joystickX\": %d, "
        "\"joystickY\": %d, "
        "\"rosa_dos_ventos\": \"%s\", "
        "\"temperature\": %.2f"
        "}",
        b1 ? "pressionado" : "solto",
        b2 ? "pressionado" : "solto",
        x, y, rosa_dos_ventos, temp
    ); 

    int json_len = strlen(json); // Tamanho da string JSON
    printf("\nJSON:\n%s\n", json); // Imprime o JSON no console (serial monitor)

    char request[1024]; // Buffer para a requisição HTTP
    int req_len = snprintf(request, sizeof(request), // Formata a requisição HTTP
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n\r\n%s",
        API_ENDPOINT, SERVER_IP, SERVER_PORT, json_len, json);

    ip_addr_t ip;
    ipaddr_aton(SERVER_IP, &ip);
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;

    tcp_connection_t *conn = (tcp_connection_t *)mem_malloc(sizeof(tcp_connection_t));
    if (!conn) { // erro na alocação de memória de conn
        tcp_abort(pcb);
        return;
    }

    // Aloca memória para os dados da requisição
    conn->data = (char *)mem_malloc(req_len);
    memcpy(conn->data, request, req_len);
    conn->len = req_len;
    conn->pcb = pcb;
    conn->connected = false;


    tcp_arg(pcb, conn);
    tcp_err(pcb, tcp_error_cb);

    err_t err = tcp_connect(pcb, &ip, SERVER_PORT, tcp_connected_cb);
    if (err != ERR_OK) {
        printf("Erro ao conectar: %d\n", err);
        mem_free(conn->data);
        mem_free(conn);
        tcp_abort(pcb);
    }
    //quando o json for enviado, piscar o led azul
    piscar_leds();
}


// Lê os sensores e atualiza as variáveis globais
void monitor_sensors() {
    bool b1 = !gpio_get(Button1);// Botão 1 pino 5 
    bool b2 = !gpio_get(Button2);// Botão 2 pino 6

    adc_select_input(1); // Joystick eixo x adc 1
    x = adc_read();
    adc_select_input(0); // Joystick eixo y adc 0
    y = adc_read();


    float temp = ler_temperatura();
    if (!isnan(temp)) {
        snprintf(temperatura_mensagem, sizeof(temperatura_mensagem), "%.2f °C", temp);
    } else {
        snprintf(temperatura_mensagem, sizeof(temperatura_mensagem), "Erro de leitura");
    }

    printf("---------------------------------------------\n");
    printf("\t\t Botão 1: %s\n", b1 ? "pressionado" : "solto");
    printf("\t\t Botão 2: %s\n", b2 ? "pressionado" : "solto");
    printf("\t\t Joystick X: %d\n", x);
    printf("\t\t Joystick Y: %d\n", y);
    printf("\t\t Rosa dos Ventos: %s\n", gerar_rosa_dos_ventos(x, y));
    printf("\t\t Temperatura: %s\n", temperatura_mensagem);
    printf("---------------------------------------------\n");
}

void setup(){ //função de configuração dos pinos 

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);
    atualizar_oled();

        
    gpio_init(Button1); 
    gpio_set_dir(Button1, GPIO_IN); 
    gpio_pull_up(Button1);

    gpio_init(Button2); 
    gpio_set_dir(Button2, GPIO_IN); 
    gpio_pull_up(Button2);

    adc_init();
    adc_set_temp_sensor_enabled(true); // ATIVA sensor interno
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    
    gpio_init(LedRed);
    gpio_set_dir(LedRed, GPIO_OUT);

    gpio_init(LedGreen);
    gpio_set_dir(LedGreen, GPIO_OUT);

    gpio_init(LedBlue);
    gpio_set_dir(LedBlue, GPIO_OUT);
}
void acender_led( uint gpio,int led) { // Função para acender o LED
   if(led==1)gpio_put(gpio, led == 1);
    else gpio_put(gpio, 0);
}
void piscar_leds() { // Função para piscar o LED
    gpio_put(LedRed, 1);
    gpio_put(LedBlue,1);
    gpio_put(LedGreen, 1);
    sleep_ms(200);
    gpio_put(LedRed, 0);
    gpio_put(LedGreen, 0);
    gpio_put(LedBlue, 0);
}
// Função principal
int main() {

    stdio_init_all();  
    setup();

    sleep_ms(2000);

    if (cyw43_arch_init()) return -1; // Inicializa o WiFi e finaliza se der erro
    cyw43_arch_enable_sta_mode(); // ativa modo cliente

    acender_led(LedRed,1); // Acende o LED vermelho pois a conexão não foi estabelecida

    while (
        cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) != 0  // conecta ao WiFi
    ) {
        printf("Tentando conectar ao WiFi...\n");
        
        sleep_ms(5000);
    }

    acender_led(LedRed,0); // Apaga o LED vermelho

    printf("WiFi conectado. IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    ip4_addr_t ip = *netif_ip4_addr(netif_list);
    snprintf(oled_linha1, sizeof(oled_linha1), "1921681513:8080");
    atualizar_oled();

    while (1) {
        monitor_sensors();
        enviar_dados_api();

        snprintf(oled_linha2, sizeof(oled_linha2), "B1:%s B2:%s", 
                gpio_get(Button1) ? "OFF" : "ON", 
                gpio_get(Button2) ? "OFF" : "ON");

        snprintf(oled_linha3, sizeof(oled_linha3), "Joy:%s", gerar_rosa_dos_ventos(x, y));
        snprintf(oled_linha4, sizeof(oled_linha4), "Temp: %.1f C", ler_temperatura());

        atualizar_oled();
 
        sleep_ms(1000);
    }

    cyw43_arch_deinit();
    return 0;
}
