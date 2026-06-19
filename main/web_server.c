#include "web_server.h"
#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include "esp_netif.h"
#include <esp_http_server.h>
#include "freertos/task.h"

// Настройки создаваемой Wi-Fi сети (Точка доступа)
#define AP_WIFI_SSID      "ESP32_C3_TwoWay"  // Имя сети, которая появится в списке
#define AP_WIFI_PASS      "12345678"          // Пароль (минимум 8 символов)
#define AP_MAX_CONN       4                   // Макс. количество подключенных устройств

static const char *TAG = "web_system";

QueueHandle_t web_text_queue = NULL;
QueueHandle_t tx_to_phone_queue = NULL;

static char rx_buffer[MAX_TEXT_LEN + 1];
static char current_status_text[MAX_TEXT_LEN + 1] = "No data from ESP32 internal logic yet";

// HTML страница с JavaScript, который каждую секунду запрашивает текст с ESP32
const char html_page[] = 
"<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<title>ESP32 AP Two-Way</title>"
"<style>"
"  body{font-family:sans-serif; margin:20px; background:#f4f7f6; color:#333;}"
"  .box{background:#fff; padding:15px; border-radius:8px; box-shadow:0 2px 4px rgba(0,0,0,0.1); margin-bottom:20px;}"
"  textarea{width:100%; max-width:400px; padding:10px; border-radius:5px; border:1px solid #ccc;}"
"  input[type='submit']{padding:10px 20px; background:#007bff; color:#fff; border:none; border-radius:5px; cursor:pointer;}"
"</style>"
"<script>"
"  setInterval(function() {"
"    fetch('/get-status').then(response => response.text()).then(data => {"
"      document.getElementById('esp_text').innerText = data;"
"    });"
"  }, 1000);" // Опрос сервера раз в секунду
"</script>"
"</head><body>"
"  <h2>ESP32 AP Control Panel</h2>"
"  <div class=\"box\">"
"    <strong>Text from ESP32 Core:</strong>"
"    <p id=\"esp_text\">Loading...</p>"
"  </div>"
"  <form action=\"/submit\" method=\"POST\">"
"    <h3>Send text to ESP32 Screen:</h3>"
"    <textarea name=\"text_data\" rows=\"5\" maxlength=\"1000\" placeholder=\"Type English text here...\"></textarea><br><br>"
"    <input type=\"submit\" value=\"Send to Screen\">"
"  </form>"
"</body></html>";

/* 1. Отдача главной страницы */
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
static const httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };

/* 2. Отдача текущего текста телефону по AJAX-запросу */
static esp_err_t status_get_handler(httpd_req_t *req) {
    char new_msg[MAX_TEXT_LEN + 1];
    // Проверяем наличие новых данных от main.c без блокировки потока (timeout = 0)
    if (xQueueReceive(tx_to_phone_queue, &new_msg, 0) == pdPASS) {
        strncpy(current_status_text, new_msg, MAX_TEXT_LEN);
        current_status_text[MAX_TEXT_LEN] = '\0';
    }
    
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_send(req, current_status_text, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
static const httpd_uri_t status_uri = { .uri = "/get-status", .method = HTTP_GET, .handler = status_get_handler };

/* 3. Прием текста от телефона */
static esp_err_t submit_post_handler(httpd_req_t *req) {
    int total_len = req->content_len;
    int cur_len = 0, received = 0;
    if (total_len >= sizeof(rx_buffer)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, rx_buffer + cur_len, total_len - cur_len);
        if (received <= 0) { if (received == HTTPD_SOCK_ERR_TIMEOUT) continue; return ESP_FAIL; }
        cur_len += received;
    }
    rx_buffer[total_len] = '\0';
    char *value_ptr = strstr(rx_buffer, "text_data=");
    if (value_ptr != NULL) {
        value_ptr += 10;
        for (char *p = value_ptr; *p != '\0'; p++) { if (*p == '+') *p = ' '; }
        char queue_msg[MAX_TEXT_LEN + 1];
        strncpy(queue_msg, value_ptr, MAX_TEXT_LEN);
        queue_msg[MAX_TEXT_LEN] = '\0';
        xQueueSend(web_text_queue, &queue_msg, pdMS_TO_TICKS(10));
        httpd_resp_send(req, "<h3>Success! Sent to screen.</h3><a href='/'>Back</a>", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}
static const httpd_uri_t submit_uri = { .uri = "/submit", .method = HTTP_POST, .handler = submit_post_handler };

static void http_server_task(void *pvParameters) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 10240;
    
    // Применяем системный фикс ошибки 431 для работы с большими текстами

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &submit_uri);
        httpd_register_uri_handler(server, &status_uri);
        ESP_LOGW(TAG, "HTTP Server active on http://192.168.4.1");
    }
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Client connected to ESP32 Access Point");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Client disconnected from ESP32 Access Point");
    }
}

// Переведенная в режим Точки Доступа инициализация Wi-Fi
static void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_WIFI_SSID,
            .ssid_len = strlen(AP_WIFI_SSID),
            .channel = 1,
            .password = AP_WIFI_PASS,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Access Point '%s' started successfully.", AP_WIFI_SSID);
}

void init_web_system(void) {
    // Выделяем память под две независимые очереди
    web_text_queue = xQueueCreate(2, MAX_TEXT_LEN + 1);
    tx_to_phone_queue = xQueueCreate(2, MAX_TEXT_LEN + 1);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Запускаем режим раздачи Wi-Fi сети
    wifi_init_softap();

    xTaskCreate(http_server_task, "http_server_task", 4096, NULL, 5, NULL);
}
