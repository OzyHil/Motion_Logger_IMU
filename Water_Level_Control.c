#include "General.h"
#include "Led.h"
#include "Buzzer.h"
#include "Button.h"
#include "Display.h"
#include "Led_Matrix.h"
#include "Webserver.h"
#include "Potentiometer.h"

system_state_t g_current_system_state = SYSTEM_DRAINING;      // Variável global para armazenar o estado atual do sistema
float water_level = 0.0f;                                     // Nível de água atual
float water_level_max_limit = MAX_WATER_LEVEL;                // Limite máximo de nível de água
float water_level_min_limit = MIN_WATER_LEVEL;                // Limite mínimo de nível de água

// Semáforos para controle de acesso e sincronização
SemaphoreHandle_t xCountButtonASemaphore,
    xCountButtonBSemaphore,
    xButtonJSemaphore,
    xDisplayMutex,
    xBuzzerMutex,
    xLedMatrixMutex,
    xWaterLevelMutex,
    xWaterLimitsMutex,
    xStateMutex;

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

    if (gpio == BUTTON_A && (now - last_time_button_A > DEBOUNCE_TIME))
    {
        last_time_button_A = now;
        signal_task_from_isr(xCountButtonASemaphore);
        
    }
    else if (gpio == BUTTON_B && (now - last_time_button_B > DEBOUNCE_TIME))
    {        
        last_time_button_B = now;
        signal_task_from_isr(xCountButtonBSemaphore);
        
    }
    else if (gpio == BUTTON_J && (now - last_time_button_J > DEBOUNCE_TIME))
    {
        last_time_button_J = now;
        signal_task_from_isr(xButtonJSemaphore);
        
    }
}

void vTaskDisplay()
{
    while (1)
    {
        if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE) // Tenta obter o mutex do display
        {
            draw_info(ipaddr_ntoa(&netif_default->ip_addr)); // Exibe no display
            vTaskDelay(pdMS_TO_TICKS(400));                  // Aguarda 500 ms antes de repetir
        }
    }
}

