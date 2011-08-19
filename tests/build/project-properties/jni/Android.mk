WANTED_PLATFORM := android-9
ifneq ($(TARGET_PLATFORM),$(WANTED_PLATFORM))
    $(error Incorrect target platform: $(TARGET_PLATFORM) (expecteding $(WANTED_PLATFORM)))
endif

$(call ndk_log,Test OK: Correct target platform retrieved from project.properties: $(TARGET_PLATFORM))

