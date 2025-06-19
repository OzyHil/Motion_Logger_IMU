#include "General.h"
#include "Led.h"
#include "Buzzer.h"
#include "Button.h"
#include "Display.h"
#include "Led_Matrix.h"
#include "Webserver.h"

// Semáforos para controle de acesso e sincronização
SemaphoreHandle_t xCountButtonASemaphore,
    xCountButtonBSemaphore,
    xButtonJSemaphore,
    xDisplayMutex,
    xLedMutex,
    xBuzzerMutex,
    xLedMatrixMutex,
    xLedPotentiometerMutex,
    xLedWaterMotorMutex;

// Variáveis para controle de tempo de debounce dos botões
uint last_time_button_A, last_time_button_B, last_time_button_J = 0;

// Função para sinalizar uma tarefa a partir de uma ISR
void signal_task_from_isr(SemaphoreHandle_t xSemaphore)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;                // Variável para verificar se uma tarefa de maior prioridade foi despertada
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken); // Libera o semáforo e verifica se uma tarefa de maior prioridade foi despertada
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                 // Se uma tarefa de maior prioridade foi despertada, realiza um yield para que ela possa ser executada imediatamente
}

// Função de tratamento de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t now = us_to_ms(get_absolute_time());

    if (gpio == BUTTON_A)
    {
        if (now - last_time_button_A > DEBOUNCE_TIME)
        {
            last_time_button_A = now;
            signal_task_from_isr(xCountButtonASemaphore);
        }
    }
    else if (gpio == BUTTON_B)
    {
        if (now - last_time_button_B > DEBOUNCE_TIME)
        {
            last_time_button_B = now;
            signal_task_from_isr(xCountButtonBSemaphore);
        }
    }
    else if (gpio == BUTTON_J)
    {
        if (now - last_time_button_J > DEBOUNCE_TIME)
        {
            last_time_button_J = now;
            signal_task_from_isr(xButtonJSemaphore);
        }
    }
}

void vTaskDisplay()
{
    while (1)
    {
        if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE) // Tenta obter o mutex do display
        {
            draw_info(ipaddr_ntoa(&netif_default->ip_addr)); // Exibe no display
            vTaskDelay(pdMS_TO_TICKS(500));                  // Aguarda 500 ms antes de repetir
        }
    }
}
void vTaskLedMatrix()
{
    while (1)
    {
        if (xSemaphoreTake(xLedMatrixMutex, portMAX_DELAY) == pdTRUE) // Tenta obter o mutex da matriz de LEDs
        {
            update_matrix_from_level(4, 8);  // Chama a função da tarefa da matriz de LEDs
            xSemaphoreGive(xLedMatrixMutex); // Libera o mutex da matriz de LEDs
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Aguarda 500 ms antes de repetir
    }
}

void vTaskTCPServer()
{
    run_tcp_server_loop(); // Inicia o loop do servidor TCP
    deinit_cyw43();        // Finaliza a arquitetura CYW43
}

int main()
{
    init_system_config(); // Função para inicializar a configuração do sistema

    configure_leds();       // Configura os LEDs
    configure_buzzer();     // Configura o buzzer
    configure_display();    // Configura o display OLED
    configure_led_matrix(); // Configura a matriz de LEDs

    configure_button(BUTTON_A); // Configura os botões
    configure_button(BUTTON_B);
    configure_button(BUTTON_J);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_J, GPIO_IRQ_EDGE_FALL, true);

    xCountButtonASemaphore = xSemaphoreCreateCounting(5, 0); // Semáforo de contagem para o botão A
    xCountButtonBSemaphore = xSemaphoreCreateCounting(5, 0);

    xButtonJSemaphore = xSemaphoreCreateBinary(); // Semáforo binário para o botão J

    // Mutexs para controle de acesso e sincronização
    xDisplayMutex = xSemaphoreCreateMutex();
    xLedMutex = xSemaphoreCreateMutex();
    xBuzzerMutex = xSemaphoreCreateMutex();
    xLedMatrixMutex = xSemaphoreCreateMutex();
    xLedPotentiometerMutex = xSemaphoreCreateMutex();
    xLedWaterMotorMutex = xSemaphoreCreateMutex();

    draw_info("Iniciando..."); // Exibe a mensagem inicial no display
    init_cyw43();                               // Inicializa a arquitetura CYW43
    draw_info("Conectando a rede..."); // Exibe a mensagem inicial no display
    connect_to_wifi();                          // Conecta ao Wi-Fi
    draw_info("Iniciando servidor..."); // Exibe a mensagem inicial no display
    struct tcp_pcb *server = init_tcp_server(); // Inicializa o servidor TCP

    // Criação das tarefas
    xTaskCreate(vTaskDisplay, "Task Display", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskLedMatrix, "Task LedMatrix", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskLedRgb, "Task LedRgb", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskBuzzer, "Task Buzzer", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskButtonA, "Task ButtonA", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskButtonB, "Task ButtonB", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskButtonJ, "Task ButtonJ", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskPotentiometer, "Task Potentiometer", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskControlWaterMotor, "Task ControlWaterMotor", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskTCPServer, "Task TCP Server", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    vTaskStartScheduler(); // Inicia o escalonador de tarefas

    panic_unsupported(); // Se o escalonador falhar, entra em pânico
}
