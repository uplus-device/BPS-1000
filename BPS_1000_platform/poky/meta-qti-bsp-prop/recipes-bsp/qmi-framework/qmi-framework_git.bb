inherit qcommon update-rc.d qprebuilt qlicense

DESCRIPTION = "QMI Framework Library"

PR = "r8"

SRC_DIR = "${WORKSPACE}/qmi-framework"
S = "${WORKDIR}/qmi-framework"

DEPENDS = "qmi"
DEPENDS += "glib-2.0"

EXTRA_OECONF = "--with-qmux-libraries=${STAGING_LIBDIR} --with-glib"

INITSCRIPT_NAME = "init_irsc_util"
INITSCRIPT_PARAMS = "start 29 2 3 4 5 . stop 71 0 1 6 ."

do_install_append() {
      install -m 0755 ${S}/irsc_util/start_irsc_util -D ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}

pkg_postinst_${PN} () {
      [ -n "$D" ] && OPT="-r $D" || OPT="-s"
      update-rc.d $OPT -f ${INITSCRIPT_NAME} remove
      update-rc.d $OPT ${INITSCRIPT_NAME} ${INITSCRIPT_PARAMS}
}
