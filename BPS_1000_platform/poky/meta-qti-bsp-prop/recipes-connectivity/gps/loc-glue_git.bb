inherit autotools-brokensep pkgconfig qcommon qlicense qprebuilt

DESCRIPTION = "GPS Location Service library"
PR = "r1"

FILESPATH =+ "${WORKSPACE}:"
SRC_DIR = "${WORKSPACE}/gps/framework/native/framework-glue"
S = "${WORKDIR}/gps/framework/framework-glue"
DEPENDS = "glib-2.0 loc-pla loc-hal gps-utils lbs-core location-geofence location-flp location-service ulp2 loc-net-iface data-items"
EXTRA_OECONF = "--with-glib"
