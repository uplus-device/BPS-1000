#packages installed to root file system of PSM build
require mdm9607-base-image.inc

# The MSM Linux minimal boot image files.
PACKAGE_EXCLUDE += "alsa-intf"

#Excluding Audio packages
PACKAGE_EXCLUDE += "audiohal"
PACKAGE_EXCLUDE += "avahi-systemd"
PACKAGE_EXCLUDE += "avahi-daemon"
PACKAGE_EXCLUDE += "avahi-dnsconfd"
PACKAGE_EXCLUDE += "avahi-autoipd"
PACKAGE_EXCLUDE += "avahi-utils"
PACKAGE_EXCLUDE += "libavahi-common"
PACKAGE_EXCLUDE += "libavahi-core"
PACKAGE_EXCLUDE += "libavahi-client"
PACKAGE_EXCLUDE += "libavahi-glib"

PACKAGE_EXCLUDE += "dnsmasq"
PACKAGE_EXCLUDE += "ddclient"
PACKAGE_EXCLUDE += "lighttpd"
PACKAGE_EXCLUDE += "minidlna"
PACKAGE_EXCLUDE += "miniupnpd"
PACKAGE_EXCLUDE += "openssl"

PACKAGE_EXCLUDE += "rtsp-alg"
PACKAGE_EXCLUDE += "system-conf"

PACKAGE_EXCLUDE += "qcacld-hl"
PACKAGE_EXCLUDE += "wireless-tools"
PACKAGE_EXCLUDE += "iputils"
PACKAGE_EXCLUDE += "wpa-supplicant-qcacld"
PACKAGE_EXCLUDE += "hostap-daemon-qcacld"

#Excluding Flouride BT packages
PACKAGE_EXCLUDE += "packagegroup-bt"
PACKAGE_EXCLUDE += "bt-app"
PACKAGE_EXCLUDE += "bt-property"
PACKAGE_EXCLUDE+= "fluoride"
PACKAGE_EXCLUDE += "libbt-vendor"

PACKAGE_EXCLUDE += "libsensors"
PACKAGE_EXCLUDE += "loc-net-iface"
PACKAGE_EXCLUDE += "start-scripts-init-can"
