inherit autotools qcommon qlicense qprebuilt

DESCRIPTION = "GPS Location process launcher"
PR = "r1"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://gps/launcher/"
SRC_DIR = "${WORKSPACE}/gps/launcher/"
S = "${WORKDIR}/gps/launcher"
DEPENDS = "glib-2.0 qmi-framework loc-base-util gps-utils loc-mq-client loc-pla"
EXTRA_OECONF ="--with-glib"

