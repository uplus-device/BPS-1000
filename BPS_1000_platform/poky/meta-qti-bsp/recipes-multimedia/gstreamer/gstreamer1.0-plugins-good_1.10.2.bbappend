
python do_getpatches(){
    import os

    cmd = "cd ${WORKSPACE}/poky/meta-qti-bsp/recipes-multimedia/gstreamer && rm -rf  gstreamer1.0-plugins-good && mkdir gstreamer1.0-plugins-good && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gstreamer1.0-plugins-good/0001-gstrtpmp4gpay-set-dafault-value-for-MPEG4-without-co.patch?h=yoctoproject.org/master -O gstreamer1.0-plugins-good/0001-gstrtpmp4gpay-set-dafault-value-for-MPEG4-without-co.patch   || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gstreamer1.0-plugins-good/avoid-including-sys-poll.h-directly.patch?h=yoctoproject.org/master -O gstreamer1.0-plugins-good/avoid-including-sys-poll.h-directly.patch  || pwd)  && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gstreamer1.0-plugins-good/ensure-valid-sentinel-for-gst_structure_get.patch?h=yoctoproject.org/master -O gstreamer1.0-plugins-good/ensure-valid-sentinel-for-gst_structure_get.patch   || pwd) && (wget https://source.codeaurora.org/quic/qyocto/oss/poky/plain/meta/recipes-multimedia/gstreamer/gstreamer1.0-plugins-good/qtdemux-free-seqh-after-calling-qtdemux_parse_svq3_s.patch?h=yoctoproject.org/master -O gstreamer1.0-plugins-good/qtdemux-free-seqh-after-calling-qtdemux_parse_svq3_s.patch  || pwd)"

    os.system(cmd)
}

addtask getpatches before do_fetch