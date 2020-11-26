/*
 * BME Resource
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include "esp_log.h"
/*
 * BME Header and configuring default sensor mode
 */
#include "BME280_Resource.h" //maybe put constants in header file?


/*
 * Payload Messages and variables
 */
static char bme280_data[100];
static int bme280_data_len = 0;

float pressure, temperature, humidity;
float lastcomparePressure, lastcompareTemperature, lastcompareHumidity;

int lastSamplingRateBME = 0;
int samplingRateBME = 1500;

const static char *RunSensor = "BME280_Debug";
const static char *Notify = "BME280_Notify_handler";

#define QUERYERRORINFO "Wrong query ?temp, ?hum or ?press"
#define NOQUERYGIVEN "use  ?temp, ?hum or ?press to access resource"
#define NOTIFYSTARTING "Notify compare starting"
#define NOKEY "Enter the correct key ?key="

/*
 * initialize ressource
 */

coap_resource_t *BME280_Resource = NULL;

/*Request Handlers*/
static void
hnd_bme280_get(coap_context_t *ctx, coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request, coap_binary_t *token,
                  coap_string_t *query, coap_pdu_t *response)
{
	
	if(query == NULL)
	{
		snprintf(bme280_data, sizeof(bme280_data), NOQUERYGIVEN);
		bme280_data_len = strlen(bme280_data);
		coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																	   (const u_char *)bme280_data);
	}
	
	if(query != NULL && coap_string_equal(query, coap_make_str_const("key=123")))
	{
		snprintf(bme280_data, sizeof(bme280_data), "Sampling rate= %d", samplingRateBME);
		bme280_data_len = strlen(bme280_data);
		coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																   (const u_char *)bme280_data);
	}

	if(query != NULL && coap_string_equal(query, coap_make_str_const("temp")))
	{
        if(temperature != 0.00)
         {
              snprintf(bme280_data, sizeof(bme280_data), "Temperature: %.2fC", temperature);
         }
		bme280_data_len = strlen(bme280_data);
        coap_add_data_blocked_response(resource, session, request, response, token,
																			COAP_MEDIATYPE_TEXT_PLAIN, 0,
																			(size_t)bme280_data_len,
																			(const u_char *)bme280_data);
	}
	else if (query != NULL && coap_string_equal(query, coap_make_str_const("hum")))
	{
		if(humidity != 0.00)
        {
            snprintf(bme280_data, sizeof(bme280_data), "Humidity: %.2f%%", humidity);
        }
		bme280_data_len = strlen(bme280_data);
        coap_add_data_blocked_response(resource, session, request, response, token,
																			COAP_MEDIATYPE_TEXT_PLAIN, 0,
																			(size_t)bme280_data_len,
																			(const u_char *)bme280_data);
	}
    else if(query != NULL && coap_string_equal(query, coap_make_str_const("press")))
    {
       if(pressure != 0.00)
        {
            snprintf(bme280_data, sizeof(bme280_data), "Pressure: %.2f Pa", pressure);
        }
		bme280_data_len = strlen(bme280_data);
        coap_add_data_blocked_response(resource, session, request, response, token,
																			COAP_MEDIATYPE_TEXT_PLAIN, 0,
																			(size_t)bme280_data_len,
																			(const u_char *)bme280_data); 
    }
	else
	{
				response->code = COAP_RESPONSE_CODE(400);
	}


}

static void
hnd_bme280_put(coap_context_t *ctx, coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request, coap_binary_t *token,
                  coap_string_t *query, coap_pdu_t *response)
{
	size_t size;
    unsigned char *data;

	if(query == NULL)
	{
		snprintf(bme280_data, sizeof(bme280_data), NOKEY);
		bme280_data_len = strlen(bme280_data);
		coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																	   (const u_char *)bme280_data);
	}

	if (query != NULL && !coap_string_equal(query, coap_make_str_const("key=123")))
	{
		response->code = COAP_RESPONSE_CODE(401);
	}

	if(query != NULL && coap_string_equal(query, coap_make_str_const("key=123")))
	{
		

		(void)coap_get_data(request, &size, &data);

		if(size != 0) //there is payload
		{
			bme280_data_len = size > sizeof(bme280_data) ? sizeof (bme280_data) : size;
        	memcpy (bme280_data, data, bme280_data_len);

			int newSamplingRateBME = atoi(bme280_data); //convert data in payload to 

			if(newSamplingRateBME == 0) //payload is not an alphanumeric number
			{
				snprintf(bme280_data, sizeof(bme280_data), "Not an integer");
				bme280_data_len = strlen(bme280_data);
				coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																	   (const u_char *)bme280_data);
			}
			else if(newSamplingRateBME == samplingRateBME)
			{
				snprintf(bme280_data, sizeof(bme280_data), "No change in sampling rate");
				bme280_data_len = strlen(bme280_data);
				coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																	   (const u_char *)bme280_data);
			}
			else if(newSamplingRateBME < 800 || newSamplingRateBME > 10000) //change limit 
			{
				snprintf(bme280_data, sizeof(bme280_data), "Sampling rate too fast use >= 1000");
				bme280_data_len = strlen(bme280_data);
				coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																	   (const u_char *)bme280_data);
			}
			else
			{
				samplingRateBME = newSamplingRateBME;
				response->code = COAP_RESPONSE_CODE(204); //changed
			}
		}
		else //no payload
		{
			snprintf(bme280_data, sizeof(bme280_data), "No data given");
			bme280_data_len = strlen(bme280_data);
			coap_add_data_blocked_response(resource, session, request, response, token,
									                                   COAP_MEDIATYPE_TEXT_PLAIN, 0,
									                                   (size_t)bme280_data_len,
																	   (const u_char *)bme280_data);
		}	
	}	
}

