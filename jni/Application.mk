APP_ABI :=  x86 x86_64 arm64-v8a armeabi-v7a
APP_CFLAGS +=-Wno-error=format-security 
APP_CFLAGS +=-Wno-pointer-arith
APP_CFLAGS +=-Wno-format
APP_CFLAGS +=-Wno-multichar
APP_CFLAGS +=-Wno-attributes
APP_CFLAGS +=-Wno-write-strings
APP_STL := c++_static
APP_CPPFLAGS := -frtti -fexceptions
APP_PLATFORM := android-19