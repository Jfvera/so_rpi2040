#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "bios.h"

#define MAX_LINEAS 100
#define TAM_LINEA 64   
#define MAX_VARS 64

char programa_memoria[MAX_LINEAS][TAM_LINEA];
char *cursor; 

typedef enum { T_NUMBER, T_STRING } VarType;

typedef struct {
    char nombre[16];
    VarType tipo;      
    union {
        double valor_num;
        char *valor_str; 
    };
    bool ocupada;
} Variable;

Variable tabla_variables[MAX_VARS];

// --- PROTOTIPOS ---
void print_biosstring(const char *p);
void gets_biosstring(char buffer[], int size);
void saltar_espacios();
double evaluar_expresion();
double evaluar_termino();
double evaluar_factor();
void interpretar_linea(char *linea);
int buscar_variable(char *nombre);
void establecer_variable(const char *nombre, VarType tipo, double n, const char *s);

// --- BIOS Y AUXILIARES ---
void print_biosstring(const char *p) {
    while (*p) bios_putchar(*p++);
}

void gets_biosstring(char buffer[], int size) {
    int i = 0;
    while (i < size - 1) {
        char c = bios_getchar();
        if (c == '\r' || c == '\n') break;
        if (c == '\b' || c == 127) {
            if (i > 0) { i--; print_biosstring("\b \b"); }
            continue;
        }
        bios_putchar(c);
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    print_biosstring("\r\n");
}

void saltar_espacios() {
    // Usamos isspace pero también forzamos el avance si hay caracteres no imprimibles bajos
    while (*cursor && (isspace((unsigned char)*cursor) || (unsigned char)*cursor < 33)) cursor++; 
}

// --- EVALUADOR REFORZADO ---

double evaluar_factor() {
    saltar_espacios(); // <--- ASEGURAR LIMPIEZA ANTES DE EMPEZAR
    if (*cursor == '(') {
        cursor++;
        double r = evaluar_expresion();
        saltar_espacios();
        if (*cursor == ')') cursor++;
        return r;
    }
    if (isdigit(*cursor) || *cursor == '.') {
        char *final;
        double r = strtod(cursor, &final);
        cursor = final;
        saltar_espacios(); // <--- CLAVE: Saltar espacios DESPUÉS de leer el número
        return r;
    }
    if (isalpha(*cursor)) {
        char nombre[16]; int i = 0;
        while (isalnum(*cursor) || *cursor == '$') { 
            if (i < 15) nombre[i++] = *cursor; 
            cursor++; 
        }
        nombre[i] = '\0';
        saltar_espacios(); // <--- CLAVE: Saltar espacios DESPUÉS de la variable
        int idx = buscar_variable(nombre);
        if (idx != -1) {
            if (tabla_variables[idx].tipo == T_NUMBER) return tabla_variables[idx].valor_num;
            else { print_biosstring("Error: Type Mismatch\r\n"); return 0; }
        }
    }
    return 0;
}

double evaluar_termino() {
    double r = evaluar_factor();
    saltar_espacios();
    while (*cursor == '*' || *cursor == '/') {
        char op = *cursor++;
        saltar_espacios(); // <--- SALTAR ESPACIOS TRAS EL OPERADOR
        if (op == '*') r *= evaluar_factor();
        else {
            double d = evaluar_factor();
            if (d != 0) r /= d;
        }
        saltar_espacios();
    }
    return r;
}

double evaluar_expresion() {
    double r = evaluar_termino();
    saltar_espacios();
    while (*cursor == '+' || *cursor == '-') {
        char op = *cursor++;
        saltar_espacios(); // <--- SALTAR ESPACIOS TRAS EL OPERADOR
        if (op == '+') r += evaluar_termino();
        else r -= evaluar_termino();
        saltar_espacios();
    }
    return r;
}

// --- GESTIÓN DE VARIABLES ---

int buscar_variable(char *nombre) {
    for (int i = 0; i < MAX_VARS; i++) {
        if (tabla_variables[i].ocupada && strcmp(tabla_variables[i].nombre, nombre) == 0)
            return i;
    }
    return -1;
}

void establecer_variable(const char *nombre, VarType tipo, double n, const char *s) {
    int idx = buscar_variable((char*)nombre);
    if (idx == -1) {
        for (int i = 0; i < MAX_VARS; i++) {
            if (!tabla_variables[i].ocupada) {
                idx = i;
                strncpy(tabla_variables[idx].nombre, nombre, 15);
                tabla_variables[idx].ocupada = true;
                tabla_variables[idx].valor_str = NULL; 
                break;
            }
        }
    }
    if (idx == -1) return;

    tabla_variables[idx].tipo = tipo; 
    if (tipo == T_NUMBER) {
        tabla_variables[idx].valor_num = n;
    } else {
        if (tabla_variables[idx].valor_str) free(tabla_variables[idx].valor_str); // <--- LIBERAR MEMORIA
        tabla_variables[idx].valor_str = strdup(s ? s : ""); // <--- DUPLICAR CADENA
    }
}

// --- COMANDOS ---

void ejecutar_print() {
    saltar_espacios();
    if (*cursor == '\0') { print_biosstring("\r\n"); return; }

    while (*cursor && *cursor != '\r' && *cursor != '\n') {
        if (*cursor == '"') {
            cursor++;
            while (*cursor && *cursor != '"') bios_putchar(*cursor++);
            if (*cursor == '"') cursor++;
        } else {
            double val = evaluar_expresion();
            char buf[32]; sprintf(buf, "%g", val);
            print_biosstring(buf);
        }
        saltar_espacios(); // <--- LIMPIEZA TRAS EVALUAR
        if (*cursor == ',' || *cursor == ';') {
            if (*cursor == ',') print_biosstring("    ");
            cursor++;
            saltar_espacios();
        } else break;
    }
    print_biosstring("\r\n");
}

void ejecutar_let() {
    char var[16]; int i = 0;
    saltar_espacios();
    while (isalnum(*cursor) || *cursor == '$') { if (i < 15) var[i++] = *cursor; cursor++; }
    var[i] = '\0';
    saltar_espacios();
    if (*cursor == '=') cursor++;
    saltar_espacios();

    if (*cursor == '"') { 
        cursor++;
        char tmp[TAM_LINEA]; int j = 0;
        while (*cursor && *cursor != '"' && j < TAM_LINEA-1) tmp[j++] = *cursor++;
        tmp[j] = '\0';
        if (*cursor == '"') cursor++;
        establecer_variable(var, T_STRING, 0, tmp); 
    } else { 
        double val = evaluar_expresion();
        establecer_variable(var, T_NUMBER, val, NULL); 
    }
}

// --- NÚCLEO ---

void interpretar_linea(char *linea) {
    cursor = linea;
    saltar_espacios();
    if (*cursor == '\0') return;

    // Modo Guardado
    if (isdigit(*cursor)) { 
        int n_lin = atoi(cursor);
        while(isdigit(*cursor)) cursor++; 
        saltar_espacios();
        if (n_lin >= 0 && n_lin < MAX_LINEAS) {
            if (*cursor == '\0') programa_memoria[n_lin][0] = '\0';
            else strncpy(programa_memoria[n_lin], linea, TAM_LINEA - 1);
        }
        return; 
    }

    char cmd[16]; int i = 0;
    while (*cursor && !isspace(*cursor) && i < 15) cmd[i++] = *cursor++;
    cmd[i] = '\0';

    if (strcasecmp(cmd, "PRINT") == 0) ejecutar_print();
    else if (strcasecmp(cmd, "LET") == 0) ejecutar_let();
    else if (strcasecmp(cmd, "LIST") == 0) {
        for (int j = 0; j < MAX_LINEAS; j++) 
            if (programa_memoria[j][0]) { print_biosstring(programa_memoria[j]); print_biosstring("\r\n"); }
    }
    else if (strcasecmp(cmd, "RUN") == 0) {
        for (int j = 0; j < MAX_LINEAS; j++) {
            if (programa_memoria[j][0]) {
                char temp[TAM_LINEA]; strcpy(temp, programa_memoria[j]);
                char *p = temp; while(isdigit(*p)) p++;
                interpretar_linea(p);
            }
        }
    }
    else if (strcasecmp(cmd, "NEW") == 0) {
        for(int k=0; k<MAX_VARS; k++) if(tabla_variables[k].ocupada && tabla_variables[k].tipo == T_STRING) free(tabla_variables[k].valor_str);
        memset(tabla_variables, 0, sizeof(tabla_variables));
        memset(programa_memoria, 0, sizeof(programa_memoria));
    }
    else if (strcasecmp(cmd, "CLS") == 0) bios_cls();
    else if (cmd[0] != '\0') print_biosstring("Syntax Error\r\n");
}

void ejecutar_interprete() {
    char buffer[TAM_LINEA]; 
    memset(programa_memoria, 0, sizeof(programa_memoria));
    memset(tabla_variables, 0, sizeof(tabla_variables));
    
    print_biosstring("PICO-BASIC V1.6 READY\r\n");
    while (1) {
        print_biosstring("> ");
        gets_biosstring(buffer, TAM_LINEA);
        interpretar_linea(buffer);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    ejecutar_interprete();
    return 0;
}
