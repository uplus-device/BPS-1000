inherit qcommon qlicense update-rc.d qprebuilt

DESCRIPTION = "Thermal Engine"
SECTION = "base"

PR = "r10"

SRC_DIR = "${WORKSPACE}/thermal-engine"
S = "${WORKDIR}/thermal-engine"

DEPENDS = "qmi-framework glib-2.0"

EXTRA_OECONF  = "--with-qmi-framework  --with-glib"
EXTRA_OECONF += "--with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"
EXTRA_OECONF += "--enable-target-${BASEMACHINE}=yes"

INITSCRIPT_NAME = "thermal-engine"
INITSCRIPT_PARAMS = "start 40 2 3 4 5 . stop 60 0 1 6 ."

do_install_append() {
       install -m 0755 ${WORKDIR}/thermal-engine/start_thermal-engine_le -D ${D}${sysconfdir}/init.d/thermal-engine
}
