#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "bios.h"

#define MAX_ARGS 8
#define BUFFER_SIZE 64

#define MAX_LINEAS 100
#define TAM_LINEA 64
char programa[MAX_LINEAS][TAM_LINEA];

//  --- Definimos la estructura para almacenar las variables ----

typedef enum { T_NUMBER, T_STRING } VarType;

typedef struct {
    char nombre[16];   // Nombre de la variable (ej: "ANCHO", "NOMBRE$")
    VarType tipo;      // ¿Es número o texto?
    union {
        double valor_num;
        char *valor_str; // Puntero a la cadena en memoria (heap)
    };
    bool ocupada;
} Variable;


// ---  Definimos la tabla donde se almacenan las variables ----

#define MAX_VARS 64
Variable tabla_variables[MAX_VARS];


// -- Definimos los tipos de comandos posibles ---


enum { ERROR = -1, HELP, PRINT, CLS, RUN, LET };

int identificar_comando(char *input) {
		if ((strcmp(input,"print") == 0) ||  (strcmp(input,"PRINT") == 0)) return PRINT;
		if ((strcmp(input,"help") == 0) ||  (strcmp(input,"HELP") == 0)) return HELP;
		if ((strcmp(input,"cls") == 0) ||  (strcmp(input,"CLS") == 0)) return CLS;
		if ((strcmp(input,"run") == 0) ||  (strcmp(input,"RUN") == 0)) return RUN;   				 
		if ((strcmp(input,"let") == 0) ||  (strcmp(input,"LET") == 0)) return LET; 
}


// --- Función que hace la búsqueda de una variable ----

int buscar_variable(char *nombre) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (tabla_variables[i].ocupada && strcmp(tabla_variables[i].nombre, nombre) == 0) {
            return i;
        }
    }
    return -1; // No existe
}

// --- FUNCIONES DE APOYO (Capa intermedia) ---

