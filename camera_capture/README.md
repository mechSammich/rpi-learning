# camera_capture

A minimal C program that captures a still image on a **Raspberry Pi Zero 2 W**
using the integrated camera interface (CSI connector) via `libcamera-still`.

---

## Requirements

| Requirement | Version |
|---|---|
| Raspberry Pi OS (Bookworm or Bullseye) | — |
| `libcamera-apps` | ≥ 0.0.5 |
| CMake | ≥ 3.16 |
| GCC / arm-linux-gnueabihf-gcc | ≥ 8 |

Install `libcamera-apps` on the Pi:

```bash
sudo apt update
sudo apt install libcamera-apps
```

---

## Building

### On the Raspberry Pi (native build)

```bash
cd camera_capture
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Cross-compilation (x86-64 host → armv7-hf target)

Install the cross-compiler toolchain on your host machine:

```bash
sudo apt install gcc-arm-linux-gnueabihf cmake
```

Then build with the bundled toolchain file:

```bash
cd camera_capture
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=toolchain-rpi.cmake
cmake --build build
```

Transfer the binary to the Pi:

```bash
scp build/camera_capture pi@<PI_IP>:~/
```

---

## Usage

```
Usage: camera_capture [OPTIONS]

Capture a still image using the Raspberry Pi camera interface.

Options:
  -o, --output <file>   Output file name          (default: capture.jpg)
  -w, --width  <px>     Image width  in pixels    (default: 1920)
  -H, --height <px>     Image height in pixels    (default: 1080)
  -t, --timeout <ms>    Camera warm-up time (ms)  (default: 2000)
      --help            Show this help and exit
```

### Examples

```bash
# Capture with defaults (1920×1080, saved as capture.jpg)
./camera_capture

# Specify output file name
./camera_capture -o my_photo.jpg

# Full sensor resolution for the Camera Module 2 (Sony IMX219)
./camera_capture --output full_res.jpg --width 3280 --height 2464

# Longer warm-up for difficult lighting
./camera_capture -o night.jpg -t 5000
```

---

## How it works

`camera_capture` is a thin C wrapper around `libcamera-still`, the command-line
still-capture utility shipped with `libcamera-apps`.  The program:

1. Parses the CLI arguments with `getopt_long`.
2. Validates all parameters (positive integers, file name length).
3. Builds and executes a `libcamera-still` command that saves a JPEG to the
   requested output path.

`libcamera-still` handles all low-level camera initialisation, ISP tuning, and
JPEG encoding, which keeps this program small and dependency-free at the C level.

---

## Supported cameras

Any camera supported by `libcamera` on Raspberry Pi OS works, including:

* Camera Module 1 (OmniVision OV5647)
* Camera Module 2 (Sony IMX219)
* Camera Module 3 (Sony IMX708)
* High Quality Camera (Sony IMX477)

---

## Enabling the camera

Make sure the camera interface is enabled.  On Raspberry Pi OS:

```bash
sudo raspi-config
# Interface Options → Camera → Enable
```

Or add `camera_auto_detect=1` to `/boot/config.txt` (Bullseye and later).
