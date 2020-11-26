/*
 * BME280 Resource Header file
 */
#ifndef USER_RESOURCES_BME280_RES_H_
#define USER_RESOURCES_BME280_RES_H_

    #include <bmp280.h>

    #if defined(CONFIG_IDF_TARGET_ESP8266)
    #define SDA_GPIO 4
    #define SCL_GPIO 5
    #else
    #define SDA_GPIO 25
    #define SCL_GPIO 26
    #endif

    void BME280_Run(void *pvParamters);

    void BME280_Notify_Handler(void);

    extern coap_resource_t* BME280_Resource;

    coap_resource_t* init_BME280_resource(coap_context_t *ctx);

#endif /* USER_RESOURCES_BME280__RES_H_ */
