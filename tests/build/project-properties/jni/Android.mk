WANTED_PLATFORM_32 := android-9
WANTED_PLATFORM_64 := android-21

ifeq (,$(filter %64, $(TARGET_ARCH)))
  ifneq ($(TARGET_PLATFORM),$(WANTED_PLATFORM_32))
    $(error Incorrect target platform: $(TARGET_PLATFORM) (expecteding $(WANTED_PLATFORM_32)))
  endif
  $(call ndk_log,Test OK: Correct target platform retrieved from project.properties: $(TARGET_PLATFORM))
else
  ifneq ($(TARGET_PLATFORM),$(WANTED_PLATFORM_64))
    $(error Incorrect target platform: $(TARGET_PLATFORM) (expecteding $(WANTED_PLATFORM_64)))
  endif
  $(call ndk_log,Test OK: Correct target platform retrieved from project.properties but auto-adjusted: $(TARGET_PLATFORM))
endif
