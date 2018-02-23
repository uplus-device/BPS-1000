inherit qcommon qlicense qprebuilt

DESCRIPTION = "Daemon to handle AT commands"

DEPENDS = "glib-2.0 qmi qmi-framework libcutils"

SRC_DIR = "${WORKSPACE}/mdm-ss-mgr/atfwd-daemon/"
S = "${WORKDIR}/mdm-ss-mgr/atfwd-daemon/"

PR = "r3"

CFLAGS += "-I${STAGING_INCDIR}/cutils"
