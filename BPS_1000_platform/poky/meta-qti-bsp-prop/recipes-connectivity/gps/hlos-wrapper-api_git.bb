inherit autotools-brokensep pkgconfig qprebuilt

DESCRIPTION = "HLOS Wrapper API"
PR = "r1"
LICENSE = "Qualcomm-Technologies-Inc.-Proprietary"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-qti-bsp-prop/files/qcom-licenses/Qualcomm-Technologies-Inc.-Proprietary;md5=92b1d0ceea78229551577d4284669bb8"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://gps-noship/libs/hlos-wrapper-api/"
SRC_DIR = "${WORKSPACE}/gps-noship/libs/hlos-wrapper-api/"
S = "${WORKDIR}/gps-noship/libs/hlos-wrapper-api/"
DEPENDS = "glib-2.0 loc-pla gps-utils xt-adapter loc-hal izat-core location-service"
EXTRA_OECONF = "--with-glib"
