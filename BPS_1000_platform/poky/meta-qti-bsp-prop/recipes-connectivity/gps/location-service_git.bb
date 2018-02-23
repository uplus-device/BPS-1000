inherit autotools-brokensep pkgconfig qcommon qlicense qprebuilt

DESCRIPTION = "GPS Location Service library"
PR = "r1"

FILESPATH =+ "${WORKSPACE}:"
SRC_DIR = "${WORKSPACE}/gps/framework/native"
S = "${WORKDIR}/gps/native"
DEPENDS = "glib-2.0 loc-pla loc-hal gps-utils lbs-core lowi-client ulp2 loc-flp-hdr data-items"
EXTRA_OECONF = "--with-locflp-includes=${STAGING_INCDIR}/loc-flp-hdr \
                --with-glib"