/*Notify Handler*/
void BME280_Notify_Handler(void)
{
        //temperature notify handler
    if(temperature != lastcompareTemperature)
    {
        float tempDiff = temperature - lastcompareTemperature;
        if(tempDiff > 1.50 || tempDiff < -1.50)
        {
            lastcompareTemperature = temperature;
            ESP_LOGW(Notify,"Temp change: %.2fC\n",lastcompareTemperature);
			coap_resource_notify_observers(BME280_Resource, NULL);
        }
    }	

        //humidity notify handler
    if(humidity != lastcompareHumidity)
    {
        float humDiff = humidity - lastcompareHumidity;
        if(humDiff > 5.00 || humDiff < -5.00)
        {
            lastcompareHumidity = humidity;
            ESP_LOGW(Notify,"Hum Change: %.2f%%\n",lastcompareHumidity);
			coap_resource_notify_observers(BME280_Resource, NULL);
        }
    }
        
	if(pressure != lastcomparePressure)
    {
        float pressDiff = pressure - lastcomparePressure;
        if(pressDiff > 50.00 || pressDiff < -50.00)
        {
            lastcomparePressure = pressure;
            ESP_LOGW(Notify,"Pressure Change: %.2f Pa\n",lastcomparePressure);
			coap_resource_notify_observers(BME280_Resource, NULL);
        }
    }

		if(samplingRateBME != lastSamplingRateBME)
		{
			lastSamplingRateBME = samplingRateBME;
			coap_resource_notify_observers(BME280_Resource, NULL);
		}

}
/*Notify Handler end*/

//initiate resource
coap_resource_t* init_BME280_resource(coap_context_t *ctx)
{
	coap_resource_t *res;

	//create BME280 resource
	res = coap_resource_init(coap_make_str_const("Sensors/BME280"), COAP_RESOURCE_FLAGS_NOTIFY_CON);

	//register handler
	coap_register_handler(res, COAP_REQUEST_GET, hnd_bme280_get);
	coap_register_handler(res, COAP_REQUEST_PUT, hnd_bme280_put);

	coap_resource_set_get_observable(res,1);

	coap_add_resource(ctx,res);

	return BME280_Resource =  res;

}




void BME280_Run(void *pvParamters)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev;
    memset(&dev, 0, sizeof(bmp280_t));

    ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, SDA_GPIO, SCL_GPIO));
    ESP_ERROR_CHECK(bmp280_init(&dev, &params));

    bool bme280p = dev.id == BME280_CHIP_ID;
    printf("BMP280: found %s\n", bme280p ? "BME280" : "BMP280");

   
    while (1)
    {
        vTaskDelay(samplingRateBME / portTICK_PERIOD_MS);
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
        {
            temperature = 0.00;
            humidity = 0.00;
            pressure = 0.00;
            snprintf(bme280_data, sizeof(bme280_data), "Could not read data from sensor");
            ESP_LOGE(RunSensor, "Could not read data from sensor");
            continue;
        }
        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        float tempDiffDebug = temperature - lastcompareTemperature;
        float humDiffDebug = humidity - lastcompareHumidity;
        float pressDiffDebug = pressure - lastcomparePressure;
            
        if (bme280p)
        {
            ESP_LOGI(RunSensor,"Pressure: %.2f Pa, Temperature: %.2f C, Humidity: %.2f%%\n", pressure, temperature, humidity);
        }
        else
        {
            printf("\n");
        }
        ESP_LOGW(RunSensor,"Temp diff: %.2fC, Last compare temp: %.2fC\n", tempDiffDebug, lastcompareTemperature);
        ESP_LOGW(RunSensor,"Pressure diff: %.2f, Last compare pressure: %.2f Pa\n",pressDiffDebug, lastcomparePressure);
        ESP_LOGW(RunSensor,"Hum diff: %.2f%%, Last compare hum: %.2f%%\n",humDiffDebug, lastcompareHumidity);
    }
}

