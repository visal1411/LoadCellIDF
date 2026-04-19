idf_component_register(
	SRCS "hx711.c"
	INCLUDE_DIRS "."
	REQUIRES esp_driver_gpio esp_timer freertos esp_rom
)