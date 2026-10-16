# Llavero Companion

Un llavero con ojos que reaccionan. Un **ESP32-C3 SuperMini** y una **OLED de 0,96"** dibujan una carita con estados de ánimo (moods) que cambian según lo que le hagas: acariciarlo, molestarlo, sacudirlo, inclinarlo o ignorarlo.

> Estado: prototipo. El código está escrito pero todavía no se ha probado en hardware real (ver "Estado y pendientes").

## Moods

| Mood | Cómo se llega | Aspecto |
|------|---------------|---------|
| Normal | Estado base | Ojos curiosos que parpadean y miran alrededor |
| Feliz | Pulsación corta (caricia) | Ojos sonrientes y risa |
| Aburrido | 20 s sin interacción | Ojos planos y entrecerrados, parpadeos lentos |
| Dormido | 45 s sin interacción | Ojos cerrados y "zzz" animados |
| Sorprendido | Lo despiertas estando dormido | Ojos grandes y sudor |
| Enfadado | Pulsación larga (lo molestas) | Ceño y temblor |
| Mareado | Sacudirlo | Ojos temblorosos |

Además, en modo normal los ojos **miran hacia donde inclinas** el llavero (requiere acelerómetro).

## Hardware

| Pieza | Notas |
|-------|-------|
| ESP32-C3 SuperMini | Placa principal |
| OLED SSD1306 128x64, I2C | Módulo GM009605 v4.3, dirección `0x3C` |
| Botón (opcional) | Pulsador normal, entre GPIO 4 y GND |
| MPU6050 (opcional) | Para sacudidas e inclinación, dirección `0x68` |

### Cableado

| OLED / MPU6050 | ESP32-C3 SuperMini |
|----------------|---------------------|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

OLED y MPU6050 comparten el mismo bus I2C (mismos pines). El botón va entre **GPIO 4** y **GND**; usa la resistencia pull-up interna.

Sin botón o sin acelerómetro el código funciona igual: simplemente no hay esas interacciones.

## Software

1. Instala [Arduino IDE](https://www.arduino.cc/en/software) y el soporte de placas **esp32** de Espressif (Gestor de placas).
2. Instala estas librerías desde el Gestor de bibliotecas:
   - `Adafruit SSD1306`
   - `Adafruit GFX Library`
   - `FluxGarage RoboEyes`
3. Selecciona la placa **ESP32C3 Dev Module**.
4. Activa **Tools → USB CDC On Boot → Enabled** (si no, el monitor serie no muestra nada).
5. Abre `llavero_moods/llavero_moods.ino`, compila y sube.

Si no aparece el puerto, mantén pulsado el botón BOOT, conecta el USB y suéltalo.

## Probar los moods sin botón

Abre el monitor serie a **115200 baudios** y escribe un número:

`1` normal · `2` feliz · `3` aburrido · `4` dormido · `5` sorprendido · `6` enfadado · `7` mareado

También puedes simularlo sin hardware en [Wokwi](https://wokwi.com) (ESP32-C3 + SSD1306, SDA→GPIO 8, SCL→GPIO 9).

## Ajustes

Todo está al principio del `.ino`:

- **Tiempos**: `T_BORED`, `T_SLEEP`, `T_HAPPY`, etc. (en milisegundos). Bájalos para probar más rápido.
- **Sacudida**: `SHAKE_THRESHOLD` (baja el valor si cuesta activarlo), `SHAKE_HITS_NEEDED`.
- **Inclinación**: `TILT_ON` / `TILT_OFF`. Si los ojos miran al lado contrario o con los ejes cambiados, usa `TILT_X_SIGN`, `TILT_Y_SIGN` (`-1` para invertir) y `TILT_SWAP_XY`. Depende de cómo montes el MPU6050.
- **Añadir un mood**: agrega un valor al `enum Mood`, un `case` en `setMood()` y, si quieres transiciones, otro en `updateMoods()`.

## Si la pantalla no responde

- Comprueba que los pines están **soldados** (sin soldar, el contacto es poco fiable).
- Ejecuta un escáner I2C: debería aparecer `0x3C`.
- Revisa VCC a 3V3, GND, y prueba a intercambiar SDA y SCL.
- Prueba la dirección `0x3D` si `0x3C` no funciona.

## Estado y pendientes

- [ ] Verificar compilación y funcionamiento en hardware real (el código se escribió sin poder compilarlo, es posible que haya que corregir nombres de la API de RoboEyes).
- [ ] Ajustar los signos de los ejes del acelerómetro según el montaje.
- [ ] Batería LiPo con módulo de carga protegido, interruptor de encendido y modo de bajo consumo.
- [ ] Carcasa impresa en 3D.

## Licencia

MIT, ver [LICENSE](LICENSE).
