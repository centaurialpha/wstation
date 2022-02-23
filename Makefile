PROJECT_NAME := weather-station

EXTRA_COMPONENT_DIRS := /home/gabox/projects/esp/components
# EXCLUDE_COMPONENTS := led_strip ls7366r
INCLUDE_COMPONENTS := dht
include $(IDF_PATH)/make/project.mk
