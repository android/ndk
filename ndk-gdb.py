#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
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

from __future__ import print_function

import argparse
import multiprocessing
import os
import operator
import subprocess
import sys
import xml.etree.cElementTree as ElementTree

# Shared functions across gdbclient.py and ndk-gdb.py.
import gdbrunner

def log(msg):
    pass


def error(msg):
    sys.exit("ERROR: {}".format(msg))


class ArgumentParser(gdbrunner.ArgumentParser):
    def __init__(self):
        super(ArgumentParser, self).__init__()
        self.add_argument(
            "--verbose", action="store_true",
            help="Enable verbose mode")

        self.add_argument(
            "--force", action="store_true",
            help="Kill existing debug session if it exists")

        self.add_argument(
            "--port", type=int, nargs="?", default="5039",
            help="override the port used on the host")

        self.add_argument(
            "--delay", type=float, default=1.0,
            help="Delay in seconds between activity start and gdbserver attach")

        self.add_argument(
            "-p", "--project", dest="project",
            help="Specify application project path")

        # Unused flag retained for compatibility
        self.add_argument("--awk", help=argparse.SUPPRESS)

        app_group = self.add_argument_group("target selection")
        start_group = app_group.add_mutually_exclusive_group()

        class NoopAction(argparse.Action):
            def __call__(self, *args, **kwargs):
                pass

        # Action for --attach is a noop, because --start's action will store a
        # false in launch if --start isn't specified.
        start_group.add_argument(
            "--attach", action=NoopAction, nargs=0,
            help="Attach to application [default]")

        start_group.add_argument(
            "--start", action="store_true", dest="launch",
            help="Launch application main activity")

        start_group.add_argument(
            "--launch", action="store", metavar="ACTIVITY", dest="launch",
            help="Launch specified application activity")

        start_group.add_argument(
            "--launch-list", action="store_true", dest="launch_list",
            help="List all launchable activity names from manifest")

        debug_group = self.add_argument_group("debugging options")
        debug_group.add_argument(
            "-x", "--exec", dest="exec_file",
            help="Execute gdb commands in EXEC_FILE after connection")

        debug_group.add_argument(
            "--nowait", action="store_true",
            help="Do not wait for debugger to attach (may miss early JNI breakpoints)")

        debug_group.add_argument(
            "-t", "--tui", action="store_true", dest="tui",
            help="Use GDB's tui mode")

        debug_group.add_argument(
            "--stdcxx-py-pr", dest="stdcxxpypr",
            help="Use stdcxx python pretty-printer",
            choices=["auto", "none", "gnustdcxx", "stlport"],
            default="none")


def extract_package_name(xmlroot):
    if "package" in xmlroot.attrib:
        return xmlroot.attrib["package"]
    error("Failed to find package name in AndroidManifest.xml")


ANDROID_XMLNS = "{http://schemas.android.com/apk/res/android}"
def extract_debuggable(xmlroot):
    applications = xmlroot.findall("application")
    if len(applications) > 1:
        error("Multiple application tags found in AndroidManifest.xml")
    debuggable_attrib = "{}debuggable".format(ANDROID_XMLNS)
    if debuggable_attrib in applications[0].attrib:
        debuggable = applications[0].attrib[debuggable_attrib]
        if debuggable == "true":
            return True
        elif debuggable == "false":
            return False
        else:
            msg = "Unexpected android:debuggable value: '{}'"
            error(msg.format(debuggable))
    return False


def extract_launchable(xmlroot):
    '''
    A given application can have several activities, and each activity
    can have several intent filters. We want to only list, in the final
    output, the activities which have a intent-filter that contains the
    following elements:

      <action android:name="android.intent.action.MAIN" />
      <category android:name="android.intent.category.LAUNCHER" />
    '''
    launchable_activities = []
    application = xmlroot.findall("application")[0]

    main_action = "android.intent.action.MAIN"
    launcher_category = "android.intent.category.LAUNCHER"
    name_attrib = "{}name".format(ANDROID_XMLNS)

    for activity in application.iter("activity"):
        if name_attrib not in activity.attrib:
            continue

        for intent_filter in activity.iter("intent-filter"):
            found_action = False
            found_category = False
            for child in intent_filter:
                if child.tag == "action":
                    if not found_action and name_attrib in child.attrib:
                        if child.attrib[name_attrib] == main_action:
                            found_action = True
                if child.tag == "category":
                    if not found_category and name_attrib in child.attrib:
                        if child.attrib[name_attrib] == launcher_category:
                            found_category = True
            if found_action and found_category:
                launchable_activities.append(activity.attrib[name_attrib])
    return launchable_activities


