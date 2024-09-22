#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTPUT_DIR=/tmp/aeld
LINUX_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
LINUX_VERSION=v6.10.11
BB_VERSION=1_33_1
SCRIPT_DIR=$(realpath $(dirname $0))
CPU_ARCH=arm64
TOOLCHAIN_PREFIX=aarch64-none-linux-gnu-
#Added this for the busybox dependancies in the cross compile directory. Easier to read and debug with this as the base directory
COMPILER_SYSROOT=$(${TOOLCHAIN_PREFIX}gcc -print-sysroot)

#If arguments are less than 1, use the default OUTPUT_DIR, if not, used the one specified as an argument
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTPUT_DIR} for output"
else
	OUTPUT_DIR=$1
	echo "Using passed directory ${OUTPUT_DIR} for output"
fi

#Added additional code if the creation of the output directory fails
if mkdir -p "${OUTPUT_DIR}"; then
	echo "Directory created successfully at ${OUTPUT_DIR}. Moving on"
else
	echo "Directory Creation failed at ${OUTPUT_DIR}. Failing"
	exit 1
fi

#Clone the linux kernel source directory as stated in step i1 on assgnmt 3 pt 2. Copy the files to OUTPUT_DIR
cd "$OUTPUT_DIR"
if [ ! -d "${OUTPUT_DIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${LINUX_VERSION} IN ${OUTPUT_DIR}"
	git clone ${LINUX_REPO} --depth 1 --single-branch --branch ${LINUX_VERSION}
fi
if [ ! -e ${OUTPUT_DIR}/linux-stable/arch/${CPU_ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${LINUX_VERSION}"
    git checkout ${LINUX_VERSION}

    # TODO: Add your kernel build steps here
    #-j4 just tells us how many cpu cores we can use to build (so 4 in our case)
    #clean step
    echo "do I make it first-----------------------------------------------------------------------------------------------------------"
    make ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} mrproper
    echo "do I make it second-----------------------------------------------------------------------------------------------------------"
    #defconfig
    make -j4 ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} defconfig
    echo "do I make it third-----------------------------------------------------------------------------------------------------------"
    #vmlinux (build the kernel image for booting with QEMU)
    make -j4 ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} all
    echo "FINISHED COMPILATION-----------------------------------------------------------------------------------------------------------"
    
    #modules (skipped because its too big)
    #make -j4 ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} modules
    #device tree (skipped because its too big)
    #make -j4 ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} dtbs
    
    
fi

echo "Adding the Image in OUTPUT_DIR"
cp ${OUTPUT_DIR}/linux-stable/arch/arm64/boot/Image ${OUTPUT_DIR}
cp ${OUTPUT_DIR}/linux-stable/arch/arm64/boot/Image /tmp/aesd-autograder/

echo "Creating the staging directory for the root filesystem"
cd "$OUTPUT_DIR"
if [ -d "${OUTPUT_DIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTPUT_DIR}/rootfs and starting over"
    sudo rm  -rf ${OUTPUT_DIR}/rootfs
fi

# TODO: Create necessary base directories
#The rootfs directory was deleted if it existed, so we gotta remake it
echo "CREATING BASE DIRECTORIES----------------------------------------------------------------------------------------------------------"
mkdir -p rootfs
#lets move there, providing absolute path as specified in instructions
cd "${OUTPUT_DIR}/rootfs" 
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

echo "BUSYBOX INSTALL-----------------------------------------------------------------------------------------------------------"
cd "$OUTPUT_DIR"
if [ ! -d "${OUTPUT_DIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BB_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make distclean
make defconfig
make -j4 ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX}
make -j4 CONFIG_PREFIX=${OUTPUT_DIR}/rootfs ARCH=${CPU_ARCH} CROSS_COMPILE=${TOOLCHAIN_PREFIX} install 
cd ${OUTPUT_DIR}/rootfs

echo "Library dependencies"
echo "Library dependencies-----------------------------------------------------------------------------------------------------------"
${TOOLCHAIN_PREFIX}readelf -a bin/busybox | grep "program interpreter"
${TOOLCHAIN_PREFIX}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "COPYING dependencies-----------------------------------------------------------------------------------------------------------"
cp ${COMPILER_SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTPUT_DIR}/rootfs/lib
cp ${COMPILER_SYSROOT}/lib64/libm.so.6 ${OUTPUT_DIR}/rootfs/lib64
cp ${COMPILER_SYSROOT}/lib64/libresolv.so.2 ${OUTPUT_DIR}/rootfs/lib64
cp ${COMPILER_SYSROOT}/lib64/libc.so.6 ${OUTPUT_DIR}/rootfs/lib64

# Appears the runner wasn't able to find these files, trying this approach as seen on the forums
#find / -type f -name "ld-linux-aarch64.so.1" -exec cp {} ${OUTPUT_DIR}/rootfs/lib/ \;
#find / -type f -name "libm.so.6" -exec cp {} ${OUTPUT_DIR}/rootfs/lib64/ \;
#find / -type f -name "libresolv.so.2" -exec cp {} ${OUTPUT_DIR}/rootfs/lib64/ \;
#find / -type f -name "libc.so.6" -exec cp {} ${OUTPUT_DIR}/rootfs/lib64/ \;

# TODO: Make device nodes
echo "DEVICE NODES-----------------------------------------------------------------------------------------------------------"
cd ${OUTPUT_DIR}/rootfs
sudo mknod -m 666 ${OUTPUT_DIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTPUT_DIR}/rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
#Moving to that repo
echo "WRITER UTIL-----------------------------------------------------------------------------------------------------------"
cd ${SCRIPT_DIR}
make clean
make CROSS_COMPILE=${TOOLCHAIN_PREFIX}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "COPYING DIRS-----------------------------------------------------------------------------------------------------------"
mkdir -p ${OUTPUT_DIR}/rootfs/home/conf
cp autorun-qemu.sh finder.sh finder-test.sh writer "${OUTPUT_DIR}/rootfs/home"
cp conf/* "${OUTPUT_DIR}/rootfs/home/conf"

# TODO: Chown the root directory
echo "CHOWN AND CREATE ROOT-----------------------------------------------------------------------------------------------------------"
cd "${OUTPUT_DIR}/rootfs"
#recursively change the owner of all files in the current directory to root:root
sudo chown -R root:root *

#Create a cpio file using an owner of root:root
find . | cpio -H newc -ov --owner root:root > ${OUTPUT_DIR}/initramfs.cpio

# TODO: Create initramfs.cpio.gz
echo "CREATING TAR-----------------------------------------------------------------------------------------------------------"
cd ${OUTPUT_DIR}
gzip -f initramfs.cpio