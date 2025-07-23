# Launch scripts for QCNode

## How to Run the QCNode package

### For the QNX
```sh
# on host Ubuntu, push the package to QNX target folder /data
lftp -c 'open -uroot, 192.168.1.1; put -O/data qcnode-aarch64-qnx.tar.gz'

telnet 192.168.1.1
# enter "root" to login
cd /data
tar xfv qcnode-aarch64-qnx.tar.gz
cd pkg-aarch64-qnx/opt/qcnode
# using ./bin/qcrun to launch the QCNode application binary
./bin/qcrun ./bin/gtest_Memory
./bin/qcrun ./bin/gtest_Logger
```

### For the Linux HGY
```sh
# on host Ubuntu, push the package to Linux target folder /data
adb connect 192.168.1.1
adb root
adb push qcnode-aarch64-linux.tar.gz /data

# on HGY
adb shell
cd /data
tar xfv qcnode-aarch64-linux.tar.gz
cd pkg-aarch64-linux/opt/qcnode
# using ./bin/qcrun to launch the QCNode application binary
./bin/qcrun ./bin/gtest_Memory
./bin/qcrun ./bin/gtest_Logger
```


### For the Ubuntu HGY
```sh
# on host Ubuntu, push the package to Ubuntu target folder /data
adb connect 192.168.1.1
adb root
adb push qcnode-aarch64-ubuntu.tar.gz /data

# on HGY
adb shell
cd /data
tar xfv qcnode-aarch64-ubuntu.tar.gz
cd pkg-aarch64-ubuntu/opt/qcnode
# using ./bin/qcrun to launch the QCNode application binary
./bin/qcrun ./bin/gtest_Memory
./bin/qcrun ./bin/gtest_Logger
```
