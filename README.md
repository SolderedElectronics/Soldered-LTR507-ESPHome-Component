# Soldered LTR-507 ESPHome Component

| ![Digital Light & Proximity Sensor LTR-507 Breakout](https://soldered.com/cdn/shop/files/333063_featured-photo_fda38e.jpg?v=1785145837&width=3840) |
| :----------------------------------------------------------------------------------------------------------------------------------------------: |
|                                   [Digital Light & Proximity Sensor LTR-507 Breakout](https://www.solde.red/333063)                                   |

The LTR-507ALS-01 combines a digital ambient light sensor (ALS, 16-bit lux reading computed on-chip) and an
infrared-reflection proximity sensor (PS) in one I2C breakout. The board is part of the
[easyC / Qwiic ecosystem](https://soldered.com/collections/qwiic-ecosystem), so it connects with a single cable.

External ESPHome component for the Soldered Digital Light & Proximity Sensor LTR-507 breakout. It is a port of the
[Soldered LTR-507 Arduino library](https://github.com/SolderedElectronics/Soldered-Digital-Light-Sensor-Arduino-Library)
(register logic follows the [Soldered LTR507 ESP-IDF component](https://github.com/SolderedElectronics/Soldered-LTR507-ESP-IDF-Component))
and publishes ambient light (lux) and proximity (raw counts) as ESPHome [sensors](https://esphome.io/components/sensor/).

> **Proximity needs an external IR LED.** The LTR-507ALS-01 has no IR emitter in its package, so the proximity sensor
> reads `0` unless an IR LED (e.g. [this one](https://www.solde.red/101922)) is connected: LED cathode (-) to the
> breakout's **VLED** pin, LED anode (+) to **VCC**.

## Repository Contents

- **components/** - the ESPHome external component (Python config + C++ implementation)
- **examples/** - example YAML configs showing how to use the component

## Usage

Reference this repo directly from your own ESPHome YAML (no need to clone it locally):

```yaml
external_components:
  - source: github://SolderedElectronics/Soldered-LTR507-ESPHome-Component
    components: [soldered_ltr507]

i2c:
  sda: GPIO21
  scl: GPIO22

sensor:
  - platform: soldered_ltr507
    update_interval: 5s
    ambient_light:
      name: "Ambient Light"
    proximity:
      name: "Proximity"
```

On boot the component checks the chip's PART_ID / MANUFAC_ID registers, writes the configuration below and puts the
ALS and/or PS block into active mode (only the blocks whose sensor is configured, so the IR LED is not pulsed when
`proximity` is left out). The chip then measures continuously on its own; every `update_interval` the component reads
the latest results. Ambient light is published only when the chip reports a fresh measurement. A saturated proximity
ADC (object very close, or strong ambient IR such as direct sunlight) is logged as a warning.

See [`examples/basic.yaml`](examples/basic.yaml) for a full working example.

### Configuration variables

- **ambient_light** (*Optional*): ambient light level in lux. All options from
  [Sensor](https://esphome.io/components/sensor/#config-sensor).
- **proximity** (*Optional*): raw 11-bit proximity reading, `0` - `2047`, higher means closer. **Requires an
  external IR LED** (see the note at the top), otherwise it always reads `0`. All options from
  [Sensor](https://esphome.io/components/sensor/#config-sensor).
- **gain** (*Optional*): ALS dynamic range. `1X` (default, 1 lux/count, up to 64k lux) or `2X` (0.5 lux/count, up to
  32k lux).
- **integration_time** (*Optional*): ALS integration time / ADC resolution. One of `75ms` (default, 16-bit), `150ms`,
  `300ms`, `600ms`, `1200ms` (20-bit). The chip's internal repeat rate is 500 ms, raised to 1 s / 2 s for the two
  longest integration times, so `update_interval` faster than that just re-reads the same value.
- **led_current** (*Optional*): IR LED peak current. One of `5mA`, `10mA`, `20mA`, `50mA` (default), `100mA`. Make
  sure the LED you connect is rated for it.
- **led_pulse_frequency** (*Optional*): IR LED pulse frequency. One of `30kHz` - `100kHz` in 10 kHz steps, default
  `60kHz`.
- **led_pulses** (*Optional*, int): number of IR LED pulses per proximity measurement, `1` - `255`. Defaults to `127`.
- **address** (*Optional*, int): I2C address of the sensor. Defaults to `0x3A`.
- **update_interval** (*Optional*, [Time](https://esphome.io/guides/configuration-types#config-time)): how often to
  read the sensor. Defaults to `60s`.
- **i2c_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types#config-id)): I2C bus to use, if there is
  more than one.

At least one of `ambient_light` / `proximity` must be set.

### Hardware design

You can find hardware design for this board in the
[_Digital light & proximity sensor LTR-507ALS breakout_](https://github.com/SolderedElectronics/Digital-light---proximity-sensor-LTR-507ALS-breakout-hardware-design)
hardware repository.

### Documentation

Access library documentation [here](https://docs.soldered.com/).

### About Soldered

<img src="https://raw.githubusercontent.com/SolderedElectronics/Soldered-Generic-Arduino-Library/dev/extras/Soldered-logo-color.png" alt="soldered-logo" width="500"/>

At Soldered, we design and manufacture a wide selection of electronic products to help you turn your ideas into acts and bring you one step closer to your final project. Our products are intented for makers and crafted in-house by our experienced team in Osijek, Croatia. We believe that sharing is a crucial element for improvement and innovation, and we work hard to stay connected with all our makers regardless of their skill or experience level. Therefore, all our products are open-source. Finally, we always have your back. If you face any problem concerning either your shopping experience or your electronics project, our team will help you deal with it, offering efficient customer service and cost-free technical support anytime. Some of those might be useful for you:

- [Web Store](https://www.soldered.com/shop)
- [Tutorials & Projects](https://soldered.com/learn)
- [Documentation](https://docs.soldered.com)

### Open-source license

Soldered invests vast amounts of time into hardware & software for these products, which are all open-source. Please support future development by buying one of our products.

Check license details in the LICENSE file. Long story short, use these open-source files for any purpose you want to, as long as you apply the same open-source licence to it and disclose the original source. No warranty - all designs in this repository are distributed in the hope that they will be useful, but without any warranty. They are provided "AS IS", therefore without warranty of any kind, either expressed or implied. The entire quality and performance of what you do with the contents of this repository are your responsibility. In no event, Soldered (TAVU) will be liable for your damages, losses, including any general, special, incidental or consequential damage arising out of the use or inability to use the contents of this repository.

## Have fun!

And thank you from your fellow makers at Soldered Electronics.
