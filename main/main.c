/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include "hardware/rtc.h"
 #include "pico/util/datetime.h"
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 #include "hardware/timer.h"
 #include <stdio.h>
 #include <string.h>
 
 const int TRIGGER_PIN = 8;
 const int ECHO = 18;
 
 typedef struct {
     alarm_id_t alarm_id;
     bool subida;
     absolute_time_t start;
     absolute_time_t finish;
     int descida;
 } sensor_state_t;
 
 volatile static sensor_state_t sensor_state = {0};
 
 static const uint timer_estourou = 50;
 
 int64_t alarme(alarm_id_t id, void *user_data) {
     if (sensor_state.subida) {
         datetime_t now;
         rtc_get_datetime(&now);

         printf("%02d:%02d:%02d - Falha\n", now.hour, now.min, now.sec);
         sensor_state.subida = false;
     }
     return 0; 
 }
 
 void echo_callback(uint gpio, uint32_t events) {
     if (events & GPIO_IRQ_EDGE_RISE) {
         sensor_state.start = get_absolute_time();
         if (sensor_state.subida) {
             cancel_alarm(sensor_state.alarm_id);
             sensor_state.subida = false;
         }
     } else if (events & GPIO_IRQ_EDGE_FALL) {
         sensor_state.finish = get_absolute_time();
         sensor_state.descida = 1;
     }
 }
 
 int main() {
     stdio_init_all();
     
     datetime_t t = {
         .year  = 2025,
         .month = 3,
         .day   = 19,
         .dotw  = 0,  
         .hour  = 0,
         .min   = 0,
         .sec   = 0
     };
     rtc_init();
     rtc_set_datetime(&t);
     
     gpio_init(TRIGGER_PIN);
     gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
     
     gpio_init(ECHO);
     gpio_set_dir(ECHO, GPIO_IN);
     gpio_set_irq_enabled_with_callback(ECHO,
                                        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                        true,
                                        echo_callback);
     
     int sensor = 0;
     
     while (true) {

        int caracter = getchar_timeout_us(500);
         if (caracter == 's') {
             sensor = !sensor;
             if (sensor) {
                 printf("sensor comecou\n");
             } else {
                 printf("sensor parou\n");
             }
         }
         
         if (sensor) {

            gpio_put(TRIGGER_PIN, 1);
             sleep_us(10);
             gpio_put(TRIGGER_PIN, 0);
             
             sensor_state.subida = true;
             sensor_state.alarm_id = add_alarm_in_ms(timer_estourou, alarme, NULL, false);
         
             if (sensor_state.descida == 1) {
                 sensor_state.descida = 0;
                 int delta_t = absolute_time_diff_us(sensor_state.start, sensor_state.finish);
                 double distancia = (delta_t * 0.0343) / 2.0;
                 datetime_t t2 = {0};
                 rtc_get_datetime(&t2);
         
                 char datetime_buf[256];
                 char *datetime_str = datetime_buf;
                 datetime_to_str(datetime_str, sizeof(datetime_buf), &t2);
                 printf("%s - %.2f cm\n", datetime_str, distancia);
             }
         }
         
         sleep_ms(1000);
     }
 }