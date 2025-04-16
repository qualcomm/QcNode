# Launch scripts for QC

## How to Run the QCNode package

For how to build the QCNode package, check this [README](../build/README.md).

### For the QNX
```sh
# on host Ubuntu, push the package to QNX target folder /data
lftp -c 'open -uroot, 192.168.1.1; put -O/data qcnode-aarch64-qos222.tar.gz'

telnet 192.168.1.1
# enter "root" to login
cd /data
tar xfv qcnode-aarch64-qos222.tar.gz
cd pkg-aarch64-qos222/opt/qcnode
# using ./bin/qcrun to launch the QC application binary
./bin/qcrun ./bin/gtest_Buffer
./bin/qcrun ./bin/gtest_ComponentIF
./bin/qcrun ./bin/gtest_Logger
```

### For the Linux HGY
```sh
# on host Ubuntu, push the package to QNX target folder /data
adb connect 192.168.1.1
adb root
adb push qcnode-aarch64-linux.tar.gz /data

# on HGY
adb shell
cd /data
tar xfv qcnode-aarch64-linux.tar.gz
cd pkg-aarch64-linux/opt/qcnode
# using ./bin/qcrun to launch the QC application binary
./bin/qcrun ./bin/gtest_Buffer
./bin/qcrun ./bin/gtest_ComponentIF
./bin/qcrun ./bin/gtest_Logger
```


### For the Ubuntu HGY
```sh
# on host Ubuntu, push the package to QNX target folder /data
adb connect 192.168.1.1
adb root
adb push qcnode-aarch64-ubuntu.tar.gz /data

# on HGY
adb shell
cd /data
tar xfv qcnode-aarch64-ubuntu.tar.gz
cd pkg-aarch64-ubuntu/opt/qcnode
# using ./bin/qcrun to launch the QC application binary
./bin/qcrun ./bin/gtest_Buffer
./bin/qcrun ./bin/gtest_ComponentIF
./bin/qcrun ./bin/gtest_Logger
```
