@ bios.S - Funciones básicas de bajo nivel
.syntax unified
.cpu cortex-m0plus
.thumb

.section .time_critical."bios_putchar", "ax"
.align 2
.global bios_putchar
.type bios_putchar, %function
.thumb_func

bios_putchar:
    @ r0 llega desde C con el carácter (el 'habitante' del sobre)
    ldr r1, =0x40034000      @ Dirección base UART0 (la 'calle')
    
wait_tx:
    ldr r2, [r1, #0x18]      @ Lee el registro de estado (Flag Register)
    tst r2, #0x20            @ Comprueba si el buffer está lleno (bit 5)
    bne wait_tx              @ Si está lleno, espera
    
    str r0, [r1, #0x00]      @ Escribe el carácter en el registro de datos
    bx lr                    @ Regresa a C
