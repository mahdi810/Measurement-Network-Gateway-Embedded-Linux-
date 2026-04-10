#
# This file is the libgcc recipe.
#

SUMMARY = "Simple libgcc application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://libgcc_s.so.1 \
	"

S = "${WORKDIR}"

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 libgcc_s.so.1 ${D}${bindir}
}
