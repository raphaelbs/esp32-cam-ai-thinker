COMPONENT_ADD_INCLUDEDIRS := conversions/include camera/include
COMPONENT_PRIV_INCLUDEDIRS := conversions/private_include camera/private_include sensors/private_include
COMPONENT_SRCDIRS := conversions camera sensors
CXXFLAGS += -fno-rtti
