/*
 * DHT Resource
 */
/*
 * String Header
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * libCoAP Header
 */
#if 1
#include "libcoap.h"
#include "coap_dtls.h"
#endif
#include "coap.h"

/*FreeRTOS for vTask*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
/*
 * DHT Header and intitialization of sensor
 */
//#include <dht.h>
#include "DHT11_Resource.h" //maybe put constants in header file?

const dht_sensor_type_t sensor_type = SENSORTYPE;

const gpio_num_t dht_gpio = GPIO;
/*
 * Payload Messages and variables
 */
static char dht11_data[100];
static int dht_data_len = 0;

// int16_t temperature = 0;
// int16_t humidity = 0;
int16_t temperature = 0;
int16_t lastcompareTemp = 0;
int16_t humidity = 0;
int16_t lastcompareHum = 0;

const static char *RunSensor = "DHT11";
const static char *Notify = "Notify_handler";

#define QUERYERRORINFO "Wrong query ?temp or ?hum"
#define NOQUERYGIVEN "use ?temp or ?hum to access resource"
#define HUMERROR "Error reading humidity DHT11"
#define TEMPERROR "Error reading temp DHT11"

/*
 * initialize ressource
 */
	coap_resource_t *DHT11_Resource = NULL;

/*Request Handlers*/
static void
hnd_dht11_get(coap_context_t *ctx, coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request, coap_binary_t *token,
                  coap_string_t *query, coap_pdu_t *response)
{
	
	if(query == NULL)
		{
					snprintf(dht11_data, sizeof(dht11_data), NOQUERYGIVEN);
					dht_data_len = strlen(dht11_data);
					coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)dht_data_len,
																	   (const u_char *)dht11_data);
		}

	if(query != NULL && coap_string_equal(query, coap_make_str_const("temp")))
	{
                if(temperature)
                {
                    snprintf(dht11_data, sizeof(dht11_data), "Temperature: %dC", temperature);
                }
				dht_data_len = strlen(dht11_data);
                coap_add_data_blocked_response(resource, session, request, response, token,
																			COAP_MEDIATYPE_TEXT_PLAIN, 0,
																			(size_t)dht_data_len,
																			(const u_char *)dht11_data);
	}
	else if (query != NULL && coap_string_equal(query, coap_make_str_const("hum")))
	{
		if(humidity)
                {
                    snprintf(dht11_data, sizeof(dht11_data), "Humidity: %d%%", humidity);
                }
				dht_data_len = strlen(dht11_data);
                coap_add_data_blocked_response(resource, session, request, response, token,
																			COAP_MEDIATYPE_TEXT_PLAIN, 0,
																			(size_t)dht_data_len,
																			(const u_char *)dht11_data);
	}
	else
	{
				response->code = COAP_RESPONSE_CODE(400);
	}


}
//TODO POST Handler to change our Sampling time with a client (key as query)
/* End Request Handlers*/

/*Notify Handler*/
void DHT11_Notify_Handler(void)
{
    // int len1 = strlen(TEMP);
    // int len2 = strlen(HUM);
    // const coap_string_t *t = {(size_t)len1, (uint8_t*)TEMP};
    // const coap_string_t *h = {(size_t)len2, (uint8_t*)HUM};


        //temperature notify handler
		if(temperature != lastcompareTemp)
		{
			lastcompareTemp = temperature;
    		ESP_LOGE(Notify,"Temp change: %dC\n",lastcompareTemp);
			coap_resource_notify_observers(DHT11_Resource, NULL);
		}

        //humidity notify handler
        if(humidity != lastcompareHum)
		{
			lastcompareHum = humidity;
            ESP_LOGE(Notify,"Hum Change: %d%%\n",lastcompareHum);
			coap_resource_notify_observers(DHT11_Resource, NULL);
		}
}
/*Notify Handler end*/

//initiate resource
coap_resource_t* init_DHT11_resource(coap_context_t *ctx)
{
	coap_resource_t *res;

	//create DHT11 resource
	res = coap_resource_init(coap_make_str_const("Sensors/DHT11"), COAP_RESOURCE_FLAGS_NOTIFY_CON);

	//register handler
	coap_register_handler(res, COAP_REQUEST_GET, hnd_dht11_get);

	//TODO test if coap_register_handler can be used to call notify handler from outside main file
	//coap_register_handler(res, COAP_REQUEST_GET,)

	coap_resource_set_get_observable(res,1);

	coap_add_resource(ctx,res);

	return DHT11_Resource =  res;

}

//TODO use put to change vtaskDelay on the fly
void DHT11_Run(void *pvParameters)
{
    while(1)
    {
        if(dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK)
        {

        temperature = temperature / 10;
        humidity = humidity / 10;
        ESP_LOGE(RunSensor, "Humidity: %d%% Temp: %dC\n",humidity,temperature);
        }
        else
        {
        temperature = NULL;
        humidity = NULL;
        snprintf(dht11_data, sizeof(dht11_data), "Could not read data from sensor");
        ESP_LOGE(RunSensor, "Could not read data from sensor");
        }
        
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
}