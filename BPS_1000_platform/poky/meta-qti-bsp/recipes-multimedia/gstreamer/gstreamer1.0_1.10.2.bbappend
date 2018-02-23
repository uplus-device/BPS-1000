
python do_getpatches() {
    import os

    cmd = "cd ${WORKSPACE}/poky/meta-qti-bsp/recipes-multimedia/gstreamer && rm -rf files gst-player gstreamer1.0  && mkdir files gst-player gstreamer1.0 && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gstreamer1.0/deterministic-unwind.patch?h=yoctoproject.org/master -O gstreamer1.0/deterministic-unwind.patch || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/files/0001-introspection.m4-prefix-pkgconfig-paths-with-PKG_CON.patch?h=yoctoproject.org/master -O files/0001-introspection.m4-prefix-pkgconfig-paths-with-PKG_CON.patch || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gst-player/0001-gtk-play-provide-similar-behaviour-for-quit-and-clos.patch?h=yoctoproject.org/master -O gst-player/0001-gtk-play-provide-similar-behaviour-for-quit-and-clos.patch || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gst-player/Add-error-signal-emission-for-missing-plugins.patch?h=yoctoproject.org/master -O gst-player/Add-error-signal-emission-for-missing-plugins.patch || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gst-player/Fix-pause-play.patch?h=yoctoproject.org/master -O gst-player/Fix-pause-play.patch  || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gst-player/filechooser.patch?h=yoctoproject.org/master -O gst-player/filechooser.patch || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gst-player/gst-player.desktop?h=yoctoproject.org/master -O gst-player/gst-player.desktop  || pwd)"

    os.system(cmd)
}

addtask getpatches before do_fetch
