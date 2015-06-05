#!/bin/bash
set -e

source $ANDROID_BUILD_TOP/build/envsetup.sh
cd "$( dirname "${BASH_SOURCE[0]}" )"

OUTDIR=$ANDROID_BUILD_TOP/out/ndk
PKGDIR=$OUTDIR/libgabi++
rm -rf $PKGDIR
mkdir -p $PKGDIR

function copy_lib {
  arch=$1
  libdir=$2
  triple=$3

  product="generic_$arch"
  if [ "$arch" == "arm" ]; then
    product="generic"
  fi

  product_dir=$ANDROID_BUILD_TOP/out/target/product/$product
  dest=$PKGDIR/$triple/lib

  mkdir -p $dest

  src=$product_dir/system/$libdir/libgabi++_shared.so
  cp $src $dest

  src=$product_dir/obj/STATIC_LIBRARIES/libgabi++_static_intermediates/libgabi++_static.a
  cp $src $dest
}

echo "Copying libs..."
copy_lib armv5 lib armv5te-linux-androideabi
copy_lib arm lib armv7-linux-androideabi
copy_lib arm64 lib64 aarch64-linux-android

copy_lib mips lib mipsel-linux-android
copy_lib mips64 lib64 mips64el-linux-android

copy_lib x86 lib i686-linux-android
copy_lib x86_64 lib64 x86_64-linux-android

echo "Copying headers..."
cp -rL include $PKGDIR

echo "Creating archive..."

tar caf $OUTDIR/libgabi++.tar.bz2 -C $OUTDIR libgabi++
echo "Created at $OUTDIR/libgabi++.tar.bz2"
