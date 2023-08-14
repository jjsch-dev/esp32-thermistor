#set(COMPONENT_ADD_INCLUDEDIRS include)
#set(COMPONENT_SRCS "thermistor.c")
#set(COMPONENT_REQUIRES esp_adc_cal)
#register_component()
idf_component_register(SRCS "thermistor.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_adc)
