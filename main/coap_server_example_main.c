/* CoAP server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * WARNING
 * libcoap is not multi-thread safe, so only this thread must make any coap_*()
 * calls.  Any external (to this thread) data transmitted in/out via libcoap
 * therefore has to be passed in/out by xQueue*() via this thread.
 */
//ESP-IDF Headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "nvs_flash.h"
//WiFi
#include "protocol_examples_common.h"

#if 1
/* Needed until coap_dtls.h becomes a part of libcoap proper */
#include "libcoap.h"
#include "coap_dtls.h"
#endif
#include "coap.h"

//add Ressources
#include "DHT11_Resource.h"
#include "BME280_Resource.h"



/* The examples use CoAP Logging Level that
   you can set via 'make menuconfig'.

   If you'd rather not, just change the below entry to a value
   that is between 0 and 7 with
   the config you want - ie #define EXAMPLE_COAP_LOG_DEFAULT_LEVEL 7
*/
#define EXAMPLE_COAP_LOG_DEFAULT_LEVEL CONFIG_COAP_LOG_DEFAULT_LEVEL

const static char *TAG = "CoAP_server";


static char espressif_data[100];
static int espressif_data_len = 0;

/*String test defines*/
#define INITIAL_DATA "Hi, my name is Ameer"


/*ESPRESSIF Handlers Start*/
static void
hnd_espressif_get(coap_context_t *ctx, coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request, coap_binary_t *token,
                  coap_string_t *query, coap_pdu_t *response)
{
    coap_add_data_blocked_response(resource, session, request, response, token,
                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
                                   (size_t)espressif_data_len,
                                   (const u_char *)espressif_data);
}

static void
hnd_espressif_put(coap_context_t *ctx,
                  coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request,
                  coap_binary_t *token,
                  coap_string_t *query,
                  coap_pdu_t *response)
{
    size_t size;
    unsigned char *data;

    coap_resource_notify_observers(resource, NULL);

    if (strcmp (espressif_data, INITIAL_DATA) == 0) {
        response->code = COAP_RESPONSE_CODE(201);
    } else {
        response->code = COAP_RESPONSE_CODE(204);
    }

    /* coap_get_data() sets size to 0 on error */
    (void)coap_get_data(request, &size, &data);

    if (size == 0) {      /* re-init */
        snprintf(espressif_data, sizeof(espressif_data), INITIAL_DATA);
        espressif_data_len = strlen(espressif_data);
    } else {
        espressif_data_len = size > sizeof (espressif_data) ? sizeof (espressif_data) : size;
        memcpy (espressif_data, data, espressif_data_len);
    }
}

static void
hnd_espressif_delete(coap_context_t *ctx,
                     coap_resource_t *resource,
                     coap_session_t *session,
                     coap_pdu_t *request,
                     coap_binary_t *token,
                     coap_string_t *query,
                     coap_pdu_t *response)
{
    coap_resource_notify_observers(resource, NULL);
    snprintf(espressif_data, sizeof(espressif_data), INITIAL_DATA);
    espressif_data_len = strlen(espressif_data);
    response->code = COAP_RESPONSE_CODE(202);
}
///////////////////////////////ESPRESSIF HANDLERS end

//todo Generic Handlers to handle all our resources

static void hnd_generic_get(coap_context_t *ctx, coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request, coap_binary_t *token,
                  coap_string_t *query, coap_pdu_t *response)
{
	coap_str_const_t *uri_path = coap_resource_get_uri_path(resource);

	//if path not found then send bad request
	if(!uri_path)
	{
		response->code = COAP_RESPONSE_CODE(400);
		return;
	}

	if(coap_string_equal(uri_path, coap_make_str_const("Espressif")))
	{
		hnd_espressif_get(ctx, resource, session, request, token, query, response);
		return;
	}


	//total failure
	response->code = COAP_RESPONSE_CODE(400);

}

/* End of generic handlers*/


//coap comms thread
static void coap_example_server(void *p)
{
    coap_context_t *ctx = NULL;
    coap_address_t serv_addr;
    coap_resource_t *resource = NULL;
   

    snprintf(espressif_data, sizeof(espressif_data), INITIAL_DATA);
    espressif_data_len = strlen(espressif_data);

   

    coap_set_log_level(EXAMPLE_COAP_LOG_DEFAULT_LEVEL);

    while (1) {
        coap_endpoint_t *ep = NULL;
        unsigned wait_ms;

        /* Prepare the CoAP server socket */
        coap_address_init(&serv_addr);
        serv_addr.addr.sin.sin_family      = AF_INET;
        serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
        serv_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);

        ctx = coap_new_context(NULL);
        if (!ctx) {
            ESP_LOGE(TAG, "coap_new_context() failed");
            //continue;
        }

        ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP);
        if (!ep) {
            ESP_LOGE(TAG, "udp: coap_new_endpoint() failed");
            goto clean_up;
        }
        ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_TCP);
        if (!ep) {
            ESP_LOGE(TAG, "tcp: coap_new_endpoint() failed");
            goto clean_up;
        }
        /* Espressif example resource and handlers */
        resource = coap_resource_init(coap_make_str_const("Espressif"), 0);
        if (!resource) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }

        coap_register_handler(resource, COAP_REQUEST_GET, hnd_generic_get);
        coap_register_handler(resource, COAP_REQUEST_PUT, hnd_espressif_put);
        coap_register_handler(resource, COAP_REQUEST_DELETE, hnd_espressif_delete);
        /* We possibly want to Observe the GETs */
        coap_resource_set_get_observable(resource, 1);
        coap_add_resource(ctx, resource);
        /* End of Espressif */
        /* DHT11 Resource */
        init_DHT11_resource(ctx);
        init_BME280_resource(ctx);
        /* End of DHT11 Resource */

        wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

        while (1) {
            int result = coap_run_once(ctx, wait_ms);
            if (result < 0)
            {
                break;
            }
            else if (result && (unsigned)result < wait_ms)
            {
                /* decrement if there is a result wait time returned */
                wait_ms -= result;

            }
            else
            {
            //TODO observe every second if the resource change?
    	    
            }

            DHT11_Notify_Handler();
            BME280_Notify_Handler();

            if (result)
            {
                /* result must have been >= wait_ms, so reset wait_ms */
                wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
            }
        }
    }
clean_up:
    coap_free_context(ctx);
    coap_cleanup();

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function. 
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(i2cdev_init());

    xTaskCreate(coap_example_server, "coap", 8 * 1024, NULL, 8, NULL);
    xTaskCreate(DHT11_Run, "DHT11", 8 * 1024, NULL, 6, NULL);
    xTaskCreate(BME280_Run, "BME280", 8 * 1024, NULL, 6, NULL);

}