def ndk_path():
    ndk = os.path.abspath(os.path.dirname(sys.argv[0])).replace("\\", "/")
    return ndk


def ndk_bin_path():
    ndk = ndk_path()
    if sys.platform.startswith("linux"):
        platform_name = "linux-x86_64"
    elif sys.platform.startswith("darwin"):
        platform_name = "darwin-x86_64"
    elif sys.platform.startswith("win"):
        # Check both x86 and x86_64.
        platform_name = "windows-x86_64"
        if not os.path.exists(os.path.join(ndk, "prebuilt", platform_name)):
            platform_name = "windows"
    else:
        error("Unknown platform: {}".format(sys.platform))

    path = os.path.join(ndk, "prebuilt", platform_name, "bin")
    if not os.path.exists(path):
        error("Failed to find ndk binary path, should be at '{}'".format(path))

    return path


def handle_args():
    def find_program(program, extra_paths=[]):
        '''Find a binary, searching in extra_paths before $PATH'''
        # FIXME:This is broken for PATH that contains quoted colons.
        PATHS = os.environ["PATH"].replace('"', '').split(os.pathsep)
        PATHS = extra_paths + PATHS
        exts = [""]
        if sys.platform.startswith("win"):
            exts += [".exe", ".bat", ".cmd"]
        for path in PATHS:
            if os.path.isdir(path):
                for ext in exts:
                    full = path + os.sep + program + ext
                    if os.path.isfile(full):
                        return True, full
        return False, None

    args = ArgumentParser().parse_args()
    ndk_bin = ndk_bin_path()
    (found_gnumake, args.make_cmd) = find_program("make", [ndk_bin])
    (found_jdb, args.jdb_cmd) = find_program("jdb", [])

    if not found_gnumake:
        error("Failed to find make in '{}'".format(ndk_bin))
    if not found_jdb:
        print("WARNING: Failed to find jdb on your path, defaulting to "
              "--nowait")
        args.nowait = True

    if args.verbose:
        global log
        log = print

    return args


def find_project(args):
    manifest_name = "AndroidManifest.xml"
    if args.project is not None:
        log("Using project directory: {}".format(args.project))
        args.project = os.path.realpath(args.project)
        if not os.path.exists(os.path.join(args.project, manifest_name)):
            msg = "could not find AndroidManifest.xml in '{}'"
            error(msg.format(args.project))
    else:
        # Walk upwards until we find AndroidManifest.xml, or run out of path.
        current_dir = os.getcwdu()
        while not os.path.exists(os.path.join(current_dir, manifest_name)):
            parent_dir = os.path.dirname(current_dir)
            if parent_dir == current_dir:
                error("Could not find AndroidManifest.xml in current"
                      " directory or a parent directory.\n"
                      "       Launch this script from inside a project, or"
                      " use --project=<path>.")
            current_dir = parent_dir
        args.project = current_dir
        log("Using project directory: {} ".format(args.project))
    args.manifest_path = os.path.join(args.project, manifest_name)
    return args.project


