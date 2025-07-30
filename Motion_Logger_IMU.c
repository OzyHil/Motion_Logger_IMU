#include "General.h"
#include "Led.h"
#include "Buzzer.h"
#include "Button.h"
#include "Display.h"
#include "pico/bootrom.h"
#include "mpu6050.h"
#include "sd_logger.h"

system_state_t g_current_system_state = SYSTEM_STATE_INITIALIZING;
bool g_sd_mounted = false;

// Semáforos para controle de acesso e sincronização
SemaphoreHandle_t xDisplayMutex,
    xStateMutex,
    xCaptureToggleSemaphore,
    xMountToggleSemaphore,
    xSdMutex,
    xSendFileSemaphore,
    xSdStateMutex;

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
        signal_task_from_isr(xCaptureToggleSemaphore);
    }
    if (gpio == BUTTON_B && (now - last_time_button_B > DEBOUNCE_TIME))
    {
        last_time_button_B = now;
        signal_task_from_isr(xMountToggleSemaphore);
    }
    if (gpio == BUTTON_J && (now - last_time_button_J > DEBOUNCE_TIME))
    {
        last_time_button_J = now;
        signal_task_from_isr(xSendFileSemaphore);
    }
}

void vTaskReadIMU()
{
    int16_t acceleration[3], gyroscope[3];
    static int16_t acc_buffer[BUFFER_SIZE][3];
    static int16_t gyro_buffer[BUFFER_SIZE][3];
    static int total_samples_saved = 0;

    while (1)
    {
        if (xSemaphoreTake(xCaptureToggleSemaphore, 0))
        {
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
            {
                g_current_system_state = SYSTEM_STATE_RECORDING;
                xSemaphoreGive(xStateMutex);
            }
            single_beep();
            display_message("Capturando dados...");

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                // Lê IMU
                MPU6050_read_raw(acceleration, gyroscope);

                // Salva no buffer
                for (int j = 0; j < 3; j++)
                {
                    acc_buffer[i][j] = acceleration[j];
                    gyro_buffer[i][j] = gyroscope[j];
                }

                vTaskDelay(pdMS_TO_TICKS(100)); // Pequeno atraso entre amostras
            }
            if (xSemaphoreTake(xSdStateMutex, portMAX_DELAY) == pdTRUE)
            {
                if (g_sd_mounted)
                {
                    xSemaphoreGive(xSdStateMutex);

                    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
                    {
                        g_current_system_state = SYSTEM_STATE_SD_ACCESSING;
                        xSemaphoreGive(xStateMutex);
                    }
                    display_message("Gravando...");

                    if (xSemaphoreTake(xSdMutex, portMAX_DELAY) == pdTRUE)
                    {
                        save_data_to_SD(acc_buffer, gyro_buffer, BUFFER_SIZE, total_samples_saved);
                        xSemaphoreGive(xSdMutex);
                        total_samples_saved += BUFFER_SIZE;
                    }

                    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
                    {
                        g_current_system_state = SYSTEM_STATE_READY;
                        xSemaphoreGive(xStateMutex);
                    }
                    
                    double_beep();
                    display_message("Dados salvos no SD, pronto para captura!");
                }
                else
                {
                    xSemaphoreGive(xSdStateMutex);
                    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE)
                    {
                        g_current_system_state = SYSTEM_STATE_ERROR;
                        xSemaphoreGive(xStateMutex);
                    }
                    display_message("Erro, cartao nao montado ou inserido.");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vTaskControlSD()
{
    static bool sd_mounted = false;

    while (1)
    {
        if (xSemaphoreTake(xSendFileSemaphore, 0))
        {
            if (xSemaphoreTake(xSdMutex, portMAX_DELAY) == pdTRUE)
            {
                read_file("dados.csv");
                xSemaphoreGive(xSdMutex);
            }
        }

        if (xSemaphoreTake(xMountToggleSemaphore, 0))
        {
            if (xSemaphoreTake(xSdMutex, portMAX_DELAY) == pdTRUE)
            {
                if (!sd_mounted)
                {
                    run_mount();
                    sd_mounted = true;

                    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
                    {
                        display_message("Cartao Montado, pronto para captura!");
                        xSemaphoreGive(xDisplayMutex);

                        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) // Espera pelo sinal do botão J
                        {
                            g_current_system_state = SYSTEM_STATE_READY;
                            xSemaphoreGive(xStateMutex);
                        }
                    }
                }
                else
                {
                    run_unmount();
                    sd_mounted = false;

                    if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE)
                    {
                        display_message("Cartao Desmontado, aguardando...");
                        xSemaphoreGive(xDisplayMutex);

                        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) // Espera pelo sinal do botão J
                        {
                            g_current_system_state = SYSTEM_STATE_INITIALIZING;
                            xSemaphoreGive(xStateMutex);
                        }
                    }
                }
                xSemaphoreGive(xSdMutex);
                if (xSemaphoreTake(xSdStateMutex, portMAX_DELAY) == pdTRUE)
                {
                    g_sd_mounted = sd_mounted;
                    xSemaphoreGive(xSdStateMutex);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTaskControlLED()
{
    system_state_t system_state;

    while (1)
    {
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) // Espera pelo sinal do botão J
        {
            system_state = g_current_system_state;
            xSemaphoreGive(xStateMutex);
        }

        switch (system_state)
        {
        case SYSTEM_STATE_INITIALIZING:
            set_led_color(YELLOW);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case SYSTEM_STATE_READY:
            set_led_color(GREEN);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case SYSTEM_STATE_RECORDING:
            set_led_color(RED);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        case SYSTEM_STATE_SD_ACCESSING:
            set_led_color(BLUE);
            vTaskDelay(pdMS_TO_TICKS(150));
            set_led_color(DARK);
            vTaskDelay(pdMS_TO_TICKS(150));
            break;
        case SYSTEM_STATE_ERROR:
            set_led_color(PURPLE);
            vTaskDelay(pdMS_TO_TICKS(150));
            set_led_color(DARK);
            vTaskDelay(pdMS_TO_TICKS(150));
            break;
        }
    }
}

int main()
{
    init_system_config(); // Função para inicializar a configuração do sistema

    configure_leds();    // Configura os LEDs
    configure_buzzer();  // Configura o buzzer
    configure_display(); // Configura o display OLED

    display_message("Inicializando...");
    set_led_color(YELLOW);

    configure_mpu6050(); // Configura o MPU6050

    configure_button(BUTTON_A);
    configure_button(BUTTON_B);
    configure_button(BUTTON_J);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_J, GPIO_IRQ_EDGE_FALL, true);

    xCaptureToggleSemaphore = xSemaphoreCreateBinary();
    xMountToggleSemaphore = xSemaphoreCreateBinary();
    xSendFileSemaphore = xSemaphoreCreateBinary();

    xStateMutex = xSemaphoreCreateMutex();
    xSdMutex = xSemaphoreCreateMutex();
    xDisplayMutex = xSemaphoreCreateMutex();
    xSdStateMutex = xSemaphoreCreateMutex();

    display_message("Aguardando...");

    // Criação das tarefas
    xTaskCreate(vTaskReadIMU, "Task Read IMU", configMINIMAL_STACK_SIZE + 256, NULL, 1, NULL);
    xTaskCreate(vTaskControlSD, "Task Control SD", configMINIMAL_STACK_SIZE + 256, NULL, 1, NULL);
    xTaskCreate(vTaskControlLED, "Task Control LED", configMINIMAL_STACK_SIZE + 256, NULL, 1, NULL);

    vTaskStartScheduler(); // Inicia o escalonador de tarefas

    panic_unsupported(); // Se o escalonador falhar, entra em pânico
}
