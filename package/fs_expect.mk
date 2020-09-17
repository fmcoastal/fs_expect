################################################################################
#
# PACKAGE_NAME: fs_expect
#
# the directives below are defined in:
#
# from https://groups.google.com/forum/#!topic/acmesystems/kPjYepcftLM
# 
################################################################################

FS_EXPECT_VERSION = 1.0.0
FS_EXPECT_SOURCE = fs_expect-$(TWSI_TOOL_VERSION).tgz

#STRIP_COMPONENTS tell the tar how many directory structures to 
# strip off the front end of your archive. (See directive in manual)
FS_EXPECT_STRIP_COMPONENTS=0
####  WHERE TO DOWNLOAD YOUR SOURCE FROM

#TWSI_TOOL_SITE_METHOD = wget
#TWSI_TOOL_SITE = root@10.11.65.84:80
FS_EXPECT_SITE_METHOD = file
FS_EXPECT_SITE = /scratch/fs_expect


####  END Download Options

FS_EXPECT_LICENSE = GPL-2.0
FS_EXPECT_LICENSE_FILES = COPYING

FS_EXPECT_INSTALL_TARGET:=YES

#BUILD COMMAND LINE
#  if defined, buildroot will execute.
define TWSI_TOOL_BUILD_CMDS
        $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

#INSTALL COMMAND LINE - places executible in resulting /bin directory
#  if defined, buildroot will execute.
define FS_EXPECT_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(@D)/fs_expect $(TARGET_DIR)/bin
endef

#HOW PERMISSIONS ARE SET
#  if defined, buildroot will execute.
define fs_expect_PERMISSIONS
       /bin/fs_expect f 4755 0 0 - - - - - 
endef


$(eval $(generic-package))