void vTaskLedMatrix()
{
    system_state_t current_state;

    while (1)
    {
        if (xSemaphoreTake(xLedMatrixMutex, portMAX_DELAY) == pdTRUE) // Tenta obter o mutex da matriz de LEDs
        {
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
            {
                current_state = g_current_system_state; // Obtém o estado atual do sistema
                xSemaphoreGive(xStateMutex);            // Libera o mutex do estado

                if (current_state == SYSTEM_FILLING)
                {
                    update_matrix_from_level(6, 8); // Chama a função da tarefa da matriz de LEDs
                }
                else
                {
                    update_matrix_from_level(2, 8); // Chama a função da tarefa da matriz de LEDs
                }
                xSemaphoreGive(xLedMatrixMutex); // Libera o mutex da matriz de LEDs
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Aguarda 500 ms antes de repetir
    }
}

void vTaskControlSystem()
{
    system_state_t current_state_copy = SYSTEM_DRAINING; // Cópia do estado atual do sistema
    float new_water_level = 0.0f;                       // Novo nível de água a ser definido
    float adc_value = 0.0f;                              // Valor lido do potenciômetro    
    
    /* Configura o pino da bomba de água*/
    gpio_init(WATER_PUMP_PIN); // Inicializa o pino da bomba de água
    gpio_set_dir(WATER_PUMP_PIN, GPIO_OUT); // Configura o pino da bomba de água como saída
    gpio_put(WATER_PUMP_PIN, 0); // Desliga a bomba de água inicialmente
    
    while (1)
    {       
        switch(current_state_copy)
        {
            case SYSTEM_DRAINING: // Estado de drenagem

                /* -------------- LEITURA DA BOIA ---------------  */
                adc_value = read_potentiometer();      
                new_water_level = map_reading(adc_value, MIN_ADC_VALUE, MAX_ADC_VALUE, 0.0F, MAX_WATER_CAPACITY);

                /* -------------- ATUALIZAÇÃO DO NÍVEL DA ÁGUA ---------------  */
                if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
                {
                    water_level = new_water_level; 
                    xSemaphoreGive(xWaterLevelMutex); 
                }
                printf("INFO: Water level: %.2f \n", new_water_level);

                /* -------------- ATUALIZAÇÃO DO ESTADO DO SISTEMA ---------------  */
                if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
                {
                    if(new_water_level < water_level_min_limit){
                        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
                        {
                            g_current_system_state = SYSTEM_FILLING; // Muda o estado do sistema para enchimento
                            current_state_copy = SYSTEM_FILLING;     // Atualiza a cópia do estado atual
                            
                            xSemaphoreGive(xStateMutex);             // Libera o mutex do estado

                            set_led_color(GREEN);                  // Define a cor do LED para verde
                            gpio_put(WATER_PUMP_PIN, 1);          // Liga a bomba de água

                            printf("INFO: System filling\n");

                        }
                    }
                    xSemaphoreGive(xWaterLimitsMutex);  
                }
                break;
            case SYSTEM_FILLING: // Estado de enchimento
                /* -------------- LEITURA DA BOIA ---------------  */
                adc_value = read_potentiometer();      
                new_water_level = map_reading(adc_value, MIN_ADC_VALUE, MAX_ADC_VALUE, 0.0F, MAX_WATER_CAPACITY);

                /* -------------- ATUALIZAÇÃO DO NÍVEL DA ÁGUA ---------------  */
                if (xSemaphoreTake(xWaterLevelMutex, portMAX_DELAY) == pdTRUE)
                {
                    water_level = new_water_level; 
                    xSemaphoreGive(xWaterLevelMutex); 
                }

                printf("INFO: Water level: %.2f \n", new_water_level);

                /* -------------- ATUALIZAÇÃO DO ESTADO DO SISTEMA ---------------  */
                if (xSemaphoreTake(xWaterLimitsMutex, portMAX_DELAY) == pdTRUE)
                {
                    if(new_water_level > water_level_max_limit){
                        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
                        {
                            g_current_system_state = SYSTEM_DRAINING; // Muda o estado do sistema para enchimento
                            current_state_copy = SYSTEM_DRAINING;     // Atualiza a cópia do estado atual
                            
                            xSemaphoreGive(xStateMutex);             // Libera o mutex do estado

                            set_led_color(DARK);                  // Desliga o LED
                            gpio_put(WATER_PUMP_PIN, 0);          // Desliga a bomba de água

                            printf("INFO: System draining\n");

                        }
                    }
                    xSemaphoreGive(xWaterLimitsMutex);  
                }
                break;
            default:            // Estado desconhecido
                printf("WARNING: Unknow state detected! Resetting system to draining state...\n");

                if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
                {
                    g_current_system_state = SYSTEM_DRAINING; // Muda o estado do sistema para enchimento
                    current_state_copy = SYSTEM_DRAINING;     // Atualiza a cópia do estado atual
                    
                    xSemaphoreGive(xStateMutex);             // Libera o mutex do estado

                    set_led_color(DARK);                  // Desliga o LED
                    gpio_put(WATER_PUMP_PIN, 0);          // Desliga a bomba de água
                }
                
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(200)); // Aguarda 500 ms antes de repetir
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
    xBuzzerMutex = xSemaphoreCreateMutex();
    xLedMatrixMutex = xSemaphoreCreateMutex();
    xWaterLevelMutex = xSemaphoreCreateMutex();
    xWaterLimitsMutex = xSemaphoreCreateMutex();
    xStateMutex = xSemaphoreCreateMutex();

    draw_info("Iniciando...");                  // Exibe a mensagem inicial no display
    init_cyw43();                               // Inicializa a arquitetura CYW43
    draw_info("Conectando a rede...");          // Exibe a mensagem inicial no display
    connect_to_wifi();                          // Conecta ao Wi-Fi
    draw_info("Iniciando servidor...");         // Exibe a mensagem inicial no display
    struct tcp_pcb *server = init_tcp_server(); // Inicializa o servidor TCP

    // Criação das tarefas
    xTaskCreate(vTaskDisplay, "Task Display", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskLedMatrix, "Task LedMatrix", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskLedRgb, "Task LedRgb", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskBuzzer, "Task Buzzer", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskButtonA, "Task ButtonA", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskButtonB, "Task ButtonB", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskButtonJ, "Task ButtonJ", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskControlSystem, "Task Control System", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    // xTaskCreate(vTaskControlWaterMotor, "Task ControlWaterMotor", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskTCPServer, "Task TCP Server", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    vTaskStartScheduler(); // Inicia o escalonador de tarefas

    panic_unsupported(); // Se o escalonador falhar, entra em pânico
}
