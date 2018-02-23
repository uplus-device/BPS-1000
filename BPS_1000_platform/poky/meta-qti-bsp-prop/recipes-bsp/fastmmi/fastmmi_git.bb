inherit autotools qcommon qlicense
DESCRIPTION = "fastmmi"
PR = "r0"

DEPENDS += "glib-2.0 diag libxml2"

DEPENDS_append_arm += "${@base_conditional('PRODUCT', 'qsap', ' ', '${MLPREFIX}mm-camera', d)}"
INSANE_SKIP_${PN} += "dev-deps"

S = "${WORKDIR}/vendor/qcom/proprietary/fastmmi"
SRC_DIR = "${WORKSPACE}/vendor/qcom/proprietary/fastmmi/"

EXTRA_OECONF += "--with-glib"
EXTRA_OECONF_append = "${@base_conditional('PRODUCT', 'drone', ' --enable-product=drone', '', d)}"
EXTRA_OECONF_append = "${@base_conditional('PRODUCT', 'qsap', ' --enable-product=qsap', '', d)}"

EXTRA_OECONF_append_apq8053 += "--enable-machine_bits=64bit"
EXTRA_OECONF_append_apq8096 += "--enable-machine_bits=64bit"

CFLAGS_append = " -I${STAGING_INCDIR}/diag \
    `pkg-config --cflags glib-2.0`"

CXXFLAGS_append = " -I${STAGING_INCDIR}/libxml2 -I${STAGING_LIBDIR}/glib-2.0/include -I${STAGING_INCDIR}/diag \
    `pkg-config --cflags glib-2.0`"

CXXFLAGS_append += "-I${STAGING_KERNEL_BUILDDIR}/usr/include/media \
                    -I${STAGING_KERNEL_BUILDDIR}/usr/include \
                    -I${STAGING_KERNEL_BUILDDIR}/usr \
                    `pkg-config --cflags glib-2.0`"

FILES_${PN}-dbg  = "${libdir}/.debug/* ${bindir}/.debug/*"
FILES_${PN} += "/data/FTM_AP"
FILES_${PN} += "${libdir}/*.so"
FILES_${PN}-dev  = "${libdir}/*.la ${includedir}"

do_install_append() {
    dest=/etc/mmi
    install -m 0755 -d ${D}${dest}
    # create FTM_AP folder
    install -dm0755 ${D}/data/FTM_AP
    # Add mmi.xml file
    install -m 0755 ${S}/res/config/le/mmi.xml -D ${D}${dest}/mmi.xml
    install -m 0755 ${S}/res/values/path_config_le.xml -D ${D}${dest}/path_config.xml
    install -m 0755 ${S}/res/config/le/start_mmi -D ${D}${sysconfdir}/init.d/start_mmi
    install -m 0755 ${S}/qti-system-daemon/start_qti_system -D ${D}${sysconfdir}/init.d/start_qti_system
    install -d ${D}${dest}/layout
    install -m 0755 ${S}/res/layout/*.xml -D ${D}${dest}/layout
    install -m 0755 ${S}/res/values/* -D ${D}${dest}
    install -m 0755 ${S}/res/raw/* -D ${D}${dest}
}

pkg_postinst_${PN} () {
        [ -n "$D" ] && OPT="-r $D" || OPT="-s"
        update-rc.d $OPT -f start_mmi remove
        update-rc.d $OPT start_mmi start 98 4 .

        update-rc.d $OPT -f start_qti_system remove
        update-rc.d $OPT start_qti_system start 45 2 3 4 5 .
}
