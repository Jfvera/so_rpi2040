#!/bin/bash

# Colorines para que se vea bien en la terminal
VERDE='\033[0;32m'
AZUL='\033[0;34m'
AMARILLO='\033[1;33m'
NC='\033[0m' # Sin color

echo -e "${AZUL}--- Iniciando subida a Git con Versionado ---${NC}"

# 1. Añadir cambios
git add .

# 2. Estado actual
git status -s

# 3. Pedir el mensaje del commit
echo -e "${VERDE}Escribe el mensaje para este cambio:${NC}"
read mensaje

if [ -z "$mensaje" ]; then
    mensaje="Actualización automática $(date +'%Y-%m-%d %H:%M')"
fi

# 4. Hacer el commit
git commit -m "$mensaje"

# 5. --- NUEVA PARTE: GESTIÓN DE VERSIONES (TAGS) ---
echo -e "${AMARILLO}¿Quieres marcar esta versión como un punto de restauración? (s/n):${NC}"
read responder

if [ "$responder" == "s" ]; then
    echo -e "${VERDE}Introduce el nombre de la versión (ej: v1.1, estable, antes-de-math):${NC}"
    read version
    if [ ! -z "$version" ]; then
        git tag -a "$version" -m "Versión guardada: $mensaje"
        echo -e "${VERDE}Etiqueta '$version' creada localmente.${NC}"
    fi
fi

# 6. Subir a la nube
echo -e "${AZUL}Subiendo código y etiquetas a GitHub...${NC}"
git push origin main
git push origin --tags  # Esto sube las "pegatinas" de versión a GitHub

echo -e "${VERDE}¡Hecho! Todo sincronizado y versionado.${NC}"

## Para ver las versiones guardadas:  git tag
## Para ver que hice en el pasado git log --oneline
## Para recuperar una vesión git checkout v1.0 donde v1.0 es el nombre de la versión.
