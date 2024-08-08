#ifndef GPS_H
#define GPS_H
#include <stdio.h>
#include <stdbool.h>
#include "driver/uart.h"

typedef struct
{
    uint8_t gps_rxd;
    uint8_t gps_txd;
    uart_port_t gps_uart_num;
    uint8_t gps_rst;
    uint8_t gps_en;
    uint8_t gps_pps;
}data_gps_config_t;

typedef struct {
    char *time;
    int hour;
    int minute;
    int second;
    bool status;
    double latitude;
    int8_t latitude_direction;
    double longitude;
    int8_t longitude_direction;
    float speed;
    float course;
    char dateTime[20];
    uint8_t satelite;
}data_gps_t;

void gps_time(const char *data_nmea, data_gps_t *data_gps);
void gps_latitude(const char *data_nmea, data_gps_t *data_gps);
int gps_send_command(data_gps_config_t *gps_config, const char *command, char *response, int response_len);
int check_gps_functionality(data_gps_config_t *gps_config);



#endif
