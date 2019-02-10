# Example google_storage

Connects with Google Cloud Storage using OAuth2.0, takes pictures every 20 seconds and upload it to the cloud.

## Instructions

In order to run this example, you need:
- A Firebase project or a Google Cloud Storage bucket
- An `OAuth 2.0 client ID` of type `other` in [google cloud credential manager](https://console.cloud.google.com/apis/credentials). 

[... todo]

Then you need to configure your WiFi SSID and Password and your GCP keys via `make menuconfig` or directly in [Kconfig.projbuild](./main/Kconfig.projbuild) file.

All camera pins are configured by default accordingly to [this A.I. Thinker document](../../assets/ESP32-CAM_Product_Specification.pdf) and you can check then inside [Kconfig.projbuild](./main/Kconfig.projbuild).

## Notes

Make sure to read [sdconfig.defaults](./sdconfig.defaults) file to get a grasp of required configurations to enable `PSRAM` and set it to `64MBit`.
