/************************************************************
*
* @file:      main.c
* @author:    Weili An
* @email:     an107@purdue.edu
* @version:   v1.0.0
* @date:      09/10/2021
* @brief:     ESP-NOW latency test sender, send to broadcast
*             addr and time the response from receiver
*
************************************************************/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_timer.h"

#define MAX_TEST_DATA_LENGTH 250
uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t test_data[MAX_TEST_DATA_LENGTH];
uint64_t last_send_time;
uint64_t send_success_time;
uint64_t recv_time;

void latency_test_send(const uint8_t *mac_addr, esp_now_send_status_t status);
void latency_test_both(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void user_send();
esp_err_t get_macAddr();

void app_main(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    // Print mac addr
    get_macAddr();

    // Init ESP-NOW
    ESP_LOGI("ESP-NOW", "Init ESP-NOW");
    ESP_ERROR_CHECK(esp_now_init());
    uint32_t esp_now_version;
    ESP_ERROR_CHECK(esp_now_get_version(&esp_now_version));
    ESP_LOGI("ESP-NOW", "ESP-NOW Version: %d", esp_now_version);

    // Init test data bank with non-zeros
    for(int i = 0; i < MAX_TEST_DATA_LENGTH; i++)
        test_data[i] = i;

    // Register ESP-NOW send callback to time the send time
    // as the callback is called when the other ESPs received it on MAC layer
    ESP_LOGI("ESP-NOW", "Register ESP-NOW send callback func");
    ESP_ERROR_CHECK(esp_now_register_send_cb(latency_test_send));

    // Register ESP-NOW recv callback to time the whole transcation time
    ESP_LOGI("ESP-NOW", "Register ESP-NOW recv callback func");
    ESP_ERROR_CHECK(esp_now_register_recv_cb(latency_test_both));

    // Start to send
    // 16 byte test
    user_send(test_data, 16);
    vTaskDelay(10/portTICK_PERIOD_MS);

    // deadloop
    for(;;);
}

/**
 * @brief ESP now send to broadcast mac
 * 
 * @param data 
 * @param len 
 */
void user_send(uint8_t* data, size_t len) {
    esp_now_send(broadcast_mac, data, len);
}

/**
 * @brief Calculate the sending time using esp_timer_get_time()
 * 
 * @param mac_addr 
 * @param status 
 */
void latency_test_send(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Send success, measure time
    send_success_time = esp_timer_get_time();
}

/**
 * @brief Calculate both send and receive time using esp_timer_get_time()
 * 
 * @param mac_addr 
 * @param data 
 * @param data_len 
 */
void latency_test_both(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    recv_time = esp_timer_get_time();
    ESP_LOGI("ESP-NOW", "Send success time: %lld us", send_success_time - last_send_time);
    ESP_LOGI("ESP-NOW", "Send-recv time: %lld us with %d bytes", recv_time - last_send_time, data_len);
}

/**
 * @brief Get mac addr of the current ESP32 over serial
 * 
 * @return esp_err_t 
 */
esp_err_t get_macAddr() {
    uint8_t mac[6] = {0};
    esp_err_t err = ESP_OK;

    err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "Station MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    err = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "AP MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    err = esp_read_mac(mac, ESP_MAC_BT);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "Bluetooth MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    err = esp_read_mac(mac, ESP_MAC_ETH);
    if (err != ESP_OK)
        return err;
    ESP_LOGI("MAC", "Ethernet MAC addr: %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return err;
}
