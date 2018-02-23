BASEMACHINE ?= "mdm9607"
require conf/machine/include/basemachine.inc

SOC_FAMILY ?= "mdm9x07"
include conf/machine/include/soc-family.inc

# Uncomment the following line to enable the hard floating point abi. Note that
# this breaks some binary libraries and 3D (neither of which ship with
# meta-yocto). For maximum compatibility, leave this disabled.
#DEFAULTTUNE ?= "cortexa8hf-neon"

require conf/machine/include/tune-cortexa8.inc

BBMASK = ".*/*/recipes-kernel/linux-msm-4.4"
BBMASK .= "|.*/*/recipes-security/libselinux/"

MACHINE_DISPLAY_WIDTH_PIXELS ?= "480"
MACHINE_DISPLAY_HEIGHT_PIXELS ?= "640"

SERIAL_CONSOLE ?= "115200 ttyHSL0"

DISTRO ?= "mdm"

MACHINE_FEATURES += "kernel26 keyboard"

PAGE_SIZE ?= '4096'
PAGE_SIZE_2K ?= '2048'

# Max supported inodes
EXTRA_IMAGECMD ?= "-N 2048"

#UBINIZE Args
UBINIZE_ARGS ?= "-m 4096 -p 256KiB -s 4096"
UBINIZE_ARGS_2K ?= "-m 2048 -p 131072 -s 2048"
#MKUBIFS Arg
MKUBIFS_ARGS ?= "-m 4096 -e 253952 -c 2146 -F"
MKUBIFS_ARGS_2K ?= "-m 2048 -e 126976 -c 2146 -F"
#Volume sizes
SYSTEM_VOLUME_SIZE ?= "74MiB"
SYSTEM_VOLUME_SIZE_2K ?= "57MiB"
CACHE_VOLUME_SIZE ?= "37MiB"
CACHE_VOLUME_SIZE_2K ?= "1MiB"
