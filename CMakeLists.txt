# Utils
list(APPEND srcs "utils/src/os_utils.c")
list(APPEND incdirs "utils/inc")

# HILs

list(APPEND srcs "hil/src/lis2dw12_hil.c")
list(APPEND srcs "hil/src/hbridge_dir_hil.c")
list(APPEND incdirs "hil/inc")

# Drivers

list(APPEND srcs "drivers/src/buttons_driver.c")
list(APPEND srcs "drivers/src/leds_driver.c")
list(APPEND srcs "drivers/src/accelerometer_driver.c")
list(APPEND srcs "drivers/src/brushed_motor_driver.c")
list(APPEND incdirs "drivers/inc")

# APIs

list(APPEND srcs "apis/src/mcpwm_api.c")
list(APPEND incdirs "apis/inc")

# Services

if(CONFIG_FWK_PID_SERVICE)
    list(APPEND srcs "services/src/pid_service.c")
endif()
list(APPEND incdirs "services/inc")

list(APPEND incdirs "inc")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incdirs}")
