#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "GPS.h"

#define TAG "GPS"

#define UART_NUM UART_NUM_1
#define UART_TX_PIN (GPIO_NUM_17)
#define UART_RX_PIN (GPIO_NUM_16)
#define UART_BUF_SIZE (1024)
#define BAUD_RATE 9600

void task_gps(void *pvParameter);

void app_main(void)
{
    xTaskCreate(&task_gps, "task_gps", 4096, NULL, 5, NULL);
}

void task_gps(void *pvParameter)
{
    uart_event_t event;
    size_t buffered_size;
    char *data = NULL;

    // Configurar UART
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    data_gps_config_t data_gps_config = {
        .gps_rxd = UART_RX_PIN,
        .gps_txd = UART_TX_PIN,
        .gps_uart_num = UART_NUM,
        .gps_rst = 0,
        .gps_en = 0,
        .gps_pps = 0,
    };

    data_gps_t data_gps;
    memset(&data_gps, 0, sizeof(data_gps));

    ESP_LOGI(TAG, "Iniciando configuração da UART");

    uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "Configuração da UART concluída");

    // Buffer para armazenar os dados recebidos
    uint8_t *uart_buffer = (uint8_t *) malloc(UART_BUF_SIZE);
    if (uart_buffer == NULL) {
        ESP_LOGE(TAG, "Não foi possível alocar memória para o buffer UART");
        return;
    }

    ESP_LOGI(TAG, "Entrando no LOOP principal");
    gps_send_command(&data_gps_config, "$PMTK353,1,1,0*36\r\n", (char*)uart_buffer, UART_BUF_SIZE);

    while (1) {
        // Esperar por dados
        int length = uart_read_bytes(UART_NUM, uart_buffer, UART_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (length > 0) {
            // Realocar memória para armazenar os dados recebidos
            char *temp_data = (char *) realloc(data, length + 1);
            if (temp_data == NULL) {
                ESP_LOGE(TAG, "Não foi possível realocar memória para armazenar os dados recebidos");
                free(uart_buffer);
                free(data); // Free previously allocated memory
                return;
            }
            data = temp_data;

            // Copiar dados para a variável 'data'
            memcpy(data, uart_buffer, length);
            data[length] = '\0'; // Null-terminate string
            ESP_LOGI(TAG, "Dados recebidos: %s", data);

            // Processar dados NMEA recebidos
            gps_time(data, &data_gps, -3);
            ESP_LOGI("Horario", "%d:%d:%d", data_gps.hour, data_gps.minute, data_gps.second);
            // ESP_LOGI(TAG, "Horario: %s", data_gps.time);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    free(uart_buffer);
    free(data);
}
