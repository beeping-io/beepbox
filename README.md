# BeepBox

BeepBox es la herramienta CLI de Beeping para generar o incrustar señales acústicas de marcaje (beeps) en archivos WAV. Utiliza **Beeping Core** para sintetizar las huellas y puede trabajar tanto en modo audible como no audible, ya sea creando un archivo nuevo o mezclando las marcas dentro de audio existente.

## Dependencias
- `libsndfile` (debe estar instalada en el sistema; se detecta vía `pkg-config`).
- `libebur128` (se descarga y compila automáticamente con CMake).
- `Beeping Core` (se obtiene vía `FetchContent` durante la configuración de CMake).

## Compilación con CMake
1. Instala `libsndfile` con tu gestor de paquetes (ej.: `brew install libsndfile` en macOS, `apt install libsndfile1-dev` en Debian/Ubuntu).
2. Configura el proyecto (descarga libebur128 y Beeping Core):
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
3. Compila:
   ```bash
   cmake --build build
   ```
4. (Opcional) Instala los binarios en `./bin` y librerías en `./lib` dentro del repo:
   ```bash
   cmake --install build
   ```

## Ejecución
El binario se llama `BeepBox` y queda en `build/BeepBox` (o en `bin/BeepBox` si usaste `cmake --install`). Ejemplo de invocación mínima:
```bash
./build/BeepBox -k 0abcd -o salida.wav
```

## Parámetro `key`
- Es obligatorio y siempre de **5 caracteres**.
- Se codifica en **base32** usando el alfabeto `0-9` y `a-v`.
- Se inserta en las marcas acústicas junto con un sello temporal para permitir la verificación posterior.

## Principales opciones del CLI
- `-m, --mode` (int): modo de generación (`0` audible, `1` hidden, `2` no audible, `3` custom).
- `-f, --file` (string): WAV de entrada para mezclar (44.1 kHz o 48 kHz, PCM 16 bits). Si se omite, se crean beeps desde cero.
- `-o, --output` (string): WAV de salida a escribir.
- `-d, --duration` (float): duración del WAV generado, en segundos (mínimo `start` + 0.1).
- `-i, --interval` (float): intervalo en segundos entre marcas sucesivas (>=2.5 por defecto).
- `-s, --start` (float): tiempo inicial de la primera marca, en segundos (>~5 por defecto).
- `-k, --key` (string): clave base32 de 5 caracteres.
- `-x, --mixmode` (int): estrategia de mezcla (`0` DefaultLevel, `1` GlobalLevel, `2` DynamicLevel).
- `-v, --volumebeeps` / `-p, --volumeprogram` (float): nivel de beeps y del programa en dB.
- `-r, --samplerate` (float): frecuencia de muestreo del WAV de salida.
- `-l, --loudnessstatistics` (int): `1` para imprimir LUFS/True Peak del resultado.
- `-bf, --basefreq` y `-ts, --tonesseparation` (float/int): definen el banco de tonos en modo `custom` (`-m 3`).
- `-sm, --synthmode` y `-sv, --synthvolume` (int/float): activan y nivelan la síntesis adicional mezclada con los beeps.

## Ejemplos
1) **Generar solo beeps en un WAV nuevo (no audible por defecto):**
```bash
./build/BeepBox \
  -k 0abc1 \
  -o beeps_solo.wav \
  -d 30 -i 10 -s 6 \
  -m 2
```

2) **Mezclar beeps dentro de un WAV existente:**
```bash
./build/BeepBox \
  -k 9v9v9 \
  -f entrada.wav \
  -o salida_con_beeps.wav \
  -m 2 -i 8 -s 5 \
  -x 1 -v -6 -p 0
```

3) **Generar beeps con frecuencia base personalizada (modo custom):**
```bash
./build/BeepBox \
  -k a1b2c \
  -o beeps_custom.wav \
  -m 3 -bf 15000 -ts 10 \
  -d 20 -i 7 -s 6
```
En modo `custom` la frecuencia base (`-bf`) y la separación de tonos (`-ts`) ajustan el espectro de las marcas para adaptarlo a distintos contenidos.
