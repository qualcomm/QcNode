
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
# black build-package.py -l 120
import os
import sys
import shutil
import argparse
import logging
import json

logging.basicConfig(level=logging.DEBUG, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")
logger = logging.getLogger("BUILD")


def RunCommand(cmd):
    logger.info("execute: %s" % (cmd))
    if type(cmd) is list:
        command = " && ".join(cmd)
    else:
        command = cmd
    r = os.system(command)
    assert 0 == r


parser = argparse.ArgumentParser(description="build package")
parser.add_argument(
    "--no_sudo", action="store_false", dest="sudo", default=True, help="Do not use sudo.", required=False
)
parser.add_argument(
    "--variant", type=str, default="linux", help="The build variant, [qnx, linux, ubuntu]", required=False
)
parser.add_argument(
    "--socid", type=str, default="8797", help="The SOC ID, [8295, 8620, 8650, 8775, 8797]", required=False
)
parser.add_argument(
    "-d", "--docker", type=str, default="qcnode-toolchain-base:v1.0", help="The docker name", required=False
)
parser.add_argument(
    "-q", "--qnn_sdk", type=str, default="./toolchain/qnn_sdk", help="The path to QNX SDK ROOT", required=True
)
parser.add_argument("-c", "--clean", action="store_true", default=False, help="Build form clean state", required=False)
parser.add_argument(
    "-t",
    "--toolchain",
    type=str,
    default="./toolchain/linux",
    help="The path to the compiler toochain folder",
    required=True,
)
parser.add_argument(
    "-e", "--env_script", type=str, default="", help="Optional toolchain env script to source.", required=False
)
args = parser.parse_args()
HOMEDIR = os.path.dirname(__file__)
if HOMEDIR == "":
    HOMEDIR = "."
HOMEDIR = os.path.abspath(HOMEDIR)
WORKSPACE = HOMEDIR + "/tmp"
VARIANT = args.variant
SOCID = args.socid
DOCKER = args.docker
QNN_SDK = os.path.abspath(args.qnn_sdk)
TOOLCHAIN = os.path.abspath(args.toolchain)
ENV_SCRIPT = args.env_script
THIRD_PARTY = WORKSPACE + "/third_party"
BUILD_DIR = WORKSPACE + "/build/" + SOCID
if args.sudo:
    OPT_SUDO = "sudo "
else:
    OPT_SUDO = ""

RunCommand(["mkdir -p " + WORKSPACE])

if TOOLCHAIN.startswith("/prj"):
    MOUNT_TOOLCHAIN = "-v /prj/:/prj"
else:
    MOUNT_TOOLCHAIN = f"-v {TOOLCHAIN}:/opt/{VARIANT}"
if ENV_SCRIPT != "":
    OPT_ENV_SETUP = f"cd {TOOLCHAIN}; source {ENV_SCRIPT} || true;"
elif VARIANT == "linux" and TOOLCHAIN.startswith("/prj"):
    OPT_ENV_SETUP = f"export LINUX_SDK_ROOT={TOOLCHAIN};"
else:
    OPT_ENV_SETUP = ""

if args.clean:
    RunCommand([f"{OPT_SUDO}rm -frv {BUILD_DIR}/bld-aarch64-{VARIANT}"])
with open(WORKSPACE + "/build.sh", "w") as f:
    f.write(
        f"""#!/bin/bash
mkdir -p {BUILD_DIR}/bld-aarch64-{VARIANT}
mkdir -p {BUILD_DIR}/run-aarch64-{VARIANT}
mkdir -p {THIRD_PARTY}
{OPT_SUDO}docker run -it \\
    -u $(id -u):$(id -g) \\
    {MOUNT_TOOLCHAIN} \\
    -v {HOMEDIR}:/data/qcnode \\
    -v {QNN_SDK}:/opt/qnn_sdk \\
    -v {THIRD_PARTY}:/data/qcnode/third_party \\
    -v {BUILD_DIR}:/opt/build \\
    -v {BUILD_DIR}/bld-aarch64-{VARIANT}:/data/qcnode/bld-aarch64-{VARIANT} \\
    -v {BUILD_DIR}/run-aarch64-{VARIANT}:/data/qcnode/run-aarch64-{VARIANT} \\
    --net=host --privileged -v /dev/bus/usb:/dev/bus/usb \\
    --rm {DOCKER} bash \\
    -c "set -xe; {OPT_ENV_SETUP} \\
        cd /data/qcnode; \\
        export QC_TARGET_SOC={SOCID}; \\
        ./scripts/build/build-target.sh aarch64-{VARIANT} .; \\
        mv -v qcnode-aarch64-{VARIANT}.tar.gz /opt/build;"
echo ======================  build output under {BUILD_DIR} ======================
ls -l {BUILD_DIR}
"""
    )

RunCommand(["sh " + WORKSPACE + "/build.sh"])