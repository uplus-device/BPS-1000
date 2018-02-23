inherit autotools update-rc.d qcommon qlicense

DESCRIPTION = "Automatically shuts the modem down on halt and reboot"
PR="r2"

SRC_URI = "file://modem-shutdown.sh"
S = "${WORKDIR}"
SRC_DIR = "${THISDIR}"

#does not appear to work if run as start 01
INITSCRIPT_NAME = "modem-shutdown"
INITSCRIPT_PARAMS = "stop 01 0 6 ."

SSTATE_CREATE_PKG = "0"

do_install() {
        install -m 0755 ${S}/modem-shutdown.sh -D ${D}/${sysconfdir}/init.d/modem-shutdown
        mkdir -p ${D}/bin
        ln -s /etc/init.d/modem-shutdown ${D}/bin/modem-shutdown 
}