void print_biosstring(const char *p) {
    while (*p != '\0') {
        bios_putchar(*p);while (*p && *p != '\r' && *p != '\n') {                                                                                                              
         // CASO: CADENA LITERAL "HOLA"                                                                                                                    
         if (*p == '"') {                                                                                                                                  
             p++;                                                                                                                                          
             while (*p && *p != '"') bios_putchar(*p++);                                                                                                   
             if (*p == '"') p++;                                                                                                                           
         }                                                                                                                                                 
         // CASO: VARIABLE (ANCHO, NOMBRE$)                                                                                                                
         else if (isalpha(*p)) {                                                                                                                           
             char nombre_var[16];                                                                                                                          
             int i = 0;                                                                                                                                    
             // Extraemos el nombre (letras, números o el signo $ para strings)                                                                            
             while (isalnum(*p) || *p == '$') {                                                                                                            
                 if (i < 15) nombre_var[i++] = *p;                                                                                                         
                 p++;                                                                                                                                      
             }                                                                                                                                             
             nombre_var[i] = '\0';         
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

// ---- FUNCIÓN QUE SALTA LOS ESPACIOS EN BLANCO DE LA LÍNEA DE COMANDOS ---

void skip_spaces(char **ptr) {
    while (**ptr && isspace(**ptr)) {
        (*ptr)++;
    }
}

// Prototipos necesarios para que las funciones se conozcan entre sí
double evaluar_expresion();
double evaluar_termino();
double evaluar_factor();

// 1. NIVEL: Sumas y Restas (Prioridad baja)
double evaluar_expresion() {
    double resultado = evaluar_termino();
    saltar_espacios();

    while (*cursor == '+' || *cursor == '-') {
        char op = *cursor;
        cursor++; // Avanzar tras el '+' o '-'
        if (op == '+') resultado += evaluar_termino();
        else resultado -= evaluar_termino();
        saltar_espacios();
    }
    return resultado;
}

// 2. NIVEL: Multiplicaciones y Divisiones (Prioridad media)
double evaluar_termino() {
    double resultado = evaluar_factor();
    saltar_espacios();

    while (*cursor == '*' || *cursor == '/') {
        char op = *cursor;
        cursor++; // Avanzar tras el '*' o '/'
        if (op == '*') {
            resultado *= evaluar_factor();
        } else {
            double divisor = evaluar_factor();
            if (divisor != 0) {
                resultado /= divisor;
            } else {
                print_biosstring("ERROR: Division por cero\r\n");
            }
        }
        saltar_espacios();
    }
    return resultado;
}

// 3. NIVEL: Números, Variables y Paréntesis (Prioridad alta)
double evaluar_factor() {
    saltar_espacios();
    double resultado = 0;

    // Manejo de paréntesis: ( expresión )
    if (*cursor == '(') {
        cursor++; // Saltar '('
        resultado = evaluar_expresion();
        saltar_espacios();
        if (*cursor == ')') cursor++; // Saltar ')'
        return resultado;
    }

    // Manejo de números
    if (isdigit(*cursor) || *cursor == '.') {
        char *endptr;
        resultado = strtod(cursor, &endptr);
        cursor = endptr; // strtod nos dice dónde termina el número
    } 
    // Manejo de variables
    else if (isalpha(*cursor)) {
        char nombre_var[16];
        int i = 0;
        while (isalnum(*cursor) || *cursor == '$') {
            if (i < 15) nombre_var[i++] = *cursor;
            cursor++;
        }
        nombre_var[i] = '\0';

        int idx = buscar_variable(nombre_var);
        if (idx != -1 && tabla_variables[idx].tipo == T_NUMBER) {
            resultado = tabla_variables[idx].valor_num;
        } else {
            resultado = 0; // Comportamiento BASIC: variable no definida es 0
        }
    }
    return resultado;
}

void ejecutar_print(char *p) {
    skip_spaces(&p);
    
    while (*p && *p != '\r' && *p != '\n') {
        // CASO: CADENA LITERAL "HOLA"
        if (*p == '"') {
            p++; 
            while (*p && *p != '"') bios_putchar(*p++);
            if (*p == '"') p++;
        }
        // CASO: VARIABLE (ANCHO, NOMBRE$)
        else if (isalpha(*p)) {
            char nombre_var[16];
            int i = 0;
            // Extraemos el nombre (letras, números o el signo $ para strings)
            while (isalnum(*p) || *p == '$') {
                if (i < 15) nombre_var[i++] = *p;
                p++;
            }
            nombre_var[i] = '\0';

            int idx = buscar_variable(nombre_var);
            if (idx == -1) {
                // OPCIÓN A: Comportamiento BASIC clásico (imprimir cero o nada)
                if (nombre_var[strlen(nombre_var)-1] == '$') print_biosstring("");
                else bios_print_str("0");
            } else {
                // OPCIÓN B: La variable existe, imprimimos su valor real
                if (tabla_variables[idx].tipo == T_NUMBER) {
                    char buf[32];
                    sprintf(buf, "%g", tabla_variables[idx].valor_num);
                    pritn_biosstring(buf);
                } else {
                    if (tabla_variables[idx].valor_str) print_biosstring(tabla_variables[idx].valor_str);
                }
            }
        }
        skip_spaces(&p);
        if (*p == ',' || *p == ';') {
            if (*p == ',') print_biosstring("    ");
            p++;
            skip_spaces(&p);
        } else break;
    }
    print_biosstring("\r\n");
}

void establecer_variable(const char *nombre, VarType tipo, double n, const char *s) {
    // 1. Buscamos si ya existe
    int idx = -1;

    idx = buscar_variable(nombre);

    // 2. Si no existe, buscamos un hueco libre para crearla
    if (idx == -1) {
        for (int i = 0; i < MAX_VARS; i++) {
            if (!tabla_variables[i].ocupada) {
                idx = i;
                strncpy(tabla_variables[idx].nombre, nombre, 15);
                tabla_variables[idx].nombre[15] = '\0';
                tabla_variables[idx].ocupada = true;
                tabla_variables[idx].valor_str = NULL; // Inicializamos el puntero
                break;
            }
        }
    }

    if (idx == -1) {
        print_biosstring("ERROR: No hay espacio para más variables\r\n");
        return;
    }

    // 3. Gestión de memoria de Cadenas (Crucial en micros)
    // Si antes era un string y ahora vamos a poner otra cosa, liberamos lo anterior
    if (tabla_variables[idx].tipo == T_STRING && tabla_variables[idx].valor_str != NULL) {
        free(tabla_variables[idx].valor_str);
        tabla_variables[idx].valor_str = NULL;
    }

    // 4. Asignación del nuevo valor y tipo
    tabla_variables[idx].tipo = tipo;
    if (tipo == T_NUMBER) {
        tabla_variables[idx].valor_num = n;
    } else {
        if (s != NULL) {
            tabla_variables[idx].valor_str = strdup(s); // Reserva memoria y copia
        } else {
            tabla_variables[idx].valor_str = strdup(""); 
        }
    }
}

void ejecutar_let(char *p) {
    char nombre_var[16];
    int i = 0;

    skip_spaces(&p);

    // 1. Extraer el nombre de la variable
    if (!isalpha(*p)) {
        bios_print_str("ERROR: El nombre de variable debe empezar por letra\r\n");
        return;
    }

    while (isalnum(*p) || *p == '$') {
        if (i < 15) nombre_var[i++] = *p;
        p++;
    }
    nombre_var[i] = '\0';

    // 2. Buscar el signo '='
    skip_spaces(&p);
    if (*p != '=') {
        bios_print_str("ERROR: Se esperaba '='\r\n");
        return;
    }
    p++; // Saltar el '='
    skip_spaces(&p);

    // 3. Determinar si el valor es Cadena o Número
    if (*p == '"') {
        // --- ASIGNACIÓN DE CADENA ---
        p++; // Saltar comilla inicial
        char buffer_temp[128];
        int j = 0;
        while (*p && *p != '"' && j < 127) {
            buffer_temp[j++] = *p++;
        }
        buffer_temp[j] = '\0';
        
        if (*p == '"') p++; // Saltar comilla final
        
        // Llamamos a tu función maestra
        establecer_variable(nombre_var, T_STRING, 0, buffer_temp);
    } 
    else {
        // --- ASIGNACIÓN DE NÚMERO ---
        // (En el futuro aquí llamarás a evaluar_expresion)
        char *endptr;
        double valor = strtod(p, &endptr);
        
        if (p == endptr) {
            bios_print_str("ERROR: Valor numérico no válido\r\n");
            return;
        }
        
        // Llamamos a tu función maestra
        establecer_variable(nombre_var, T_NUMBER, valor, NULL);
    }
}


void ejecutar_interprete() {
    char linea[BUFFER_SIZE];
    char *argv[MAX_ARGS];
    int argc;

    print_biosstring("\r\nSISTEMA INICIADO - BIOS EN RAM\r\n");

    while (1) {
            print_biosstring("\r\n> "); 
            gets_biosstring(linea, sizeof(linea)); // Tu función de lectura
    
            // 1. Limpieza básica: saltar espacios iniciales
            char *p = linea;
            while (isspace(*p)) p++;
            if (*p == '\0') continue; // Línea vacía
    
            // 2. ¿EMPIEZA POR NÚMERO? (Modo Programa)
            if (isdigit(*p)) {
                int num_linea = (int)strtol(p, &p, 10); // Lee el número y avanza el puntero 'p'
                
                if (num_linea >= 0 && num_linea < MAX_LINEAS) {
                    while (isspace(*p)) p++; // Saltar espacios tras el número
                    
                    if (*p == '\0') {
                        // Si el usuario pone "10" y nada más, borramos la línea
                        programa_memoria[num_linea][0] != '\0';
                    } else {
                        // Guardamos el resto de la cadena en la memoria
                        strncpy(programa_memoria[num_linea], p, MAX_CHARS_LINEA - 1);
                        programa_memoria[num_linea][MAX_CHARS_LINEA - 1] = '\0';
                    }
                } else {
                    print_biosstring("ERROR: Numero de linea fuera de rango\r\n");
                }
                continue; // Importante: volver al principio del bucle
            }
    
            // 3. MODO DIRECTO (Ejecución inmediata)
            // Usamos una copia o punteros para no destruir la línea original
            char *cmd_ptr = p;
            while (*p && !isspace(*p)) p++; // Buscamos el final del comando (el primer espacio)
            
            char comando_temporal[20];
            int len_cmd = p - cmd_ptr;
            if (len_cmd >= sizeof(comando_temporal)) len_cmd = sizeof(comando_temporal) - 1;
            strncpy(comando_temporal, cmd_ptr, len_cmd);
            comando_temporal[len_cmd] = '\0';
    
            // 'p' ahora apunta al inicio de los argumentos
            skip_spaces(&p); 
    
            int id_cmd = identificar_comando(comando_temporal);
    
            switch (id_cmd) {
                case PRINT:
                    // p apunta a todo lo que hay tras "PRINT" (ej: "A + 5")
                    ejecutar_print(p); 
                    break;
                case CLS:
                    bios_cls();
                    break;
    
                case RUN:
                    // Bucle que recorre programa_memoria y llama a ejecutar_linea
                    ejecutar_programa_completo();
                    break;
    
                case HELP:
                    print_biosstring("Comandos: PRINT, CLS, RUN, LIST, HELP\r\n");
                    break;
 				case LET::                                                        
 				      ejecutar_let(p);
 				      break;                                                        
                default:
                    print_biosstring("ERROR: Comando desconocido\r\n");
                    break;
            }
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
