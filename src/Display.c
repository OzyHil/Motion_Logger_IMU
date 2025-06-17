#include "Display.h" // Inclusão do cabeçalho com definições relacionadas ao display OLED

ssd1306_t ssd; // Estrutura que representa o display OLED SSD1306

// Função para configurar a comunicação I2C e inicializar o display OLED
void configure_display()
{
    // Inicializa o barramento I2C na porta e com frequência de 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);

    // Define as funções dos pinos SDA e SCL como I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // Ativa resistores de pull-up nos pinos SDA e SCL
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display com resolução 128x64, sem rotação, endereço I2C e porta definida
    ssd1306_init(&ssd, 128, 64, false, DISPLAY_ADDRESS, I2C_PORT);

    // Configura o display com parâmetros padrão
    ssd1306_config(&ssd);
}

// Função para limpar o display OLED
void clear_display()
{
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Função para desenhar informações no display OLED
void draw_info(uint8_t person_count, uint8_t max_people)
{
    // char people[8];
    // char spots[8];

    // ssd1306_fill(&ssd, false);

    // snprintf(people, sizeof(people), "%d/%d", person_count, max_people); // Formata a string com o número de pessoas e a capacidade máxima
    // snprintf(spots, sizeof(spots), "%d/%d", max_people - person_count, max_people); // Calcula o número de vagas restantes

    // ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); // Desenha um retângulo preenchido com a cor atual
    // ssd1306_rect(&ssd, 1, 1, 127, 12, true, true);  // Desenha um retângulo preenchido com a cor atual

    // ssd1306_vline(&ssd, 60, 17, 63, true); // Desenha uma linha horizontal no meio do display

    // ssd1306_hline(&ssd, 1, 127, 29, true); // Desenha uma linha horizontal no meio do display
    // ssd1306_hline(&ssd, 1, 127, 46, true); // Desenha uma linha horizontal no meio do display

    // float proportion = max_people / 10.0f;

    // int plus_count = (int)(person_count / proportion);

    // for (size_t i = 0; i < plus_count; i++)
    // {
    //     ssd1306_draw_string(&ssd, "+", 6 + 12 * i, 2);
    // }

    // ssd1306_draw_string(&ssd, "People", 6, 18);
    // ssd1306_draw_string(&ssd, people, 66, 18);

    // ssd1306_draw_string(&ssd, "Spots", 6, 34);
    // ssd1306_draw_string(&ssd, spots, 66, 34);

    // ssd1306_draw_string(&ssd, "Access", 6, 51);
    // ssd1306_draw_string(&ssd, person_count == max_people ? "Crowded" : "Allowed", 66, 51);

    // ssd1306_send_data(&ssd);
}