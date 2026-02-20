# 1. Versión mínima de CMake
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "/home/jfvera/pico/pico-sdk")

# 2. Importar el SDK de la Raspberry Pi Pico
# Nota: Debes tener el archivo 'pico_sdk_import.cmake' en la misma carpeta
include(pico_sdk_import.cmake)

# 3. Definir el nombre del proyecto y lenguaje
project(mi_proyecto_pico C CXX ASM)

# 4. Inicializar el SDK
pico_sdk_init()

# 5. Añadir el ejecutable (nombre del exe y archivo fuente)
add_executable(mi_proyecto
    main.c
)

# 6. Vincular las librerías estándar del SDK (pico_stdlib)
# Aquí puedes añadir más como 'hardware_adc', 'hardware_pwm', etc.
target_link_libraries(mi_proyecto 
    pico_stdlib
)

# 7. Habilitar la salida por USB (Serial) y/o UART
pico_enable_stdio_usb(mi_proyecto 1)
pico_enable_stdio_uart(mi_proyecto 0)

# 8. Crear los archivos extra (.uf2, .bin, .elf) para flashear
pico_add_extra_outputs(mi_proyecto)

