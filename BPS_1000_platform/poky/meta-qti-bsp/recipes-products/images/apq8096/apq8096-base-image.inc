# Rootfs creation.
require ../../../recipes-ss-mgr/packagegroup/ss-mgr.inc
DEPENDS_append += "lib32-lk"
# The OE-Core minimal boot image files.
IMAGE_FSTYPES = "ext4"

IMAGE_INSTALL += "packagegroup-core-boot"
IMAGE_INSTALL += "${ROOTFS_PKGMANAGE_BOOTSTRAP}"
IMAGE_INSTALL += "${CORE_IMAGE_EXTRA_INSTALL}"

# utils and scripts needed for SELinux
IMAGE_INSTALL += "packagegroup-core-selinux"
IMAGE_INSTALL += "selinux-config"
IMAGE_INSTALL += "start-scripts-firmware-links"
IMAGE_INSTALL += "refpolicy-mls"
IMAGE_INSTALL += "start-scripts-find-partitions"
IMAGE_INSTALL += "start-scripts-misc-daemon"

# The MSM Linux minimal boot image files.
IMAGE_INSTALL += "alsa-lib"
IMAGE_INSTALL += "alsa-utils"

IMAGE_INSTALL += "loc-stub"
IMAGE_INSTALL += "loc-pla"
IMAGE_INSTALL += "loc-hal"
IMAGE_INSTALL += "loc-net-iface"

IMAGE_INSTALL += "base-files"
IMAGE_INSTALL += "base-passwd"
IMAGE_INSTALL += "dhcpcd"
IMAGE_INSTALL += "dnsmasq"

IMAGE_INSTALL += "e2fsprogs-e2fsck"
IMAGE_INSTALL += "e2fsprogs-resize2fs"

IMAGE_INSTALL += "ebtables"
IMAGE_INSTALL += "ethtool"
DEPENDS += "gdb-cross-aarch64"

IMAGE_INSTALL += "glib-2.0"
IMAGE_INSTALL += "iproute2"
IMAGE_INSTALL += "iptables"
#IMAGE_INSTALL += "iw"
#IMAGE_INSTALL += "jquery"
#IMAGE_INSTALL += "jquery-ui"
#IMAGE_INSTALL += "kernel-modules"
#IMAGE_INSTALL += "libcomerr"
#IMAGE_INSTALL += "libss"
#IMAGE_INSTALL += "libe2p"
#IMAGE_INSTALL += "libext2fs"
#IMAGE_INSTALL += "libnl"
#IMAGE_INSTALL += "libavahi-common"
#IMAGE_INSTALL += "libavahi-core"
#IMAGE_INSTALL += "libavahi-client"
#IMAGE_INSTALL += "libavahi-glib"
#IMAGE_INSTALL += "libstdc++"
#IMAGE_INSTALL += "libxml2"
#IMAGE_INSTALL += "lighttpd"
#IMAGE_INSTALL += "loc-api"
#IMAGE_INSTALL += "minidlna"
#IMAGE_INSTALL += "mtd-utils-ubifs"
#IMAGE_INSTALL += "openssl"
#IMAGE_INSTALL += "powerapp"
#IMAGE_INSTALL += "powerapp-powerconfig"
#IMAGE_INSTALL += "powerapp-reboot"
#IMAGE_INSTALL += "powerapp-shutdown"
#IMAGE_INSTALL += "ppp"
IMAGE_INSTALL += "qcacld-ll"

IMAGE_INSTALL += "start-scripts-find-partitions"
IMAGE_INSTALL += "start-scripts-firmware-links"
IMAGE_INSTALL += "system-core-adbd"
IMAGE_INSTALL += "system-core-logd"
IMAGE_INSTALL += "system-core-leprop"
IMAGE_INSTALL += "system-core-usb"
IMAGE_INSTALL += "system-core-debuggerd"

IMAGE_INSTALL += "liblog"
IMAGE_INSTALL += "libcutils"
IMAGE_INSTALL += "system-conf"
IMAGE_INSTALL += "system-prop"
IMAGE_INSTALL += "tcpdump"
IMAGE_INSTALL += "wireless-tools"

IMAGE_INSTALL += "hostap-daemon-qcacld"
IMAGE_INSTALL += "wpa-supplicant-qcacld"
#IMAGE_PREPROCESS_COMMAND += "echo ${MACHINE} >> ${IMAGE_ROOTFS}/target;"

# 32-bit packages
IMAGE_INSTALL += "lib32-live555-qti"
IMAGE_INSTALL += "lib32-qmmf-support"
IMAGE_INSTALL += "lib32-qmmf-webserver"
IMAGE_INSTALL += "lib32-media"
IMAGE_INSTALL += "lib32-camera"
IMAGE_INSTALL += "lib32-tinyalsa"
IMAGE_INSTALL += "lib32-audiohal"
IMAGE_INSTALL += "lib32-encoders"
IMAGE_INSTALL += "lib32-gtest"
IMAGE_INSTALL += "lib32-qmmf-sdk"

# Below is to generate sparse ext4 system image (OE by default supports raw ext4 images)
SELINUX_FILE_CONTEXTS = "${IMAGE_ROOTFS}/etc/selinux/mls/contexts/files/file_contexts"
do_makesystem() {

    make_ext4fs -s -a / -S ${SELINUX_FILE_CONTEXTS} -l ${SYSTEM_SIZE_EXT4} ${DEPLOY_DIR_IMAGE}/${MACHINE}-sysfs.ext4 ${IMAGE_ROOTFS}
}
