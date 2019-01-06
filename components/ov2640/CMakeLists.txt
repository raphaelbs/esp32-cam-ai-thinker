set(COMPONENT_SRCS
  camera/camera.c
  camera/sccb.c
  camera/sensor.c
  camera/twi.c
  camera/xclk.c
  sensors/ov2640.c
  conversions/yuv.c
  conversions/to_jpg.cpp
  conversions/to_bmp.c
  conversions/jpge.cpp
  conversions/esp_jpg_decode.c
  )

set(COMPONENT_ADD_INCLUDEDIRS
  camera/include
  conversions/include
  )

set(COMPONENT_PRIV_INCLUDEDIRS
  camera/private_include
  sensors/private_include
  conversions/private_include
  )

set(COMPONENT_REQUIRES driver)
set(COMPONENT_PRIV_REQUIRES freertos)

register_component()
