#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/pwm.h"

#include "inc/ssd1306.h"
#include "hardware/i2c.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define LED_PIN 12
#define BUTTON1_PIN 5
#define BUTTON2_PIN 6
#define VRX 26
#define VRY 27
#define SW 22
#define WIFI_SSID "AGUIA 2.4G"
#define WIFI_PASS "Leticia150789"

char button1_message[50] = "Nenhum evento no botao 1";
char button2_message[50] = "Nenhum evento no botao 2";
char joystick_message[50] = "Centro";
char http_response[2048];

struct tcp_pcb *sse_client_pcb = NULL;

void setup_joystick() {
    adc_gpio_init(VRX);
    adc_gpio_init(VRY);
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
    adc_init();
}

void read_joystick(uint16_t *x, uint16_t *y, bool *sw) {
    adc_select_input(0);
    sleep_us(2);
    *x = adc_read();
    adc_select_input(1);
    sleep_us(2);
    *y = adc_read();
    *sw = !gpio_get(SW);
}

void compute_direction(uint16_t x, uint16_t y) {
    char last_direction[50];
    strcpy(last_direction, joystick_message);

    if (x < 1000 && y > 3000)
        strcpy(joystick_message, "Sudeste");
    else if (x > 3000 && y > 3000)
        strcpy(joystick_message, "Nordeste");
    else if (x < 1000 && y < 1000)
        strcpy(joystick_message, "Sudoeste");
    else if (x > 3000 && y < 1000)
        strcpy(joystick_message, "Noroeste");
    else if (x < 1000)
        strcpy(joystick_message, "Sul");
    else if (x > 3000)
        strcpy(joystick_message, "Norte");
    else if (y > 3000)
        strcpy(joystick_message, "Leste");
    else if (y < 1000)
        strcpy(joystick_message, "Oeste");
    else
        strcpy(joystick_message, "Centro");

    if (strcmp(joystick_message, last_direction) != 0) {
        printf("Direcao do joystick: %s\n", joystick_message);
    }
}

float read_temperature() {
    adc_select_input(4);
    uint16_t raw = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    return 27 - (voltage - 0.706f) / 0.001721f;
}

void create_http_response() {
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Controle SSE</title>"
        "<script>"
        "const evt = new EventSource('/events');"
        "evt.onmessage = e => {"
        "const d = JSON.parse(e.data);"
        "document.getElementById('botao1').innerText = d.botao1;"
        "document.getElementById('botao2').innerText = d.botao2;"
        "document.getElementById('temperatura').innerText = d.temperatura.toFixed(2) + ' °C';"
        "document.getElementById('joystick').innerText = d.joystick;"
        "};"
        "</script></head><body>"
        "<h1>Servidor com SSE</h1>"
        "<p><a href='/led/on'>Ligar LED</a> | <a href='/led/off'>Desligar LED</a></p>"
        "<h2>Estado dos Botoes:</h2>"
        "<p id='botao1'>Carregando...</p>"
        "<p id='botao2'>Carregando...</p>"
        "<h2>Temperatura Interna:</h2>"
        "<p id='temperatura'>Carregando...</p>"
        "<h2>Direcao Joystick:</h2>"
        "<p id='joystick'>Carregando...</p>"
        "</body></html>");
}

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    if (strstr(request, "GET /led/on")) {
        gpio_put(LED_PIN, 1);
    } else if (strstr(request, "GET /led/off")) {
        gpio_put(LED_PIN, 0);
    } else if (strstr(request, "GET /events")) {
        const char *sse_headers =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: keep-alive\r\n\r\n";

        tcp_write(tpcb, sse_headers, strlen(sse_headers), TCP_WRITE_FLAG_COPY);
        sse_client_pcb = tpcb;
        pbuf_free(p);
        return ERR_OK;
    }

    create_http_response();
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);
    pbuf_free(p);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

