#!/bin/sh
#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# dev-cleanup.sh
#
# Remove any intermediate files (e.g. object files) from the development
# directories.
#
. `dirname $0`/../core/ndk-common.sh

DIR=$ANDROID_NDK_ROOT

if [ -f $DIR/RELEASE/TXT ]; then
    echo "ERROR: You cannot run this script in a release directory !"
    exit 1
fi
if [ ! -d $DIR/.git ] ; then
    echo "ERROR: You must call this script in a development directory !"
    exit 1
fi

rm -rf $DIR/build/platforms
rm -rf $DIR/platforms
rm -rf $DIR/samples
rm -rf $DIR/apps
rm -rf $DIR/out

DIR=`dirname $ANDROID_NDK_ROOT`/development/ndk
if [ ! -d $DIR ] ; then
    echo "WARNING: Development directory missing: $DIR"
    exit 0
fi

clean_dir ()
{
    if [ -d "$1" ] ; then
        echo "rm -rf $1"
        rm -rf $1
    fi
}

clean_file ()
{
    if [ -f "$1" ] ; then
        echo "rm -f $1"
        rm -f $1
    fi
}

cleanup_project ()
{
    clean_dir $1/obj
    clean_dir  $1/libs
    clean_dir  $1/bin
    clean_dir  $1/gen
    clean_file $1/build.xml
    clean_file $1/local.properties
}

for PROJECT in $DIR/samples/*; do
    cleanup_project $PROJECT
done
for PROJECT in $DIR/platforms/android-*/samples/*; do
    cleanup_project $PROJECT
done
