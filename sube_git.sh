#!/bin/bash

# Colorines para que se vea bien en la terminal
VERDE='\033[0;32m'
AZUL='\033[0;34m'
NC='\033[0m' # Sin color

echo -e "${AZUL}--- Iniciando subida a Git ---${NC}"

# 1. Añadir todos los cambios (respetando el .gitignore)
git add .

# 2. Mostrar el estado actual
git status -s

# 3. Pedir el mensaje del commit
echo -e "${VERDE}Escribe el mensaje para este cambio (ej: 'Añadido sensor ADC'):${NC}"
read mensaje

# Si el mensaje está vacío, poner uno por defecto
if [ -z "$mensaje" ]; then
    mensaje="Actualización automática $(date +'%Y-%m-%d %H:%M')"
fi

# 4. Hacer el commit
git commit -m "$mensaje"

# 5. Subir a la nube (ajusta 'main' si tu rama se llama 'master')
echo -e "${AZUL}Subiendo a la nube...${NC}"
git push origin master

echo -e "${VERDE}¡Hecho! Código actualizado en el repositorio.${NC}"