void monitor_buttons() {
    static bool button1_last_state = false;
    static bool button2_last_state = false;

    bool button1_state = !gpio_get(BUTTON1_PIN);
    bool button2_state = !gpio_get(BUTTON2_PIN);

    if (button1_state != button1_last_state) {
        button1_last_state = button1_state;
        snprintf(button1_message, sizeof(button1_message), button1_state ? "Botao 1 pressionado!" : "Botao 1 solto!");
        printf("%s\n", button1_message);
    }

    if (button2_state != button2_last_state) {
        button2_last_state = button2_state;
        snprintf(button2_message, sizeof(button2_message), button2_state ? "Botao 2 pressionado!" : "Botao 2 solto!");
        printf("%s\n", button2_message);
    }
}

void update_oled_display(uint8_t *ssd, struct render_area *frame_area, float temp) {
    memset(ssd, 0, ssd1306_buffer_length);

    char linha_b1[22];
    snprintf(linha_b1, sizeof(linha_b1), "B1: %s", button1_message);
    ssd1306_draw_string(ssd, 0, 5, linha_b1);

    char linha_b2[22];
    snprintf(linha_b2, sizeof(linha_b2), "B2: %s", button2_message);
    ssd1306_draw_string(ssd, 0, 20, linha_b2);

    char linha_joy[22];
    snprintf(linha_joy, sizeof(linha_joy), "Joy: %s", joystick_message);
    ssd1306_draw_string(ssd, 0, 35, linha_joy);

    char linha_temp[22];
    snprintf(linha_temp, sizeof(linha_temp), "Temp: %.1f C", temp);
    ssd1306_draw_string(ssd, 0, 50, linha_temp);

    render_on_display(ssd, frame_area);
}

// ... [todo o código acima permanece igual até a linha do `main()`]

int main() {
    stdio_init_all();
    sleep_ms(1000);
    printf("Iniciando servidor HTTP\n");

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    } else {
        printf("Conectado ao Wi-Fi\n");
        uint8_t *ip = (uint8_t *)&cyw43_state.netif[0].ip_addr.addr;
        printf("IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    }

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(BUTTON1_PIN);
    gpio_set_dir(BUTTON1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON1_PIN);
    gpio_init(BUTTON2_PIN);
    gpio_set_dir(BUTTON2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON2_PIN);

    setup_joystick();
    adc_set_temp_sensor_enabled(true);

    start_http_server();

    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    absolute_time_t last_sse_send = get_absolute_time();
    float temp = 0.0f;

    // variáveis para comparar mudanças
    char last_button1[50] = "";
    char last_button2[50] = "";
    char last_joystick[50] = "";
    float last_temp = -1000.0f;

    while (true) {
        cyw43_arch_poll();
        monitor_buttons();

        uint16_t x, y;
        bool sw;
        read_joystick(&x, &y, &sw);
        compute_direction(x, y);
        temp = read_temperature();

        if (strcmp(last_button1, button1_message) != 0 ||
            strcmp(last_button2, button2_message) != 0 ||
            strcmp(last_joystick, joystick_message) != 0 ||
            fabsf(temp - last_temp) >= 0.5f) {

            update_oled_display(ssd, &frame_area, temp);
            strcpy(last_button1, button1_message);
            strcpy(last_button2, button2_message);
            strcpy(last_joystick, joystick_message);
            last_temp = temp;
        }

        if (sse_client_pcb && absolute_time_diff_us(last_sse_send, get_absolute_time()) > 1000000) {
            last_sse_send = get_absolute_time();
            char sse_msg[512];
            snprintf(sse_msg, sizeof(sse_msg),
                "data: {\"botao1\": \"%s\", \"botao2\": \"%s\", \"temperatura\": %.2f, \"joystick\": \"%s\"}\n\n",
                button1_message, button2_message, temp, joystick_message);
            err_t err = tcp_write(sse_client_pcb, sse_msg, strlen(sse_msg), TCP_WRITE_FLAG_COPY);
            if (err != ERR_OK) {
                printf("Erro SSE: %d\n", err);
                sse_client_pcb = NULL;
            }
        }

        sleep_ms(200);
    }

    cyw43_arch_deinit();
    return 0;
}
