inherit autotools qcommon qlicense qprebuilt

DESCRIPTION = "SLIM Utils"
PR = "r1"
SRC_DIR = "${WORKSPACE}/gps/slim/utils/"
S = "${WORKDIR}/gps/slim/utils"
DEPENDS = "glib-2.0 qmi-framework slim-common-hdr loc-pla diag"
EXTRA_OECONF = "--with-slimcommon-includes=${STAGING_INCDIR}/slim-common-hdr \
                --with-core-includes=${STAGING_INCDIR} \
                --with-glib"

