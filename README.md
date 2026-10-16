# ESP-Guitar-Pedal

A digital guitar pedal built around the **ESP32-S3 (N16R8)**, featuring a looper, gain control, and delay with tap-tempo — designed as a compact, 3D-printed stompbox with an OLED display and physical knobs for live control.

## Features

- **Looper** — record and loop back guitar input in real time, with at least 60 seconds of total loop time
- **Gain** — adjustable boost/overdrive stage
- **Delay** — with a tap-tempo footswitch for setting delay time on the fly
- **OLED display** (1.3" I2C) for status/mode feedback
- **Physical knobs** (potentiometers) to adjust parameters live, without reflashing firmware
- **Two latching 3PDT footswitches** for effect/loop control, plus a larger external momentary footswitch (cable-routed) for tap tempo

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-S3-N16R8 (16MB flash / 8MB PSRAM) |
| ADC | PCM1808 |
| DAC | PCM5102 |
| Input buffer op-amps | TL072 x2 |
| Voltage regulation | AMS1117 3.3V x2 |
| Pots | 100K x4 |
| Display | 1.3" I2C OLED |
| Footswitches | 2x latching 3PDT, 1x SPDT momentary (external) |
| Enclosure | 1590BB-sized (119.5 x 94 x 34mm), 3D-printed |

## Signal Chain

```
Guitar In → TL072 Input Buffer → PCM1808 ADC → ESP32-S3 (DSP) → PCM5102 DAC → Output Buffer → Amp Out
```

## Firmware

Built on the ESP32-S3, handling:
- Real-time audio I/O over I2S (ADC/DAC)
- Looper record/overdub/playback logic
- Delay effect with tap-tempo timing
- OLED UI for mode/status display
- Pot/footswitch input handling

## Hardware Design

- Perfboard prototype (8x12 cm) for initial bring-up
- KiCad schematic/PCB design (in progress)
- 3D-printed enclosure (CAD files included)

## Project Status

🚧 Work in progress — built as a technical club open-round application project.

- [x] Source all components
- [x] Guitar pass-through via ADC/DAC on ESP32-S3
- [ ] Looper firmware
- [ ] Delay + tap-tempo firmware
- [ ] Final perfboard wiring
- [ ] Enclosure build
- [ ] PCB design

## Repository Structure

```
/firmware     — ESP32-S3 source code
/hardware     — KiCad schematics, perfboard layout, wiring diagrams
/enclosure    — 3D-printable enclosure CAD files
/docs         — notes, photos, build log
```

## License

TBD
