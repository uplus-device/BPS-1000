SS_MGR_PACKAGE = "packagegroup-ss-mgr"
# MDM9607 doesn't use init-mss, explicitly installing reboot-daemon for this machine
SS_MGR_PACKAGE_mdm9607 = "reboot-daemon"
IMAGE_INSTALL += "${SS_MGR_PACKAGE}"
