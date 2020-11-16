/*
 * DHT11 Resource Header file
 */
#ifndef USER_RESOURCES_DHT11_RES_H_
#define USER_RESOURCES_DHT11_RES_H_

	#include <dht.h>

	#define GPIO 32
	#define SENSORTYPE DHT_TYPE_DHT11
	
	//use vTaskCreate to run sensor
	void DHT11_Run(void *pvParameters);

	void DHT11_Notify_Handler(void);

	//change sensor type for different sensors maybe
	// const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;

	// const gpio_num_t dht_gpio = 32;

	coap_resource_t* init_DHT11_resource(coap_context_t *ctx);

	extern coap_resource_t* DHT11_Resource;


#endif /* USER_RESOURCES_DHT11__RES_H_ */
