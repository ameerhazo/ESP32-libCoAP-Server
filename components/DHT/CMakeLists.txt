set(component_dirs coap dht)

idf_component_register(SRCS "DHT11_Resource.c"
                    INCLUDE_DIRS "."
                    REQUIRES "${component_dirs}")
