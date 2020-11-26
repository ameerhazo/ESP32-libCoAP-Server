set(component_dirs coap bmp280)

idf_component_register(SRCS "BME280_Resource.c"
                    INCLUDE_DIRS "."
                    REQUIRES "${component_dirs}")
