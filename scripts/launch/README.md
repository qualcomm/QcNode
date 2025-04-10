# Launch scripts for RideHal

## How to Run the RideHal package

For how to build the RideHal package, check this [README](../build/README.md).

### For the QNX
```sh
# on host Ubuntu, push the package to QNX target folder /data
lftp -c 'open -uroot, 192.168.1.1; put -O/data ridehal-aarch64-qos222.tar.gz'

telnet 192.168.1.1
# enter "root" to login
cd /data
tar xfv ridehal-aarch64-qos222.tar.gz
cd pkg-aarch64-qos222/opt/ridehal
# using ./bin/rhrun to launch the RideHal application binary
./bin/rhrun ./bin/gtest_Buffer
./bin/rhrun ./bin/gtest_ComponentIF
./bin/rhrun ./bin/gtest_Logger
```

### For the Linux HGY
```sh
# on host Ubuntu, push the package to QNX target folder /data
adb connect 192.168.1.1
adb root
adb push ridehal-aarch64-linux.tar.gz /data

# on HGY
adb shell
cd /data
tar xfv ridehal-aarch64-linux.tar.gz
cd pkg-aarch64-linux/opt/ridehal
# using ./bin/rhrun to launch the RideHal application binary
./bin/rhrun ./bin/gtest_Buffer
./bin/rhrun ./bin/gtest_ComponentIF
./bin/rhrun ./bin/gtest_Logger
```


### For the Ubuntu HGY
```sh
# on host Ubuntu, push the package to QNX target folder /data
adb connect 192.168.1.1
adb root
adb push ridehal-aarch64-ubuntu.tar.gz /data

# on HGY
adb shell
cd /data
tar xfv ridehal-aarch64-ubuntu.tar.gz
cd pkg-aarch64-ubuntu/opt/ridehal
# using ./bin/rhrun to launch the RideHal application binary
./bin/rhrun ./bin/gtest_Buffer
./bin/rhrun ./bin/gtest_ComponentIF
./bin/rhrun ./bin/gtest_Logger
```
