# Guide to build qcnode-toolchain base docker

## Notes
The qcnode-toolchain base docker provides a basic host toolchain environment that splits from platform SDK toolchains, users can directly map the platform SDK toolchains path to this docker with '-v' option, thus the switching of platform SDK toolchains version could be easily.

## How to build qcnode-toolchain-base docker

- Step 1: Prepare basic docker image
  The basic docker image is ubuntu 20.04, user can get the docker image in two methods.
  - method 1: pull the image with command
    ```
    sudo docker pull ubuntu:20.04
    ```
  - method 2: get the image is from existed docker image package:
    ```
    sudo docker load -i ubuntu-20.04.tar
    ```
    Then use the command to show docker image id:
    ```
    sudo docker images -a
    ```
    The docker image info shows as this way:
    ```
    REPOSITORY                   TAG               IMAGE ID           CREATED         SIZE
    <none>                       <none>            52882761a72a       2 days ago      77.9MB
    ```
    Then tag the docker image with image id:
    ```
    sudo docker tag 52882761a72a ubuntu:20.04
    ```

- Step 2: build qcnode-toolchain-base docker image
  Run the script to build:
  ```
    ./build-docker-image.sh
  ```

  After building, the docker image could be shown as below:
  ```
    sudo docker images -a
    REPOSITORY                           TAG               IMAGE ID           CREATED         SIZE
    qcnode-toolchain-base               v1.0              84bcf57cad70       2 days ago      3.6GB
  ```
