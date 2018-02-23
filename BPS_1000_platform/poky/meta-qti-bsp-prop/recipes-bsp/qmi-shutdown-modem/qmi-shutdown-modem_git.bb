inherit qcommon qprebuilt qlicense update-rc.d

DESCRIPTION = "QMI shutdown modem Daemon"
PR = "r4"

FILESPATH =+ "${WORKSPACE}:"
SRC_URI  = "file://mdm-ss-mgr/qmi-shutdown-modem/"
SRC_URI += "file://qmi_shutdown_modemd.service"

SRC_DIR = "${WORKSPACE}/mdm-ss-mgr/qmi-shutdown-modem/"

S = "${WORKDIR}/mdm-ss-mgr/qmi-shutdown-modem/"

EXTRA_OECONF_msm += "--enable-target-apq"

DEPENDS = "qmi-framework"
RDEPENDS_${PN} = "qmi-framework"

INITSCRIPT_NAME = "qmi_shutdown_modemd"
INITSCRIPT_PARAMS = "start 45 2 3 4 5 ."

do_install_append (){
   install -m 0755 ${S}/qmi_shutdown_modemd -D ${D}${sysconfdir}/init.d/qmi_shutdown_modemd
   if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
      install -d ${D}${systemd_unitdir}/system/
      install -m 0644 ${WORKDIR}/qmi_shutdown_modemd.service -D ${D}${systemd_unitdir}/system/qmi_shutdown_modemd.service
      install -d ${D}${systemd_unitdir}/system/multi-user.target.wants/
      # enable the service for multi-user.target
      ln -sf ${systemd_unitdir}/system/qmi_shutdown_modemd.service \
           ${D}${systemd_unitdir}/system/multi-user.target.wants/qmi_shutdown_modemd.service
   fi
}

FILES_${PN} += "${systemd_unitdir}/system/"
