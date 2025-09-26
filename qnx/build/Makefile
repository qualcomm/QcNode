ifdef QNX_HOST

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)
include ${BSP_ROOT}/fileset_copy.mk

QCNODE_ROOT=$(AMSS_ROOT)/qcnode/vendor/qcom/proprietary/qcnode

all : install

depends:
	make -C ${BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/build/qnx/qcx/libqcxclient install
	make -C ${BSP_ROOT}/AMSS/multimedia/qcamera/camera_qcx/build/qnx/qcx-common/osal install
	make -C ${BSP_ROOT}/AMSS/multimedia/compute/fadas install

install:
	@echo Starting QCNODE build...
	$(shell chmod +x ./scripts/build-target.sh)
	ls
	./scripts/build-target.sh $(QCNODE_ROOT)
	cp -v secpolfiles/qcnode.txt ${BSP_ROOT}/install/secpolfiles/

hinstall:

# All clean from here
clean:
	@echo Cleaning QCNODE build...
	rm -rf tmp

else

hinstall:
install:
clean:

endif
