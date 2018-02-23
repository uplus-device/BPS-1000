# Rootfs creation.
require ../../../recipes-ss-mgr/packagegroup/ss-mgr.inc

DEPENDS_append += "lk"
# The OE-Core minimal boot image files.
IMAGE_FSTYPES = "ext4"

IMAGE_INSTALL += "packagegroup-core-boot"
IMAGE_INSTALL += "${ROOTFS_PKGMANAGE_BOOTSTRAP}"
IMAGE_INSTALL += "${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','selinux', 'packagegroup-core-selinux', '', d)}"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','selinux', 'selinux-config', '', d)}"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','selinux', 'refpolicy-mls', '', d)}"

IMAGE_INSTALL += "start-scripts-firmware-links"
IMAGE_INSTALL += "start-scripts-find-partitions"
IMAGE_INSTALL += "start-scripts-misc-daemon"

# The MSM Linux minimal boot image files.
IMAGE_INSTALL += "alsa-lib"
IMAGE_INSTALL += "alsa-utils"
IMAGE_INSTALL += "base-files"
IMAGE_INSTALL += "base-passwd"
IMAGE_INSTALL += "dhcpcd"
IMAGE_INSTALL += "dnsmasq"
IMAGE_INSTALL += "ebtables"
IMAGE_INSTALL += "ethtool"
IMAGE_INSTALL += "native-frameworks"

IMAGE_INSTALL += "glib-2.0"
IMAGE_INSTALL += "init-audio"
IMAGE_INSTALL += "iproute2"
IMAGE_INSTALL += "iptables"
IMAGE_INSTALL += "libnl"
IMAGE_INSTALL += "libcutils"
IMAGE_INSTALL += "liblog"
IMAGE_INSTALL += "libxml2"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','location-services', 'loc-stub', '', d)}"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','location-services', 'loc-pla', '', d)}"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','location-services', 'loc-hal', '', d)}"
IMAGE_INSTALL += "${@base_contains('DISTRO_FEATURES','location-services', 'loc-net-iface', '', d)}"
IMAGE_INSTALL += "system-core-adbd"
IMAGE_INSTALL += "system-core-logd"
IMAGE_INSTALL += "system-core-leprop"
IMAGE_INSTALL += "system-core-usb"
IMAGE_INSTALL += "system-core-debuggerd"
IMAGE_INSTALL += "system-core-post-boot"
IMAGE_INSTALL += "system-conf"
IMAGE_INSTALL += "system-prop"
IMAGE_INSTALL += "tcpdump"
IMAGE_INSTALL += "wireless-tools"
IMAGE_INSTALL += "wcnss"
IMAGE_INSTALL += "wlan-opensource"
IMAGE_INSTALL += "qcacld-utils"
IMAGE_INSTALL += "hostap-daemon-qcacld"
IMAGE_INSTALL += "wpa-supplicant-qcacld"
IMAGE_INSTALL += "libbt-vendor"
IMAGE_INSTALL += "fluoride"
IMAGE_INSTALL += "bt-property"
IMAGE_INSTALL += "bt-app"
IMAGE_INSTALL += "display-hal"

#32-bit packages
IMAGE_INSTALL += "live555-qti"
IMAGE_INSTALL += "qmmf-support"
IMAGE_INSTALL += "qmmf-webserver"
IMAGE_INSTALL += "media"
IMAGE_INSTALL += "camera"
IMAGE_INSTALL += "tinyalsa"
IMAGE_INSTALL += "audiohal"
IMAGE_INSTALL += "encoders"
IMAGE_INSTALL += "gtest"

IMAGE_INSTALL += "qmmf-sdk"
IMAGE_INSTALL += "cairo"

IMAGE_INSTALL += "libpagemap"
IMAGE_INSTALL += "procrank"

# Below is to generate sparse ext4 system image (OE by default supports raw ext4 images)
SELINUX_FILE_CONTEXTS = "${IMAGE_ROOTFS}/etc/selinux/mls/contexts/files/file_contexts"

IMAGE_EXT4_SELINUX_OPTIONS = "${@base_contains('DISTRO_FEATURES','selinux',' -a / -S ${SELINUX_FILE_CONTEXTS} ','',d)}"

do_makesystem() {
    make_ext4fs ${IMAGE_EXT4_EXTRA_OPTIONS} -s ${IMAGE_EXT4_SELINUX_OPTIONS} -l ${SYSTEM_SIZE_EXT4} ${DEPLOY_DIR_IMAGE}/${MACHINE}-sysfs.ext4 ${IMAGE_ROOTFS}
}
