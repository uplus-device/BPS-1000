inherit autotools-brokensep pkgconfig qcommon qlicense qprebuilt

DESCRIPTION = "GPS LBS Core"
PR = "r1"

FILESPATH =+ "${WORKSPACE}:"
SRC_DIR = "${WORKSPACE}/gps/framework/native/core/"
DEPENDS = "glib-2.0 qmi-framework loc-pla gps-utils loc-hal izat-core loc-base-util loc-flp-hdr loc-net-iface"
EXTRA_OECONF ="--with-glib \
              --with-locflp-includes=${STAGING_INCDIR}/loc-flp-hdr"

S = "${WORKDIR}/gps/framework/native/core"
