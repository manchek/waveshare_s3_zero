set(COMPONENT_ADD_INCLUDEDIRS .)
set(COMPONENT_NAME ".")

idf_component_register(
	SRCS
		"libdow.c"
	INCLUDE_DIRS
		"."
	REQUIRES
		"driver"
		"esp_hw_support"
		"esp_timer"
)
