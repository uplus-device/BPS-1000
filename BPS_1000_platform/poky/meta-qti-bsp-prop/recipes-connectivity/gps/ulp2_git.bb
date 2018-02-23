inherit autotools-brokensep pkgconfig qcommon qlicense qprebuilt

DESCRIPTION = "GPS LBS Core"
PR = "r1"

FILESPATH =+ "${WORKSPACE}:"
SRC_DIR = "${WORKSPACE}/gps/ulp2"
DEPENDS = "glib-2.0 qmi-framework loc-pla loc-hal gps-utils izat-core lbs-core drplugin event-observer loc-flp-hdr"
EXTRA_OECONF ="--with-glib \
               --with-locflp-includes=${STAGING_INCDIR}/loc-flp-hdr"

S = "${WORKDIR}/ulp2"
