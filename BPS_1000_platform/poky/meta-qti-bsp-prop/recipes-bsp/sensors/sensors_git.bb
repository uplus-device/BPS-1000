inherit qcommon update-rc.d qprebuilt

DESCRIPTION = "Sensors Library"
LICENSE = "Qualcomm-Technologies-Inc.-Proprietary"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-qti-bsp-prop/files/qcom-licenses/\
Qualcomm-Technologies-Inc.-Proprietary;md5=92b1d0ceea78229551577d4284669bb8"

PR = "r0"

DEPENDS = "liblog libcutils system-core common glib-2.0 qmi-framework diag libhardware"

FILESPATH =+ "${WORKSPACE}/:"
SRC_URI = "file://sensors/"
SRC_URI += "file://init_qti_sensors"
SRC_URI += "file://sensors-daemon.service"

SRC_DIR = "${WORKSPACE}/sensors/sensors"
S = "${WORKDIR}/sensors/sensors"

CFLAGS += "-I${STAGING_INCDIR}/cutils -I${STAGING_INCDIR}/utils"
LDFLAGS += "-lcutils -lutils"
CFLAGS += "-Dstrlcpy=g_strlcpy"
CFLAGS += "-Dstrlcat=g_strlcat"

EXTRA_OECONF += " --enable-sns-la"
EXTRA_OECONF += " --enable-sns-le-apq8053"
EXTRA_OECONF += " --with-glib"
EXTRA_OECONF += " --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"
EXTRA_OEMAKE += " 'CC=${CC}' 'CFLAGS=${CFLAGS}'"

INITSCRIPT_NAME = "init_qti_sensors"
INITSCRIPT_PARAMS = "start 99 2 3 4 5 S . stop 1 0 1 6 K ."

do_install_append() {
    if [ "${MLPREFIX}" == "lib32-" ] || [ "${MLPREFIX}" == "" -a "${TUNE_ARCH}" == "arm" ]; then
        install -d ${D}/data/sensors
        install -m 0777 ${D}/usr/bin/sensorrdiag -D ${D}/data/sensors/sensorrdiag
    fi
    install -d ${D}${sysconfdir}/sensors
    install -m 0755 ${WORKDIR}/init_qti_sensors -D ${D}${sysconfdir}/init.d/init_qti_sensors
    install -m 0644 ${S}/dsps/reg_defaults/sensor_def_qcomdevl.conf -D ${D}${sysconfdir}/sensors/sensor_def_qcomdev.conf
    chmod 0755 ${D}/usr/bin/sensors.qcom
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -d ${D}${systemd_unitdir}/system/
        install -m 0644 ${WORKDIR}/sensors-daemon.service -D ${D}${systemd_unitdir}/system/sensors-daemon.service
        install -d ${D}${systemd_unitdir}/system/multi-user.target.wants/
        # enable the service for multi-user.target
        ln -sf ${systemd_unitdir}/system/sensors-daemon.service \
             ${D}${systemd_unitdir}/system/multi-user.target.wants/sensors-daemon.service
    fi
}

pkg_postinst_${PN}() {
    [ -n "$D" ] && OPT="-r $D" || OPT="-s"
    #remove all rc.d-links potentially created from alternatives
    update-rc.d $OPT -f ${INITSCRIPT_NAME} remove
    update-rc.d $OPT ${INITSCRIPT_NAME} ${INITSCRIPT_PARAMS}
}

#Disable the split of debug information into -dbg files
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

#Skips check for .so symlinks
INSANE_SKIP_${PN} = "dev-so"

FILES_${PN} += "/data/*"
FILES_${PN} += "${includedir}/*"
FILES_${PN} += "${systemd_unitdir}/system/"
FILES_${PN} += "${libdir}/*.so ${libdir}/pkgconfig/*"
FILES_${PN}-dev  = "${libdir}/*.la"
