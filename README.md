# ESP32 Soil Sensor Dashboard

A web-based dashboard running on an ESP32 that reads soil nutrient data from a **CWT-SOIL-NPKPHCTH-S** sensor via RS485 (Modbus RTU) and serves a live dashboard over WiFi Access Point.

## Features

- Reads **7 soil parameters**: Nitrogen (N), Phosphorus (P), Potassium (K), pH, EC, Moisture, Temperature
- Live dashboard accessible from any browser on the same WiFi
- **Calibration offsets** for each parameter — saved to flash (NVS), survive reboot
- Dark-themed responsive UI — works on mobile and desktop
- RS485 half-duplex control via MAX485 module

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 (38-pin DevKit) |
| Soil sensor | CWT-SOIL-NPKPHCTH-S (7-in-1) |
| RS485 module | MAX485 |
| Communication | RS485 / Modbus RTU @ 4800 baud |

### Wiring

| ESP32 Pin | MAX485 Pin | Notes |
|---|---|---|
| GPIO16 (RX2) | RO | Receive |
| GPIO17 (TX2) | DI | Transmit |
| GPIO4 | DE + RE | Driver/Receiver enable (tied together) |
| 3.3V / 5V | VCC | Power |
| GND | GND | Ground |

Connect the MAX485 A/B terminals to the sensor's RS485 A/B lines.

## Software

### Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- Libraries (auto-installed via `platformio.ini`):
  - `ModbusMaster` by 4-20ma

### Build & Flash

1. Clone this repository
2. Open the folder in VS Code with PlatformIO installed
3. Click **Upload** (→ button) or run:
   ```bash
   pio run --target upload
   ```

## Usage

1. Power on the ESP32
2. Connect your phone or laptop to the WiFi network:
   - **SSID**: `SoilSensor`
   - **Password**: `12345678`
3. Open your browser and go to `http://192.168.4.1`
4. The dashboard will show live sensor readings, updating every 2 seconds

### Calibration Offsets

At the bottom of the dashboard, enter offset values to correct sensor readings. Click **Apply Offsets** to save. Offsets are stored in flash and persist across reboots.

## Dashboard Preview

```
┌─────────────────────────────────────┐
│  🌿 Soil Sensor Dashboard           │
├───────┬───────┬───────┬─────────────┤
│  N    │  P    │  K    │  pH         │
│ 32.0  │ 18.5  │ 25.0  │ 6.80        │
│ mg/kg │ mg/kg │ mg/kg │             │
├───────┴───────┼───────┴─────────────┤
│  EC           │  Moisture  Temp     │
│  420 µS/cm    │  38.5%    26.3°C    │
└───────────────┴─────────────────────┘
```

## Pin Summary

```
ESP32
├── GPIO4  → MAX485 DE/RE
├── GPIO16 → MAX485 RO  (RX2)
└── GPIO17 → MAX485 DI  (TX2)
```

## License

MIT License — free to use, modify and distribute.

## Author

**Dr. Zainal Abidin Arsat**
Department of Agrotechnology, Universiti Malaysia Perlis (UniMAP)
