# Packages installed to the root file system of PSM build
require mdm9607-base-qti-image.inc

#Excluding audio packages
PACKAGE_EXCLUDE += "acdbloader"
PACKAGE_EXCLUDE += "audio-ftm"
PACKAGE_EXCLUDE += "acdbmapper"
PACKAGE_EXCLUDE += "acdbrtac"
PACKAGE_EXCLUDE += "adiertac"
PACKAGE_EXCLUDE += "audcal"
PACKAGE_EXCLUDE += "audcaltests"
PACKAGE_EXCLUDE += "audioalsa"
PACKAGE_EXCLUDE += "soundtrigger"
PACKAGE_EXCLUDE += "csd-server"
PACKAGE_EXCLUDE += "ftm"

#Excluding mcm packages
PACKAGE_EXCLUDE += "loc-mcm-test-shim"
PACKAGE_EXCLUDE += "loc-mcm-qmi-test-shim"
PACKAGE_EXCLUDE += "loc-mcm-type-conv"
PACKAGE_EXCLUDE += "mcm-core"
PACKAGE_EXCLUDE += "mcmlocserver"

#Excluding Wlan packages
PACKAGE_EXCLUDE += "wlan-services"
PACKAGE_EXCLUDE += "qcacld-utils"

#Excluding Flouride BT Packages
PACKAGE_EXCLUDE += "hci-qcomm-init"
PACKAGE_EXCLUDE += "qc-bt-daemon"

#Excluding GPS Packges
PACKAGE_EXCLUDE += "loc-launcher"
PACKAGE_EXCLUDE += "dr-ap-service"
PACKAGE_EXCLUDE += "slim-daemon"
PACKAGE_EXCLUDE += "izat-client-api"
PACKAGE_EXCLUDE += "lowi-server"
PACKAGE_EXCLUDE += "lowi-test"
PACKAGE_EXCLUDE += "xtwifi-client"
PACKAGE_EXCLUDE += "xtwifi-inet-agent"
PACKAGE_EXCLUDE += "gtp-ap-le-os-agent"
PACKAGE_EXCLUDE += "nmea-test-app"

#Excluding Synergy BT packages
PACKAGE_EXCLUDE += "alsa-lib"
PACKAGE_EXCLUDE += "synergy"

PACKAGE_EXCLUDE += "canwrapper"
PACKAGE_EXCLUDE += "k61-firmware"
