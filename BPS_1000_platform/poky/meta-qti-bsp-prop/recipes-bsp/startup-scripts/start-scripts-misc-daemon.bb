inherit qcommon qlicense update-rc.d
DESCRIPTION = "Start up script for misc daemons"

SRC_URI ="file://start_misc-daemon"
S = "${WORKDIR}"
SRC_DIR = "${THISDIR}"

PR = "r2"

INITSCRIPT_NAME = "misc-daemon"
INITSCRIPT_PARAMS = "start 98 5 . stop 2 0 1 6 ."

do_install() {
        install -m 0755 ${WORKDIR}/start_misc-daemon -D ${D}${sysconfdir}/init.d/${INITSCRIPT_NAME}
}
