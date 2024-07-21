#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp8266-idf-tuya

#CXXFLAGS += -frtti
EXTRA_COMPONENT_DIRS := $(PROJECT_PATH)/../../esp8266-rtos-core

include $(IDF_PATH)/make/project.mk

#LDLIBS += -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lwpa2 -lcrypto -lmain
#LDLIBS += -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lwpa2 -lcrypto -lmain -lesp8266



get-size:
	make -j8 size-components

change-target:
	echo "Change target to: ${target}"
	rm -rf ./sdkconfig
	cp sdkconfig."${target}" ./sdkconfig
	make clean

build:
ifdef version
	echo "${version}" > version.txt
endif
	@echo "#ifndef BUILD_INFO_H" > main/build_info.h
	@echo "#define BUILD_INFO_H" >> main/build_info.h
	@echo "#define BUILD_DATE \"$(shell date +%Y-%m-%d)\"" >> main/build_info.h
	@echo "#define BUILD_TIME \"$(shell date +%H:%M:%S)\"" >> main/build_info.h
	@echo "#define GIT_COMMIT \"$(shell git rev-parse --short HEAD)\"" >> main/build_info.h
	@echo "#define APP_VERSION \"$(shell cat version.txt)\"" >> main/build_info.h
	@echo "#endif" >> main/build_info.h

#Example: make release target=esp12e version=1.0.0
release:
ifndef target
		$(error target is not set)
endif
ifndef target
		$(error version is not set)
endif
	make build version=${version}
	make change-target target=${target}
	make