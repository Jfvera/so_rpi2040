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
    @ r0: carácter a enviar
    ldr r1, =0x40034000      @ Dirección base UART0
    
wait_tx:
    ldr r2, [r1, #0x18]      @ Lee el registro de estado (UARTFR)
    movs r3, #0x20           @ <--- SOLUCIÓN: Cargamos el bit 5 en r3
    tst r2, r3               @ <--- SOLUCIÓN: Comparamos registro contra registro
    bne wait_tx              @ Si el bit 5 (TXFF) está a 1, el buffer está lleno
    
    str r0, [r1, #0x00]      @ Escribe el carácter en UARTDR
    bx lr                    @ Regresa


.global bios_getchar
.thumb_func
bios_getchar:
    ldr r1, =0x40034000      @ UART0 base
wait_rx:
    ldr r2, [r1, #0x18]      @ UARTFR (Flag register)
    movs r3, #0x10           @ Bit 4: RXFE (FIFO vacía)
    tst r2, r3
    bne wait_rx              @ Si está vacía, espera
    ldr r0, [r1, #0x00]      @ Lee el carácter
    bx lr
