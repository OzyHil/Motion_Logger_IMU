#include "Potentiometer.h" // Inclusão do cabeçalho com definições do potenciômetro

// Função para configurar o potenciômetro utilizando ADC
void configure_potentiometer()
{
    // Inicializa o ADC no pino do potenciômetro (POTENTIOMETER_A) com valor de "wrap" especificado
    adc_gpio_init(POTENTIOMETER_PIN);
}

// Função para ler a posição do potenciômetro e retornar o valor
float read_potentiometer() {
    float sum = 0.0f;
    adc_select_input(2);

    for (int i = 0; i < 100; i++)
    {
        sum += adc_read();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    float average = sum / 100.0f;
    float unknown_resistor = (KNOWN_RESISTOR * average) / (4095 - average);

    return average;
}