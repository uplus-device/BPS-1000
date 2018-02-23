FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}-${PV}:"
DEPENDS = "base-passwd"

SRC_URI_append += "file://fstab"
SRC_URI_append += "file://ro-fstab"
SRC_URI_append_apq8053 += "file://apq8053/ro-fstab"
SRC_URI_append_mdm9607 += "file://mdm9607/ro-fstab"

dirs755 += "/media/cf /media/net /media/ram \
            /media/union /media/realroot /media/hdd \
            /media/mmc1"

dirs755_append_apq8053 +="/persist /cache /dsp "
#TODO Enabling systemd we need to add /firmware in dirs_755 list.
dirs755_append_apq8009 += "/firmware /persist /cache"
dirs755_append_apq8017 += "/firmware /persist /cache /dsp"
do_install_append(){
    install -m 755 -o diag -g diag -d ${D}/media
    install -m 755 -o diag -g diag -d ${D}/mnt/sdcard
    if ${@base_contains('DISTRO_FEATURES','ro-rootfs','true','false',d)}; then
        # Override fstab for apq8053 and mdm9607 
        if [ ${BASEMACHINE} == "apq8053" || ${BASEMACHINE} == "mdm9607" ]; then
            install -m 0644 ${WORKDIR}/${BASEMACHINE}/ro-fstab ${D}${sysconfdir}/fstab
        else
            install -m 0644 ${WORKDIR}/ro-fstab ${D}${sysconfdir}/fstab
        fi
    else
        install -m 0644 ${WORKDIR}/fstab ${D}${sysconfdir}/fstab
    fi
    ln -s /mnt/sdcard ${D}/sdcard
    rmdir ${D}/tmp
    ln -s /var/tmp ${D}/tmp
    ln -s /var/run/resolv.conf ${D}/etc/resolv.conf
}
