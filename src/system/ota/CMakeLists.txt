idf_component_register(SRCS "*.cpp"
        INCLUDE_DIRS ".")

if(CONFIG_SUPPORT_OTA_UPDATE)
    set_property(TARGET ${COMPONENT_LIB} PROPERTY EXCLUDE_FROM_ALL 1)
endif()
