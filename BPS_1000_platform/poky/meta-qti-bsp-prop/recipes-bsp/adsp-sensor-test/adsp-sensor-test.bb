inherit autotools qcommon qlicense qprebuilt
DESCRIPTION = "adsp-sensor-test"

PR = "r0"
PV = "1.0"

DEPENDS += "glib-2.0 diag libxml2"
INSANE_SKIP_${PN} += "dev-deps"

S = "${WORKDIR}/vendor/qcom/proprietary/drones/sensortest"
SRC_DIR = "${WORKSPACE}/vendor/qcom/proprietary/drones/sensortest/"

EXTRA_OECONF += "--with-glib"

do_install_append() {
    dest=/etc/mmi
    install -m 0755 -d ${D}${dest}
    install -m 0755 ${S}/config/sensor.config -D ${D}${dest}/sensor.config
    install -m 0755 ${S}/config/start_adsp_sensor_test -D ${D}${sysconfdir}/init.d/start_adsp_sensor_test
}

pkg_postinst_${PN} () {
    [ -n "$D" ] && OPT="-r $D" || OPT="-s"
    update-rc.d $OPT -f start_adsp_sensor_test remove
    update-rc.d $OPT start_adsp_sensor_test start 97 4 .
}
