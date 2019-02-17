# Example change_detection

Detect changes by taking continuously QQVGA/grayscale pictures and averaging pixels. Takes about 1.5ms to take and compare this small pictures.

After change is detected, camera mode switches to operate at UXGA/JPEG. Then takes a single picture, adds it to the queue and returns to detection mode. The queue is processed every 5 seconds.

Bellow is an average time to detect and take pictures at given resolution:

| Resolution | Time |
| --- | --- |
| UXGA | ≈1.5s |
| SVGA | ≈1s |
| VGA | ≈1s |
| QVGA | ≈.86s |

**Note:** Consider ±50ms

## Instructions

You can adjust the sensitivity by tweeking the config MAX_ACCEPTED_AVG_DIFF, 0 being the most sensitive and 255 the maximum.

Also you can change the ammount of pictures that goes into the queue by change config PICTURE_QUEUE_SIZE.

All camera pins are configured by default accordingly to [this A.I. Thinker document](../../assets/ESP32-CAM_Product_Specification.pdf) and you can check then inside [Kconfig.projbuild](./main/Kconfig.projbuild).
