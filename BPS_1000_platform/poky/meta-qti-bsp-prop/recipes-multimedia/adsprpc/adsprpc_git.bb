inherit qcommon qlicense qprebuilt

SUMMARY = "adsprpc daemon"

DEPENDS = "system-core"

FILESPATH =+ "${WORKSPACE}/:"
SRC_URI   = "file://adsprpc"
SRC_URI  += "file://start_adsprpcd"
SRC_URI  += "file://start_sdsprpcd"
SRC_URI  += "file://start_mdsprpcd"

SRC_DIR   = "${WORKSPACE}/adsprpc"

S  = "${WORKDIR}/adsprpc"
PR = "r0"

EXTRA_OECONF_apq8096 += "--enable-sdsprpc"
EXTRA_OECONF_apq8009 += "--enable-mdsprpc"

INITSCRIPT_NAME = "adsprpcd"
INITSCRIPT_NAME_apq8009 = "mdsprpcd"

do_install_append () {
    install -m 0755 ${WORKDIR}/start_${INITSCRIPT_NAME} -D ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}

pkg_postinst_${PN} () {
    update-alternatives --install ${sysconfdir}/init.d/${INITSCRIPT_NAME} ${INITSCRIPT_NAME} ${INITSCRIPT_NAME} 60
    [ -n "$D" ] && OPT="-r $D" || OPT="-s"
    # remove all rc.d-links potentially created from alternatives
    update-rc.d $OPT -f ${INITSCRIPT_NAME} remove
    update-rc.d $OPT ${INITSCRIPT_NAME} start 70 S 2 3 4 5 S . stop 70 0 1 6 .
}

do_install_append_apq8096 () {
        install -m 0755 ${WORKDIR}/start_sdsprpcd -D ${D}${sysconfdir}/init.d/sdsprpcd
}

pkg_postinst_${PN}_append_apq8096 () {
        update-rc.d $OPT -f sdsprpcd remove
        update-rc.d $OPT sdsprpcd start 31 2 3 4 5 . stop 71 0 1 6 .
}

FILES_${PN}-dbg  = "${libdir}/.debug/* ${bindir}/.debug/*"
FILES_${PN}      = "${libdir}/*.so ${libdir}/*.so.* ${sysconfdir}/* ${bindir}/* ${libdir}/pkgconfig/*"
FILES_${PN}-dev  = "${libdir}/*.la ${includedir}"