def parse_manifest(args):
    # args.launch is an element that can either be a boolean or String:
    #   False:              attach to existing application
    #   True:               launch the application's main activity
    #   "FileNotFound":     launch the "FileNotFound" activity
    manifest = ElementTree.parse(args.manifest_path)
    manifest_root = manifest.getroot()
    package_name = extract_package_name(manifest_root)
    log("Found package name: {}".format(package_name))

    debuggable = extract_debuggable(manifest_root)
    if not debuggable:
        error("Application is not marked as debuggable in its manifest.")

    def canonicalize_activity(activity_name):
        if activity_name.startswith("."):
            return "{}{}".format(package_name, activity_name)
        return activity_name

    activities = extract_launchable(manifest_root)
    activities = map(canonicalize_activity, activities)

    if args.launch_list:
        print("Launchable activities: {}".format(", ".join(activities)))
        sys.exit(0)

    if args.launch is False:
        log("Attaching to existing application process.")
    else:
        if args.launch is True:
            if len(activities) == 0:
                error("No launchable activities found.")

            args.launch = activities[0]

            if len(activities) > 1:
                print("WARNING: Multiple launchable activities found, choosing"
                      " '{}'.".format(activities[0]))
            else:
                log("Selecting activity '{}'".format(args.launch))
        else:
            activity_name = canonicalize_activity(args.launch)
            if activity_name not in activities:
                msg = "Could not find launchable activity: '{}'."
                error(msg.format(activity_name))
            args.launch = activity_name
            log("Selecting activity '{}'.".format(args.launch))

    return (package_name, args.launch)


def dump_var(args, variable, abi=None):
    make_args = [args.make_cmd, "--no-print-dir", "-f",
                 os.path.join(ndk_path(), "build/core/build-local.mk"),
                 "-C", args.project, "DUMP_{}".format(variable)]

    if abi is not None:
        make_args.append("APP_ABI={}".format(abi))

    saved_pwd = os.environ["PWD"]
    os.environ["PWD"] = args.project
    try:
        make_output = subprocess.check_output(make_args, cwd=args.project)
    except subprocess.CalledProcessError:
        error("Failed to retrieve application ABI from Android.mk.")
    os.environ["PWD"] = saved_pwd
    return make_output.splitlines()[0]


def get_api_level(device_props):
    # Check the device API level
    if "ro.build.version.sdk" not in device_props:
        error("Failed to find target device's supported API level.\n"
              "ndk-gdb only supports devices running Android 2.2 or higher.")
    api_level = int(device_props["ro.build.version.sdk"])
    if api_level < 8:
        error("ndk-gdb only supports devices running Android 2.2 or higher.\n"
              "(expected API level 8, actual: {})".format(api_level))

    return api_level


def fetch_abi(args):
    '''
    Figure out the intersection of which ABIs the application is built for and
    which ones the device supports, then pick the best one, so that we know
    which gdbserver to push and run on the device.
    '''

    app_abis = dump_var(args, "APP_ABI").split(" ")
    if "all" in app_abis:
        app_abis = dump_var(args, "NDK_ALL_ABIS").split(" ")
    app_abis_msg = "Application ABIs: {}".format(", ".join(app_abis))
    log(app_abis_msg)

    device_props = args.device.get_props()

    new_abi_props = ["ro.product.cpu.abilist"]
    old_abi_props = ["ro.product.cpu.abi", "ro.product.cpu.abi2"]
    abi_props = new_abi_props
    if len(set(new_abi_props).intersection(device_props.keys())) == 0:
        abi_props = old_abi_props

    device_abis = [device_props[key].split(",") for key in abi_props]

    # Flatten the list.
    device_abis = reduce(operator.add, device_abis)
    device_abis_msg = "Device ABIs: {}".format(", ".join(device_abis))
    log(device_abis_msg)

    for abi in device_abis:
        if abi in app_abis:
            # TODO(jmgao): Do we expect gdb to work with ARM-x86 translation?
            log("Selecting ABI: {}".format(abi))
            return abi

    msg = "Application cannot run on the selected device."

    # Don't repeat ourselves.
    if not args.verbose:
        msg += "\n{}\n{}".format(app_abis_msg, device_abis_msg)

    error(msg)


def get_app_data_dir(args, package_name):
    cmd = ["/system/bin/sh", "-c", "pwd", "2>/dev/null"]
    cmd = gdbrunner.get_run_as_cmd(package_name, cmd)
    (rc, stdout, _) = args.device.shell_nocheck(cmd)
    if rc != 0:
        error("Could not find application's data directory. Are you sure that "
              "the application is installed and debuggable?")
    data_dir = stdout.strip()
    log("Found application data directory: {}".format(data_dir))
    return data_dir


