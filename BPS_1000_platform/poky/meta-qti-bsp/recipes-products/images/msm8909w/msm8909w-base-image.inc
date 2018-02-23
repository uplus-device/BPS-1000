# Rootfs creation.
require ../../../recipes-ss-mgr/packagegroup/ss-mgr.inc
DEPENDS_append_aarch64 += "lib32-lk"
DEPENDS_append_arm += "lk"
# The OE-Core minimal boot image files.
#IMAGE_FSTYPES = "ext4"

IMAGE_INSTALL += "packagegroup-core-boot"
IMAGE_INSTALL += "${ROOTFS_PKGMANAGE_BOOTSTRAP}"
IMAGE_INSTALL += "${CORE_IMAGE_EXTRA_INSTALL}"

# The MSM Linux minimal boot image files.
IMAGE_INSTALL += "alsa-lib"
IMAGE_INSTALL += "alsa-utils"
IMAGE_INSTALL += "tinycompress"
IMAGE_INSTALL += "base-files"
IMAGE_INSTALL += "base-passwd"
IMAGE_INSTALL += "bluez5"
IMAGE_INSTALL += "dhcpcd"
IMAGE_INSTALL += "dnsmasq"
DEPENDS += "gdb-cross-arm"
IMAGE_INSTALL += "glib-2.0"
IMAGE_INSTALL += "init-audio"
IMAGE_INSTALL += "start-scripts-find-partitions"
IMAGE_INSTALL += "start-scripts-firmware-links"
IMAGE_INSTALL += "system-core-adbd"
IMAGE_INSTALL += "system-core-usb"
IMAGE_INSTALL += "system-conf"
IMAGE_INSTALL += "tcpdump"
IMAGE_INSTALL += "wireless-tools"
IMAGE_INSTALL += "wcnss"
IMAGE_INSTALL += "wlan-opensource"
IMAGE_INSTALL += "hostap-daemon-qcacld"
IMAGE_INSTALL += "wpa-supplicant-qcacld"

# Below is to generate sparse ext4 system image (OE by default supports raw ext4 images)

do_makesystem() {

    make_ext4fs -s -l ${SYSTEM_SIZE_EXT4} ${DEPLOY_DIR_IMAGE}/${MACHINE}-sysfs.ext4 ${IMAGE_ROOTFS}
}
