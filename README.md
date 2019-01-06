# A.I. Thinker ESP32-CAM

The goal of this repository is to centralize information and examples for the A.I. Thinker ESP32-CAM.

<img src="https://www.open-electronics.org/wp-content/uploads/2018/10/ESP32-CAM.jpg" width="auto" height="200" alt="A.I. Thinker ESP32-CAM" />

# Capabilities

This board and the included OV2640 can acomplish the following:

- External 64MBit PSRAM
- Embeed flash light
- Sd Card
  - mount using FatFS
- Ov2640
  - Native resolutions:
    - UXGA
    - SVGA
    - CIF
  - Gain control
  - Balance control
  - Windowing
    - Zoom and panning modes
    - Sub-sampling mode
      - Svga
      - Cif
  - Frame exposure mode
  - Output formatter
    - Scaling image output
  - Power down mode
  - Strobe (flash control)

# Examples

To run the examples, make sure to have `xtensa` and `esp-idf` configured in your machine. If you haven't, refer to [this tutorial](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html).

- [JPG via Http server](./examples/http_jpg)

# Contributions

The roadmap for this repository is to implement examples uppon the capabilities of the board.

Feel free to open issues, describe bugs, suggest new examples and implementations.

I'll be reviewing and accepting PR's.