def get_gdbserver_path(args, package_name, app_data_dir, abi):
    app_gdbserver_path = "{}/lib/gdbserver".format(app_data_dir)
    cmd = ["ls", app_gdbserver_path, "2>/dev/null"]
    cmd = gdbrunner.get_run_as_cmd(package_name, cmd)
    (rc, _, _) = args.device.shell_nocheck(cmd)
    if rc == 0:
        log("Found app gdbserver: {}".format(app_gdbserver_path))
        return app_gdbserver_path

    # We need to upload our gdbserver
    log("App gdbserver not found at {}, uploading.".format(app_gdbserver_path))
    if abi.startswith("armeabi"):
        abi = "arm"
    elif abi == "arm64-v8a":
        abi = "arm64"
    local_path = "{}/prebuilt/android-{}/gdbserver/gdbserver"
    local_path = local_path.format(ndk_path(), abi)
    remote_path = "/data/local/tmp/{}-gdbserver".format(abi)
    args.device.push(local_path, remote_path)

    # Copy gdbserver into the data directory on M+
    if get_api_level(args.props) >= 23:
        destination = "{}/{}-gdbserver".format(app_data_dir, abi)
        log("Copying gdbserver to {}.".format(destination))
        cmd = ["cat", remote_path, "|", "run-as", package_name,
               "sh", "-c", "'cat > {}'".format(destination)]
        (rc, _, _) = args.device.shell_nocheck(cmd)
        if rc != 0:
            error("Failed to copy gdbserver to {}.".format(destination))
        (rc, _, _) = args.device.shell_nocheck(["run-as", package_name,
                                                "chmod", "700", destination])
        if rc != 0:
            error("Failed to chmod gdbserver at {}.".format(destination))

        remote_path = destination

    log("Uploaded gdbserver to {}".format(remote_path))
    return remote_path


def pull_binaries(device, out_dir, is64bit):
    required_files = []
    libraries = ["libc.so", "libm.so", "libcutils.so", "libutils.so",
                 "libandroid_runtime.so", "libc++.so"]

    if is64bit:
        required_files = ["/system/bin/app_process64", "/system/bin/linker64"]
        library_path = "/system/lib64"
    else:
        required_files = ["/system/bin/app_process", "/system/bin/linker"]
        library_path = "/system/lib"

    for library in libraries:
        required_files.append("{}/{}".format(library_path, library))

    for required_file in required_files:
        local_name = "{}{}".format(out_dir, required_file)
        dirname = os.path.dirname(required_file)
        local_dirname = "{}{}".format(out_dir, dirname)
        if not os.path.isdir(local_dirname):
            os.makedirs(local_dirname)
        device.pull(required_file, local_name)


def generate_gdb_script(gdb_setup, sysroot, binary_path, is64bit, port,
                        connect_timeout=5):
    # Generate a gdb script.
    with open(gdb_setup) as f:
        gdb_setup_cmds = f.read()

    gdb_commands = ""
    for line in gdb_setup_cmds.splitlines():
        if not line.startswith("set solib-search-path"):
            gdb_commands += "{}\n".format(line)

    gdb_commands += "file '{}'\n".format(binary_path)

    solib_search_path = [sysroot, "{}/system/bin".format(sysroot)]
    if is64bit:
        solib_search_path.append("{}/system/lib64".format(sysroot))
    else:
        solib_search_path.append("{}/system/lib".format(sysroot))
    solib_search_path = os.pathsep.join(solib_search_path)
    gdb_commands += "set solib-absolute-prefix {}\n".format(sysroot)
    gdb_commands += "set solib-search-path {}\n".format(solib_search_path)

    for filename in os.listdir(sysroot):
        if filename.endswith(".so"):
            command = "sharedlibrary '{}'\n"
            gdb_commands += command.format(os.path.join(sysroot, filename))

    # Try to connect for a few seconds, sometimes the device gdbserver takes
    # a little bit to come up, especially on emulators.
    gdb_commands += """
python

def target_remote_with_retry(target, timeout_seconds):
  import time
  end_time = time.time() + timeout_seconds
  while True:
    try:
      gdb.execute("target remote " + target)
      return True
    except gdb.error as e:
      time_left = end_time - time.time()
      if time_left < 0 or time_left > timeout_seconds:
        print("Error: unable to connect to device.")
        print(e)
        return False
      time.sleep(min(0.25, time_left))

target_remote_with_retry(':{}', {})

end
""".format(port, connect_timeout)
    return gdb_commands


