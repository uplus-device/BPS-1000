inherit autotools qcommon qlicense update-rc.d qprebuilt

DESCRIPTION = "WLAN Services required for QCACLD"
PR = "r2"

DEPENDS = "data qcacld-ll"
DEPENDS_mdm9607 = "data qcacld-hl"
DEPENDS_apq8009 = "data qcacld-hl"
DEPENDS_apq8017 = "data qcacld-hl"

RDEPENDS_${PN} = "data"

SRC_DIR = "${WORKSPACE}/wlan-proprietary/wlan-services/"

S = "${WORKDIR}/wlan-proprietary/wlan-services/qcacld"

EXTRA_OECONF = "--enable-debug"

INITSCRIPT_NAME = "start_wlan_services"
INITSCRIPT_PARAMS = "start 90 2 3 4 5 . stop 10 0 1 6 ."
