inherit autotools-brokensep pkgconfig qprebuilt

DESCRIPTION = "XT Adapter"
PR = "r1"
LICENSE = "Qualcomm-Technologies-Inc.-Proprietary"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta-qti-bsp-prop/files/qcom-licenses/Qualcomm-Technologies-Inc.-Proprietary;md5=92b1d0ceea78229551577d4284669bb8"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI = "file://gps-noship/XT/XTAdapter/"
SRC_DIR = "${WORKSPACE}/gps-noship/XT/XTAdapter/"
S = "${WORKDIR}/gps-noship/XT/XTAdapter/"
DEPENDS = "glib-2.0 loc-pla loc-hal izat-core gps-utils"
EXTRA_OECONF = "--with-glib"
