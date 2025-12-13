# BeepBox

BeepBox es la herramienta de línea de comandos de Beeping para generar o incrustar señales acústicas de marcaje (beeps) en WAV. Se apoya en **Beeping Core** para sintetizar las huellas y permite trabajar en modos audibles, no audibles, ocultos o personalizados. Su objetivo es transportar datos identificadores a través de audio de forma robusta.

## Instalación / Build
- Dependencias: CMake, compilador C++, `libsndfile` (sistema), `libebur128` (FetchContent), Beeping Core (FetchContent).
- Pasos:
  ```bash
  cmake -S . -B build
  cmake --build build
  cmake --install build   # opcional, instala en ./bin y ./lib del repo
  ```
- Binario resultante: `build/BeepBox` (o `bin/BeepBox` tras instalar).

## Uso básico
Comando mínimo (genera 1 beep por defecto):
```bash
./build/BeepBox -k 00001 -o output.wav
```
Defaults relevantes: `duration=2.3s`, `interval=2.3s`, `start=0`, modo `non-audible` (2), samplerate 44100 Hz. Si solo cabe un beep, se coloca al inicio y se ignora `interval`.

## Parámetros del CLI
- `-k, --key` (string, requerido): Clave base32 de 5 chars `[0-9a-v]` que se incrusta en los beeps.
- `-o, --output` (string, requerido): Ruta del WAV de salida.
- `-d, --duration` (float): Duración en segundos. Mínimo `2.3` y `start + 2.3 <= duration`. Default `2.3`.
- `-i, --interval` (float): Intervalo entre beeps en segundos. Mínimo `2.3`. Default `2.3`. Si solo cabe 1 beep, se ignora con warning.
- `-s, --start` (float): Momento del primer beep en segundos (>=0). Default `0`.
- `-m, --mode` (int): 0 audible | 1 hidden | 2 non-audible (default) | 3 custom.
- `--bf, --basefreq` (float): Frecuencia base en Hz para modo custom. Default `12000`. Requiere `mode=3`. Debe ser >0.
- `--ts, --tonesseparation` (int): Separación de tonos (>=1) en modo custom. Default `1`.
- `-f, --file` (string): WAV de entrada (44.1k/48k, PCM16) para mezclar. Obligatorio para usar `mixmode`/`volumeprogram`.
- `-x, --mixmode` (int): 0 DefaultLevel | 1 GlobalLevel | 2 DynamicLevel. Requiere `--file`; de lo contrario, error.
- `-v, --volumebeeps` (float): Nivel de beeps en dB. Clamping [-60, 12]. Default `-3`.
- `-p, --volumeprogram` (float): Nivel del programa en dB. Sólo aplica con `--file`; si no, se ignora. Default `0`.
- `-r, --samplerate` (float): 44100 o 48000 para generación sin `--file`. Si hay `--file`, se usa el SR del WAV. Default `44100`.
- `-l, --loudnessstatistics` (int): 0 desactivado | 1 activado. Default `0`.
- `-sm, --synthmode` (int): 0 desactivado | 1 r2d2. Clamping a [0,1]. Default `0`.
- `-sv, --synthvolume` (float): Volumen del synth en dB relativo a beeps. Clamping [-60, 12]. Default `0`.
- `-n, --dry-run` (flag): Valida y muestra el plan (beeps, timestamps) sin generar audio ni escribir archivos.
- `-h, --help` (flag): Muestra ayuda y ejemplos.

## Reglas conceptuales
- Un beep ocupa al menos `2.3s`.
- El primer beep empieza en `--start` (default 0). Debe cumplirse `start + 2.3 <= duration`.
- `--interval` marca la separación entre beeps; debe ser >=2.3. Si solo cabe 1 beep, se avisa y se ignora el intervalo.
- Para mezclar (`--mixmode`, `--volumeprogram`) es obligatorio `--file`; sin archivo se produce error al usar `--mixmode` y `--volumeprogram` se ignora.
- `--samplerate` solo se usa al generar desde cero; con `--file` se usa el SR del WAV de entrada.
- Valores fuera de rango se clamp/warning (volúmenes, synthmode).
- Errores bloquean la ejecución; warnings ajustan o ignoran parámetros pero continúan.

## Ejemplos
- Beep único por defecto:
  ```bash
  BeepBox -k 00001 -o beep.wav
  ```
- Múltiples beeps en audio generado:
  ```bash
  BeepBox -k 00001 -o long.wav -d 15 -i 3 -s 0
  ```
- Mezclar con WAV de entrada:
  ```bash
  BeepBox -k 00001 -f input.wav -o mixed.wav -m 2 -i 4 -x 1 -v -6 -p 0
  ```
- Modo custom (freq y separación):
  ```bash
  BeepBox -k 00001 -o custom.wav -m 3 -bf 15000 -ts 10 -d 10 -i 3
  ```
- Dry-run (plan sin generar archivo):
  ```bash
  BeepBox -n -k 00001 -o plan.wav -d 10 -i 2.5
  ```
