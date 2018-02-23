# Packages that drone installed to the root file system
require ${BASEMACHINE}-base-qti-image.inc

IMAGE_INSTALL += "lib32-mm-video-utils"
#IMAGE_INSTALL += "flight-slpi"
IMAGE_INSTALL += "flight-addon"
IMAGE_INSTALL += "thermal-engine"
IMAGE_INSTALL += "adsp-sensor-test"
IMAGE_INSTALL += "start-scripts-qseecomd-daemon"
IMAGE_INSTALL += "start-scripts-sfs-config"
IMAGE_INSTALL += "securemsm-noship"
IMAGE_INSTALL += "securemsm"
IMAGE_INSTALL += "adreno200"
IMAGE_INSTALL += "lib32-securemsm-noship"
IMAGE_INSTALL += "lib32-securemsm"