def main():
    args = handle_args()
    device = args.device

    if device is None:
        error("Could not find a unique connected device/emulator.")

    adb_version = subprocess.check_output(device.adb_cmd + ["version"])
    log("ADB command used: '{}'".format(" ".join(device.adb_cmd)))
    log("ADB version: {}".format(" ".join(adb_version.splitlines())))

    args.props = device.get_props()

    project = find_project(args)
    (pkg_name, launch) = parse_manifest(args)
    abi = fetch_abi(args)

    out_dir = os.path.join(project, (dump_var(args, "TARGET_OUT", abi)))
    out_dir = os.path.realpath(out_dir)

    app_data_dir = get_app_data_dir(args, pkg_name)
    gdbserver_path = get_gdbserver_path(args, pkg_name, app_data_dir, abi)

    # Launch the application if needed, and get its pid
    if launch is not False:
        am_cmd = ["am", "start",]
        if not args.nowait:
            am_cmd.append("-D")
        component_name = "{}/{}".format(pkg_name, launch)
        am_cmd.append(component_name)
        log("Launching application...")
        (rc, _, _) = device.shell_nocheck(am_cmd)
        if rc != 0:
            error("Failed to start {}".format(component_name))

    pids = gdbrunner.get_pids(device, pkg_name)
    if len(pids) == 0:
        error("Failed to find running process '{}'".format(pkg_name))
    if len(pids) > 1:
        error("Multiple running processes named '{}'".format(pkg_name))
    pid = pids[0]

    # Pull the linker, zygote, and notable system libraries
    is64bit = "64" in abi
    pull_binaries(device, out_dir, is64bit)
    if is64bit:
        zygote_path = os.path.join(out_dir, "system", "bin", "app_process64")
    else:
        zygote_path = os.path.join(out_dir, "system", "bin", "app_process")

    # Start gdbserver.
    debug_socket = os.path.join(app_data_dir, "debug_socket")
    log("Starting gdbserver...")
    gdbrunner.start_gdbserver(
        device, None, gdbserver_path,
        target_pid=pid, run_cmd=None, debug_socket=debug_socket,
        port=args.port, user=pkg_name)

    gdb_path = os.path.join(ndk_bin_path(), "gdb")
    gdbsetup_path = os.path.join(project, "libs", abi, "gdb.setup")

    # Start jdb to unblock the application if necessary.
    if launch is not False and not args.nowait:
        # Do this in a thread before starting gdb, since jdb won't connect
        # until gdb connects and continues.
        def jdb_thread():
            log("Starting jdb to unblock application.")
            jdb_port = 65534
            device.forward("tcp:{}".format(jdb_port), "jdwp:{}".format(pid))
            jdb_cmd = [args.jdb_cmd, "-connect",
                       "com.sun.jdi.SocketAttach:hostname=localhost,port={}".format(jdb_port)]

            windows = os.name == "nt"
            flags = subprocess.CREATE_NEW_PROCESS_GROUP if windows else 0
            preexec_fn = None if windows else os.setpgrp
            jdb = subprocess.Popen(jdb_cmd,
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT,
                                   creationflags=flags,
                                   preexec_fn=preexec_fn)

            log("JDB started, telling it to quit...")
            jdb.communicate("exit\n")
            log("JDB exited.")

        jdb_process = multiprocessing.Process(target=jdb_thread)
        jdb_process.start()


    # Start gdb.
    gdb_commands = generate_gdb_script(gdbsetup_path, out_dir, zygote_path,
                                       is64bit, args.port)
    gdb_flags = []
    if args.tui:
        gdb_flags.append("--tui")
    gdbrunner.start_gdb(gdb_path, gdb_commands, gdb_flags)

if __name__ == "__main__":
    main()
