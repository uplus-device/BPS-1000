inherit update-rc.d systemd

DESCRIPTION = "Q6 Admin Utilities"
LICENSE = "Qualcomm-Technologies-Inc.-Proprietary"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-qti-bsp-prop/files/qcom-licenses/\
Qualcomm-Technologies-Inc.-Proprietary;md5=92b1d0ceea78229551577d4284669bb8"

PACKAGES = "${PN}-adsp ${PN}-slpi"

SRC_URI  = "file://adsp-start.sh"
SRC_URI  += "file://adsp.sh"
SRC_URI  += "file://slpi-start.sh"
SRC_URI  += "file://slpi.sh"

FILES_${PN}-adsp += "/usr/local/qr-linux/adsp-start.sh"
FILES_${PN}-adsp += "/etc/init.d/adsp.sh"
FILES_${PN}-slpi += "/usr/local/qr-linux/slpi-start.sh"
FILES_${PN}-slpi += "/etc/init.d/slpi.sh"


INITSCRIPT_PACKAGES = "${PN}-adsp ${PN}-slpi"
INITSCRIPT_NAME_${PN}-adsp = "adsp.sh"
INITSCRIPT_NAME_${PN}-slpi = "slpi.sh"
INITSCRIPT_PARAMS_${PN}-adsp = "start 50 5 . stop 20 0 1 6 ."
INITSCRIPT_PARAMS_${PN}-slpi = "start 51 5 . stop 21 0 1 6 ."

do_install() {
    dest=/usr/local/qr-linux
    install -d ${D}${dest}
    install -m 755 ${WORKDIR}/adsp-start.sh ${D}${dest}
    install -m 755 ${WORKDIR}/slpi-start.sh ${D}${dest}

    dest=/etc/init.d/
    install -d ${D}${dest}
    install -m 0755 ${WORKDIR}/adsp.sh -D ${D}${dest}
    install -m 0755 ${WORKDIR}/slpi.sh -D ${D}${dest}

}

pkg_postinst_${PN}-adsp() {
    # Create the folder for ADSP dynamic libs
    install -d $D/usr/share/data/adsp
    [ -n "$D" ] && OPT="-r $D" || OPT="-s"
    update-rc.d $OPT ${INITSCRIPT_NAME_${PN}-adsp} ${INITSCRIPT_PARAMS_${PN}-adsp}
}

pkg_postinst_${PN}-slpi() {
    # Create the folder for SLPI dynamic libs
    install -d $D/dsp
    [ -n "$D" ] && OPT="-r $D" || OPT="-s"
    update-rc.d $OPT ${INITSCRIPT_NAME_${PN}-slpi} ${INITSCRIPT_PARAMS_${PN}-slpi}
}
