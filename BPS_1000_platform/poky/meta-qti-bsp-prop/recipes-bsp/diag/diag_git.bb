inherit qcommon qprebuilt qlicense

DESCRIPTION = "Library and routing applications for diagnostic traffic"
DEPENDS = "common glib-2.0 system-core time-genoff"

PR = "r10"

FILESPATH =+ "${WORKSPACE}/:"
SRC_URI   = "file://diag/"
SRC_URI  += "file://chgrp-diag"

SRC_DIR = "${WORKSPACE}/diag"
S       = "${WORKDIR}/diag"

EXTRA_OECONF += "--with-glib \
                 --with-common-includes=${STAGING_INCDIR}"

do_install_append() {
	install -m 0755 ${WORKDIR}/chgrp-diag -D ${D}${sysconfdir}/init.d/chgrp-diag
}

pkg_postinst_${PN} () {
        [ -n "$D" ] && OPT="-r $D" || OPT="-s"
        update-rc.d $OPT -f start_diag_qshrink4_daemon remove
        update-rc.d $OPT start_diag_qshrink4_daemon start 15 2 3 4 5 . stop 15 0 6 .
        
        update-rc.d $OPT -f chgrp-diag remove
        update-rc.d $OPT chgrp-diag start 15 2 3 4 5 .
}
