#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "bios.h"

#define MAX_ARGS 8
#define BUFFER_SIZE 64

// --- FUNCIONES DE APOYO (Capa intermedia) ---

void print_biosstring(const char *p) {
    while (*p != '\0') {
        bios_putchar(*p);
        p++;
    }
}

void gets_biosstring(char *buffer, int size) {
    int i = 0;
    while (i < size - 1) {
        char c = bios_getchar();
        
        // Manejar Enter (CR o LF)
        if (c == '\r' || c == '\n') break;
        
        // Manejar Backspace (Borrar)
        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                print_biosstring("\b \b"); 
            }
            continue;
        }

        bios_putchar(c); // Echo: para ver lo que escribes
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    print_biosstring("\r\n");
}

// --- INTÉRPRETE DE COMANDOS ---

void ejecutar_interprete() {
    char linea[BUFFER_SIZE];
    char *argv[MAX_ARGS];
    int argc;

    print_biosstring("\r\nSISTEMA INICIADO - BIOS EN RAM\r\n");

    while (1) {
        print_biosstring("> ");
        
        // Leemos la línea usando nuestra nueva función
        gets_biosstring(linea, sizeof(linea));

        // --- TOKENIZACIÓN ---
        argc = 0;
        char *token = strtok(linea, " \n\r");
        
        while (token != NULL && argc < MAX_ARGS) {
            argv[argc] = token;
            argc++;
            token = strtok(NULL, " \n\r");
        }

        if (argc == 0) continue; 

        // --- LÓGICA DE COMANDOS ---
        if (strcmp(argv[0], "PRINT") == 0) {
            if (argc > 1) {
                print_biosstring(argv[1]); 
                print_biosstring("\r\n");
            } else {
                print_biosstring("ERROR: Nada que imprimir\r\n");
            }
        } 
        else if (strcmp(argv[0], "HELP") == 0) {
            print_biosstring("Comandos: PRINT <texto>, HELP\r\n");
        }
        else {
            print_biosstring("ERROR: Comando desconocido\r\n");
        }
    }
}

int main() {
    // Inicialización del hardware (SDK)
    stdio_init_all();
    
    // Opcional: Configura el LED de la placa para confirmar arranque
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    // Esperamos 2 segundos para que puedas abrir minicom
    sleep_ms(2000);
    
    ejecutar_interprete();
    
    return 0;
}
