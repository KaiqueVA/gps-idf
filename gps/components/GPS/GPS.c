#include <stdio.h>
#include <string.h>
#include "GPS.h"
#include "esp_log.h"
#include "esp_system.h"

#define TAG "GPS"

const char *PMTK_TEST_COMMAND = "$PMTK605*31\r\n";

void aloca_char(char **data, uint16_t *len)
{
    *data = (char*)malloc((*len + 1) * sizeof(char));
    if(*data == NULL)
    {
        ESP_LOGI(TAG, "Error allocating memory");
        esp_restart();
    }
}

void gps_time(const char *data_nmea, data_gps_t *data_gps, int8_t fuso)
{
    uint16_t len;
    const char *time_ptr = NULL;

    if (strncmp(data_nmea, "$GPGGA", 6) == 0) {
        time_ptr = strchr(data_nmea, ',') + 1;
    } else if (strncmp(data_nmea, "$GPRMC", 6) == 0) {
        time_ptr = strchr(data_nmea, ',') + 1;
    } else if (strncmp(data_nmea, "$GPGLL", 6) == 0) {
        time_ptr = strrchr(data_nmea, ',') - 9; // Time is towards the end
    }

    if (time_ptr == NULL) {
        ESP_LOGI(TAG, "No valid time found in NMEA sentence");
        return;
    }

    if (data_gps->time != NULL) {
        free(data_gps->time);
    }
    if(fuso > 0)
        data_gps->hour = (time_ptr[0] - '0') * 10 + (time_ptr[1] - '0') + fuso;
    else if(fuso < 0)
        data_gps->hour = (time_ptr[0] - '0') * 10 + (time_ptr[1] - '0') - (-fuso);
    else
        data_gps->hour = (time_ptr[0] - '0') * 10 + (time_ptr[1] - '0');
    data_gps->minute = (time_ptr[2] - '0') * 10 + (time_ptr[3] - '0');
    data_gps->second = (time_ptr[4] - '0') * 10 + (time_ptr[5] - '0');

    len = snprintf(NULL, 0, "%02d:%02d:%02d", data_gps->hour, data_gps->minute, data_gps->second) + 1;
    aloca_char(&data_gps->time, &len);
    snprintf(data_gps->time, len, "%02d:%02d:%02d", data_gps->hour, data_gps->minute, data_gps->second);
}

void gps_latitude(const char *data_nmea, data_gps_t *data_gps)
{
    const char *lat_dir_ptr, *lat_ptr;
    int lat_deg;

    if (strncmp(data_nmea, "$GPGGA", 6) == 0) {
        lat_ptr = strchr(data_nmea, ',') + 7;
    } else if (strncmp(data_nmea, "$GPGLL", 6) == 0) {
        lat_ptr = strchr(data_nmea, ',') + 7;
    } else if (strncmp(data_nmea, "$GPRMC", 6) == 0) {
        lat_ptr = strchr(data_nmea, ',') + 19;
    } else {
        return;
    }

    data_gps->latitude = atof(lat_ptr);
    lat_deg = (int)(data_gps->latitude / 100);
    data_gps->latitude = lat_deg + (data_gps->latitude - lat_deg * 100) / 60;

    lat_dir_ptr = lat_ptr + 9;
    data_gps->latitude_direction = (lat_dir_ptr[0] == 'N') ? 1 : -1;

    data_gps->latitude *= data_gps->latitude_direction;
}

int gps_send_command(data_gps_config_t *gps_config, const char *command, char *response, int response_len) {
    ESP_LOGI(TAG, "Sending command: %s", command);
    uart_write_bytes(gps_config->gps_uart_num, command, strlen(command));
    
    memset(response, 0, response_len);

    int len = uart_read_bytes(gps_config->gps_uart_num, (uint8_t *)response, response_len - 1, 1000 / portTICK_PERIOD_MS);
    if (len > 0) {
        response[len] = '\0';
        ESP_LOGI(TAG, "Response: %s", response);
        return len;
    }
    ESP_LOGI(TAG, "No response received");
    return -1;
}

int check_gps_functionality(data_gps_config_t *gps_config) {
    char response[128];
    int retries = 10;
    while (retries-- > 0) {
        int len = gps_send_command(gps_config, PMTK_TEST_COMMAND, response, sizeof(response));
        ESP_LOGI(TAG, "Response: %s", response);
        if (len > 0) {
            ESP_LOGI(TAG, "GPS module is working");
            return 1;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "GPS module is not working");
    return -1;
}
