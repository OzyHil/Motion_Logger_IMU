#include "Webserver.h" // Biblioteca geral do sistema

void init_cyw43()
{
    // Inicializa a arquitetura CYW43
    if (cyw43_arch_init())
    {
        //////////////////// Falha ao inicializar o Wi-Fi
        sleep_ms(100);
    }

    // Coloca GPIO do módulo em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);
}

void connect_to_wifi()
{
    // Ativa o modo Station (cliente Wi-Fi)
    cyw43_arch_enable_sta_mode();

    //////////////////// Conectaando

    // Tenta conectar ao Wi-Fi com timeout de 20 segundos
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        /////////////// Falha na conexão Wi-Fi
        sleep_ms(100);
    }
}

struct tcp_pcb *init_tcp_server()
{
    // Cria o controle de protocolo TCP
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        ////////////// Falha ao criar o servidor TCP
        return NULL;
    }

    // Tenta vincular à porta 80
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        ///////////////// Falha ao vincular à porta
        return NULL;
    }

    // Coloca em modo de escuta e define a função de callback
    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    return server;
}

void run_tcp_server_loop()
{
    // Loop principal para manter o Wi-Fi ativo e escutar conexões
    while (true)
    {
        cyw43_arch_poll();
        vTaskDelay(100);
    }
}

void deinit_cyw43()
{
    // Finaliza a arquitetura CYW43
    cyw43_arch_deinit();
    vTaskDelete(NULL); // Deleta a tarefa atual
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Tratamento do request do usuário
void user_request(char **request_ptr)
{
    char *request = *request_ptr;

    if (strstr(request, ""))
    {
    }
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinâmica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    // Tratamento de request - Controle dos LEDs
    user_request(&request);

    char html[7144];

    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Connection: close\r\n"
             "\r\n"
             "<!DOCTYPE html>"
             "<html>"

             "<head>"
             "<meta charset='UTF-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
             "<title>Water Level Control</title>"
             "</head>"

             "<body>"
             "<h1>Water Level Control</h1>"
             "</body>"

             "</html>");

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    // libera memória alocada dinamicamente
    free(request);

    // libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}