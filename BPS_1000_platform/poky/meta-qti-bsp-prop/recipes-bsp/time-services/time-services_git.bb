inherit autotools qcommon qlicense update-rc.d qprebuilt
DESCRIPTION = "Time Services Daemon"
PR = "r9"

SRC_DIR = "${WORKSPACE}/time-services"
S = "${WORKDIR}/time-services"

DEPENDS += "virtual/kernel"
DEPENDS += "glib-2.0"
DEPENDS += "diag"
DEPENDS += "qmi-framework"
DEPENDS_append_msm8974 += "dlog"
DEPENDS_append_msm8610 += "dlog"
DEPENDS_append_msm8226 += "dlog"
RDEPENDS_${PN} += "qmi-framework"


INITSCRIPT_NAME = "time_serviced"
INITSCRIPT_PARAMS = "start 29 2 3 4 5 . stop 1 0 1 6 ."


EXTRA_OECONF += " --with-glib \
		  --with-common-includes=${STAGING_INCDIR} \
		  --with-sanitized-headers=${STAGING_KERNEL_BUILDDIR}/usr/include"
EXTRA_OECONF_append_msm8974 = " --enable-target-msm8974=yes \
								--with-dlog"
EXTRA_OECONF_append_msm8610 = " --enable-target-msm8610=yes \
								--with-dlog"
EXTRA_OECONF_append_msm8226 = " --enable-target-msm8226=yes \
								--with-dlog"



do_install_append () {
    install -m 0755 ${S}/time_serviced -D ${D}${sysconfdir}/init.d/time_serviced
}

