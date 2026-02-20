#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "bios.h"

#define MAX_ARGS 8
#define BUFFER_SIZE 64

// Función que recorre una cadena usando tu bios_putchar
void print_string(const char *p) {
    while (*p != '\0') {     // Mientras el 'habitante' no sea el nulo
        bios_putchar(*p);    // Pasamos el contenido (*p)
        p++;                 // Avanzamos a la siguiente dirección
    }
}

void ejecutar_interprete() {
    char linea[BUFFER_SIZE];
    char *argv[MAX_ARGS];
    int argc;

    print_string("\r\nSISTEMA INICIADO - BIOS EN RAM\r\n");

    while (1) {
        print_string("> ");
        
        // Leemos la línea del usuario
        if (!fgets(linea, sizeof(linea), stdin)) continue;

        // --- TOKENIZACIÓN ---
        argc = 0;
        char *token = strtok(linea, " \n\r");
        
        while (token != NULL && argc < MAX_ARGS) {
            argv[argc] = token; // Guardamos la dirección del trozo
            argc++;
            token = strtok(NULL, " \n\r"); // Pedimos el siguiente trozo
        }

        if (argc == 0) continue; // Si solo pulsó Enter

        // --- BUSQUEDA DE COMANDOS (SWITCH) ---
        // Usamos argv[0] que es el primer token (el comando)
        if (strcmp(argv[0], "PRINT") == 0) {
            if (argc > 1) {
                print_string(argv[1]); // Imprime la primera palabra tras PRINT
                print_string("\r\n");
            } else {
                print_string("ERROR: Nada que imprimir\r\n");
            }
        } 
        else if (strcmp(argv[0], "HELP") == 0) {
            print_string("Comandos: PRINT <texto>, HELP\r\n");
        }
        else {
            print_string("ERROR: Comando desconocido\r\n");
        }
    }
}

int main() {
    stdio_init_all(); // Inicializa USB/UART del SDK
    sleep_ms(2000);   // Espera para abrir la terminal
    ejecutar_interprete();
    return 0;
}
