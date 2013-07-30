#!/usr/bin/env python

r'''
 Copyright (C) 2010 The Android Open Source Project
 Copyright (C) 2012 Ray Donnelly <mingw.android@gmail.com>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.


 This wrapper script is used to launch a native debugging session
 on a given NDK application. The application must be debuggable, i.e.
 its android:debuggable attribute must be set to 'true' in the
 <application> element of its manifest.

 See docs/NDK-GDB.TXT for usage description. Essentially, you just
 need to launch ndk-gdb-py from your application project directory
 after doing ndk-build && ant debug && \
  adb install && <start-application-on-device>
'''

import sys, os, argparse, subprocess, types
import xml.etree.cElementTree as ElementTree
import shutil, time
from threading import Thread
try:
    from Queue import Queue, Empty
except ImportError:
    from queue import Queue, Empty  # python 3.x

def find_program(program, extra_paths = []):
    ''' extra_paths are searched before PATH '''
    PATHS = extra_paths+os.environ['PATH'].split(os.pathsep)
    exts = ['']
    if sys.platform.startswith('win'):
        exts += ['.exe', '.bat', '.cmd']
    for path in PATHS:
        if os.path.isdir(path):
            for ext in exts:
                full = path + os.sep + program + ext
                if os.path.isfile(full):
                    return True, full
    return False, None

# Return the prebuilt bin path for the host os.
def ndk_bin_path(ndk):
    if sys.platform.startswith('linux'):
        return ndk+os.sep+'prebuilt/linux-x86/bin'
    elif sys.platform.startswith('darwin'):
        return ndk+os.sep+'prebuilt/darwin-x86/bin'
    elif sys.platform.startswith('win'):
        return ndk+os.sep+'prebuilt/windows/bin'
    return ndk+os.sep+'prebuilt/UNKNOWN/bin'

VERBOSE = False
PROJECT = None
ADB_CMD = None
GNUMAKE_CMD = None
JDB_CMD = None
# Extra arguments passed to the NDK build system when
# querying it.
GNUMAKE_FLAGS = []

OPTION_FORCE = None
OPTION_EXEC = None
OPTION_START = None
OPTION_LAUNCH = None
OPTION_LAUNCH_LIST = None
OPTION_TUI = None
OPTION_WAIT = ['-D']
OPTION_STDCXXPYPR = None

PYPRPR_BASE = sys.prefix + '/share/pretty-printers/'
PYPRPR_GNUSTDCXX_BASE = PYPRPR_BASE + 'libstdcxx/'

DEBUG_PORT = 5039
JDB_PORT = 65534

# Name of the manifest file
MANIFEST = 'AndroidManifest.xml'

# Delay in seconds between launching the activity and attaching gdbserver on it.
# This is needed because there is no way to know when the activity has really
# started, and sometimes this takes a few seconds.
#
DELAY = 2.0
NDK = os.path.abspath(os.path.dirname(sys.argv[0])).replace('\\','/')
DEVICE_SERIAL = ''
ADB_FLAGS = ''

def log(string):
    global VERBOSE
    if VERBOSE:
        print(string)

def error(string, errcode=1):
    print('ERROR: %s' % (string))
    exit(errcode)

def handle_args():
    global VERBOSE, DEBUG_PORT, DELAY, DEVICE_SERIAL
    global GNUMAKE_CMD, GNUMAKE_FLAGS
    global ADB_CMD, ADB_FLAGS
    global JDB_CMD
    global PROJECT, NDK
    global OPTION_START, OPTION_LAUNCH, OPTION_LAUNCH_LIST
    global OPTION_FORCE, OPTION_EXEC, OPTION_TUI, OPTION_WAIT
    global OPTION_STDCXXPYPR
    global PYPRPR_GNUSTDCXX_BASE

    parser = argparse.ArgumentParser(description='''
Setup a gdb debugging session for your Android NDK application.
Read ''' + NDK + '''/docs/NDK-GDB.html for complete usage instructions.''',
    formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument( '--verbose',
                         help='Enable verbose mode', action='store_true', dest='verbose')

    parser.add_argument( '--force',
                         help='Kill existing debug session if it exists',
                         action='store_true')

    parser.add_argument( '--start',
                         help='Launch application instead of attaching to existing one',
                         action='store_true')

    parser.add_argument( '--launch',
                         help='Same as --start, but specify activity name (see below)',
                         dest='launch_name', nargs=1)

    parser.add_argument( '--launch-list',
                         help='List all launchable activity names from manifest',
                         action='store_true')

    parser.add_argument( '--delay',
                         help='Delay in seconds between activity start and gdbserver attach',
                         type=float, default=DELAY,
                         dest='delay')

    parser.add_argument( '-p', '--project',
                         help='Specify application project path',
                         dest='project')

    parser.add_argument( '--port',
                         help='Use tcp:localhost:<DEBUG_PORT> to communicate with gdbserver',
                         type=int, default=DEBUG_PORT,
                         dest='debug_port')

    parser.add_argument( '-x', '--exec',
                         help='Execute gdb initialization commands in <EXEC_FILE> after connection',
                         dest='exec_file')

    parser.add_argument( '--adb',
                         help='Use specific adb command',
                         dest='adb_cmd')

    parser.add_argument( '--awk',
                         help='Use specific awk command (unused flag retained for compatability)')

    parser.add_argument( '-e',
                         help='Connect to single emulator instance....(either this,)',
                         action='store_true', dest='emulator')

    parser.add_argument( '-d',
                         help='Connect to single target device........(this,)',
                         action='store_true', dest='device')

    parser.add_argument( '-s',
                         help='Connect to specific emulator or device.(or this)',
                         default=DEVICE_SERIAL,
                         dest='device_serial')

    parser.add_argument( '-t','--tui',
                         help='Use tui mode',
                         action='store_true', dest='tui')

    parser.add_argument( '--gnumake-flag',
                         help='Flag to pass to gnumake, e.g. NDK_TOOLCHAIN_VERSION=4.8',
                         action='append', dest='gnumake_flags')

    parser.add_argument( '--nowait',
                         help='Do not wait for debugger to attach (may miss early JNI breakpoints)',
                         action='store_true', dest='nowait')

    stdcxx_pypr_versions = [ 'gnustdcxx'+d.replace('gcc','')
                             for d in os.listdir(PYPRPR_GNUSTDCXX_BASE)
                             if os.path.isdir(os.path.join(PYPRPR_GNUSTDCXX_BASE, d)) ]

    parser.add_argument( '--stdcxx-py-pr',
                         help='Specify stdcxx python pretty-printer',
                         choices=['auto', 'none', 'gnustdcxx'] + stdcxx_pypr_versions + ['stlport'],
                         default='none', dest='stdcxxpypr')

    args = parser.parse_args()

    VERBOSE = args.verbose

    ndk_bin = ndk_bin_path(NDK)
    (found_adb,     ADB_CMD)     = find_program('adb',    [ndk_bin])
    (found_gnumake, GNUMAKE_CMD) = find_program('make',   [ndk_bin])
    (found_jdb,     JDB_CMD)     = find_program('jdb',    [])

    if not found_gnumake:
        error('Failed to find GNU make')

    log('Android NDK installation path: %s' % (NDK))

    if args.device:
        ADB_FLAGS = '-d'
    if args.emulator:
        if ADB_FLAGS != '':
            parser.print_help()
            exit(1)
        ADB_FLAGS = '-e'
    if args.device_serial != '':
        DEVICE_SERIAL = args.device_serial
        if ADB_FLAGS != '':
            parser.print_help()
            exit(1)
        ADB_FLAGS = '-s'
    if args.adb_cmd != None:
        log('Using specific adb command: %s' % (args.adb_cmd))
        ADB_CMD = args.adb_cmd
    if ADB_CMD is None:
        error('''The 'adb' tool is not in your path.
       You can change your PATH variable, or use
       --adb=<executable> to point to a valid one.''')
    if not os.path.isfile(ADB_CMD):
        error('Could not run ADB with: %s' % (ADB_CMD))

    if args.project != None:
        PROJECT = args.project

    if args.start != None:
        OPTION_START = args.start

    if args.launch_name != None:
        OPTION_LAUNCH = args.launch_name

    if args.launch_list != None:
        OPTION_LAUNCH_LIST = args.launch_list

    if args.force != None:
        OPTION_FORCE = args.force

    if args.exec_file != None:
        OPTION_EXEC = args.exec_file

    if args.tui != False:
        OPTION_TUI = True

    if args.delay != None:
        DELAY = args.delay

    if args.gnumake_flags != None:
        GNUMAKE_FLAGS = args.gnumake_flags

    if args.nowait == True:
        OPTION_WAIT = []
    elif not found_jdb:
        error('Failed to find jdb.\n..you can use --nowait to disable jdb\n..but may miss early breakpoints.')

    OPTION_STDCXXPYPR = args.stdcxxpypr

def get_build_var(var):
    global GNUMAKE_CMD, GNUMAKE_FLAGS, NDK, PROJECT
    text = subprocess.check_output([GNUMAKE_CMD,
                                  '--no-print-dir',
                                  '-f',
                                  NDK+'/build/core/build-local.mk',
                                  '-C',
                                  PROJECT,
                                  'DUMP_'+var] + GNUMAKE_FLAGS
                                  )
    # replace('\r', '') due to Windows crlf (\r\n)
    #  ...universal_newlines=True causes bytes to be returned
    #     rather than a str
    return text.decode('ascii').replace('\r', '').splitlines()[0]

def get_build_var_for_abi(var, abi):
    global GNUMAKE_CMD, GNUMAKE_FLAGS, NDK, PROJECT
    text = subprocess.check_output([GNUMAKE_CMD,
                                   '--no-print-dir',
                                   '-f',
                                   NDK+'/build/core/build-local.mk',
                                   '-C',
                                   PROJECT,
                                   'DUMP_'+var,
                                   'APP_ABI='+abi] + GNUMAKE_FLAGS,
                                   )
    return text.decode('ascii').replace('\r', '').splitlines()[0]

# Silent if gdb is running in tui mode to keep things tidy.
def output_gdbserver(text):
    if not OPTION_TUI or OPTION_TUI != 'running':
        print(text)

# Likewise, silent in tui mode (also prepends 'JDB :: ')
def output_jdb(text):
    if not OPTION_TUI or OPTION_TUI != 'running':
        print('JDB :: %s' % text)

def input_jdb(inhandle):
    while True:
        inhandle.write('\n')
        time.sleep(1.0)

def background_spawn(args, redirect_stderr, output_fn, redirect_stdin = None, input_fn = None):

    def async_stdout(outhandle, queue, output_fn):
        for line in iter(outhandle.readline, b''):
            output_fn(line.replace('\r', '').replace('\n', ''))
        outhandle.close()

    def async_stderr(outhandle, queue, output_fn):
        for line in iter(outhandle.readline, b''):
            output_fn(line.replace('\r', '').replace('\n', ''))
        outhandle.close()

    def async_stdin(inhandle, queue, input_fn):
        input_fn(inhandle)
        inhandle.close()

    if redirect_stderr:
        used_stderr = subprocess.PIPE
    else:
        used_stderr = subprocess.STDOUT
    if redirect_stdin:
        used_stdin = subprocess.PIPE
    else:
        used_stdin = None
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=used_stderr, stdin=used_stdin,
                         bufsize=1, close_fds='posix' in sys.builtin_module_names)
    qo = Queue()
    to = Thread(target=async_stdout, args=(p.stdout, qo, output_fn))
    to.daemon = True
    to.start()
    if redirect_stderr:
        te = Thread(target=async_stderr, args=(p.stderr, qo, output_fn))
        te.daemon = True
        te.start()
    if redirect_stdin:
        ti = Thread(target=async_stdin, args=(p.stdin, qo, input_fn))
        ti.daemon = True
        ti.start()

def adb_cmd(redirect_stderr, args, log_command=False, adb_trace=False, background=False):
    global ADB_CMD, ADB_FLAGS, DEVICE_SERIAL
    fullargs = [ADB_CMD]
    if ADB_FLAGS != '':
        fullargs += [ADB_FLAGS]
    if DEVICE_SERIAL != '':
        fullargs += [DEVICE_SERIAL]
    if isinstance(args, str):
        fullargs.append(args)
    else:
        fullargs += [arg for arg in args]
    new_env = os.environ.copy()
    retval = 0
    if adb_trace:
        new_env["ADB_TRACE"] = "1"
    if background:
        if log_command:
            log('## COMMAND: adb_cmd %s [BACKGROUND]' % (' '.join(args)))
        background_spawn(fullargs, redirect_stderr, output_gdbserver)
        return 0, ''
    else:
        if log_command:
            log('## COMMAND: adb_cmd %s' % (' '.join(args)))
        try:
            if redirect_stderr:
                text = subprocess.check_output(fullargs,
                                               stderr=subprocess.STDOUT,
                                               env=new_env
                                               )
            else:
                text = subprocess.check_output(fullargs,
                                               env=new_env
                                               )
        except subprocess.CalledProcessError as e:
            retval = e.returncode
            text = e.output
        # rstrip() because of final newline.
        return retval, text.decode('ascii').replace('\r', '').rstrip()

def _adb_var_shell(args, redirect_stderr=False, log_command=True):
    if log_command:
        log('## COMMAND: adb_cmd shell %s' % (' '.join(args)))
    arg_str = str(' '.join(args)+' ; echo $?')
    adb_ret,output = adb_cmd(redirect_stderr=redirect_stderr,
                           args=['shell', arg_str], log_command=False)
    output = output.splitlines()
    retcode = int(output.pop())
    return retcode,'\n'.join(output)

def adb_var_shell(args, log_command=False):
    return _adb_var_shell(args, redirect_stderr=False, log_command=log_command)

def adb_var_shell2(args, log_command=False):
    return _adb_var_shell(args, redirect_stderr=True, log_command=log_command)

# Return the PID of a given package or program, or 0 if it doesn't run
# $1: Package name ("com.example.hellojni") or program name ("/lib/gdbserver")
# Out: PID number, or 0 if not running
#
def get_pid_of(package_name):
    '''
    Some custom ROMs use busybox instead of toolbox for ps.
    Without -w, busybox truncates the output, and very long
    package names like com.exampleisverylongtoolongbyfar.plasma
    exceed the limit.
    '''
    ps_command = 'ps'
    retcode,output = adb_cmd(False, ['shell', 'readlink $(which ps)'])
    if output:
        output = output.replace('\r', '').splitlines()[0]
    if output == 'busybox':
        ps_command = 'ps -w'
    retcode,output = adb_cmd(False,['shell', ps_command])
    output = output.replace('\r', '').splitlines()
    columns = output.pop(0).split()
    try:
        PID_column = columns.index('PID')
    except:
        PID_column = 1
    while output:
        columns = output.pop().split()
        if columns.pop() == package_name:
            return 0,int(columns[PID_column])
    return 1,0

def extract_package_name(xmlfile):
    '''
    The name itself is the value of the 'package' attribute in the
    'manifest' element.
    '''
    tree = ElementTree.ElementTree(file=xmlfile)
    root = tree.getroot()
    if 'package' in root.attrib:
        return root.attrib['package']
    return None

def extract_debuggable(xmlfile):
    '''
    simply extract the 'android:debuggable' attribute value from
    the first <manifest><application> element we find.
    '''
    tree = ElementTree.ElementTree(file=xmlfile)
    root = tree.getroot()
    for application in root.iter('application'):
        for k in application.attrib.keys():
            if str(k).endswith('debuggable'):
                return application.attrib[k] == 'true'
    return False

def extract_launchable(xmlfile):
    '''
    A given application can have several activities, and each activity
    can have several intent filters. We want to only list, in the final
    output, the activities which have a intent-filter that contains the
    following elements:

      <action android:name="android.intent.action.MAIN" />
      <category android:name="android.intent.category.LAUNCHER" />
    '''
    tree = ElementTree.ElementTree(file=xmlfile)
    root = tree.getroot()
    launchable_activities = []
    for application in root.iter('application'):
        for activity in application.iter('activity'):
            for intent_filter in activity.iter('intent-filter'):
                found_action_MAIN = False
                found_category_LAUNCHER = False
                for child in intent_filter:
                    if child.tag == 'action':
                        if True in [str(child.attrib[k]).endswith('MAIN') for k in child.attrib.keys()]:
                            found_action_MAIN = True
                    if child.tag == 'category':
                        if True in [str(child.attrib[k]).endswith('LAUNCHER') for k in child.attrib.keys()]:
                            found_category_LAUNCHER = True
                if found_action_MAIN and found_category_LAUNCHER:
                    names = [str(activity.attrib[k]) for k in activity.attrib.keys() if str(k).endswith('name')]
                    for name in names:
                        if name[0] != '.':
                            name = '.'+name
                        launchable_activities.append(name)
    return launchable_activities

def main():
    global ADB_CMD, NDK, PROJECT
    global JDB_CMD
    global OPTION_START, OPTION_LAUNCH, OPTION_LAUNCH_LIST
    global OPTION_FORCE, OPTION_EXEC, OPTION_TUI, OPTION_WAIT
    global OPTION_STDCXXPYPR
    global PYPRPR_BASE, PYPRPR_GNUSTDCXX_BASE

    if NDK.find(' ')!=-1:
        error('NDK path cannot contain space')
    handle_args()
    if OPTION_EXEC:
        if not os.path.isfile(OPTION_EXEC):
            error('Invalid initialization file: %s' % (OPTION_EXEC))
    ADB_VERSION = subprocess.check_output([ADB_CMD, 'version'],
                                        ).decode('ascii').replace('\r', '').splitlines()[0]
    log('ADB version found: %s' % (ADB_VERSION))
    if DEVICE_SERIAL == '':
        log('Using ADB flags: %s' % (ADB_FLAGS))
    else:
        log('Using ADB flags: %s "%s"' % (ADB_FLAGS,DEVICE_SERIAL))
    if PROJECT != None:
        log('Using specified project path: %s' % (PROJECT))
        if not os.path.isdir(PROJECT):
            error('Your --project option does not point to a directory!')
        if not os.path.isfile(PROJECT+os.sep+MANIFEST):
            error('''Your --project does not point to an Android project path!
       It is missing a %s file.''' % (MANIFEST))
    else:
        # Assume we are in the project directory
        if os.path.isfile(MANIFEST):
            PROJECT = '.'
        else:
            PROJECT = ''
            CURDIR = os.getcwd()

            while CURDIR != os.path.dirname(CURDIR):
                if os.path.isfile(CURDIR+os.sep+MANIFEST):
                    PROJECT=CURDIR
                    break
                CURDIR = os.path.dirname(CURDIR)

            if not os.path.isdir(PROJECT):
                error('Launch this script from an application project directory, or use --project=<path>.')
        log('Using auto-detected project path: %s' % (PROJECT))

    PACKAGE_NAME = extract_package_name(PROJECT+os.sep+MANIFEST)
    if PACKAGE_NAME is None:
        PACKAGE_NAME = '<none>'
    log('Found package name: %s' % (PACKAGE_NAME))
    if PACKAGE_NAME == '<none>':
        error('''Could not extract package name from %s.
       Please check that the file is well-formed!''' % (PROJECT+os.sep+MANIFEST))
    if OPTION_LAUNCH_LIST:
        log('Extracting list of launchable activities from manifest:')
        print(' '.join(extract_launchable(PROJECT+os.sep+MANIFEST)))
        exit(0)
    APP_ABIS = get_build_var('APP_ABI').split(' ')
    if 'all' in APP_ABIS:
        ALL_ABIS = get_build_var('NDK_ALL_ABIS').split(' ')
        APP_ABIS = APP_ABIS[:APP_ABIS.index('all')]+ALL_ABIS+APP_ABIS[APP_ABIS.index('all')+1:]
    log('ABIs targetted by application: %s' % (' '.join(APP_ABIS)))

    retcode,ADB_TEST = adb_cmd(True,['shell', 'ls'])
    if retcode != 0:
        print(ADB_TEST)
        error('''Could not connect to device or emulator!
       Please check that an emulator is running or a device is connected
       through USB to this machine. You can use -e, -d and -s <serial>
       in case of multiple ones.''')

    retcode,API_LEVEL = adb_var_shell(['getprop', 'ro.build.version.sdk'])
    if retcode != 0 or API_LEVEL == '':
        error('''Could not find target device's supported API level!
ndk-gdb will only work if your device is running Android 2.2 or higher.''')
    API_LEVEL = int(API_LEVEL)
    log('Device API Level: %d' % (API_LEVEL))
    if API_LEVEL < 8:
        error('''ndk-gdb requires a target device running Android 2.2 (API level 8) or higher.
The target device is running API level %d!''' % (API_LEVEL))
    COMPAT_ABI = []
    _,CPU_ABI1 = adb_var_shell(['getprop', 'ro.product.cpu.abi'])
    _,CPU_ABI2 = adb_var_shell(['getprop', 'ro.product.cpu.abi2'])
    # Both CPU_ABI1 and CPU_ABI2 may contain multiple comma-delimited abis.
    # Concatanate CPU_ABI1 and CPU_ABI2.
    CPU_ABIS = CPU_ABI1.split(',')+CPU_ABI2.split(',')
    log('Device CPU ABIs: %s' % (' '.join(CPU_ABIS)))
    COMPAT_ABI = [ABI for ABI in CPU_ABIS if ABI in APP_ABIS]

    if not len(COMPAT_ABI):
        error('''The device does not support the application's targetted CPU ABIs!
       Device supports:  %s
       Package supports: %s''' % (' '.join(CPU_ABIS),' '.join(APP_ABIS)))
    COMPAT_ABI = COMPAT_ABI[0]
    log('Compatible device ABI: %s' % (COMPAT_ABI))
    GDBSETUP_INIT = get_build_var_for_abi('NDK_APP_GDBSETUP', COMPAT_ABI)
    log('Using gdb setup init: %s' % (GDBSETUP_INIT))

    TOOLCHAIN_PREFIX = get_build_var_for_abi('TOOLCHAIN_PREFIX', COMPAT_ABI)
    log('Using toolchain prefix: %s' % (TOOLCHAIN_PREFIX))

    APP_OUT = get_build_var_for_abi('TARGET_OUT', COMPAT_ABI)
    log('Using app out directory: %s' % (APP_OUT))
    DEBUGGABLE = extract_debuggable(PROJECT+os.sep+MANIFEST)
    log('Found debuggable flag: %s' % ('true' if DEBUGGABLE==True else 'false'))
    # If gdbserver exists, then we built with 'ndk-build NDK_DEBUG=1' and it's
    # ok to not have android:debuggable set to true in the original manifest.
    # However, if this is not the case, then complain!!
    #
    gdbserver_path = os.path.join(PROJECT,'libs',COMPAT_ABI,'gdbserver')
    if not DEBUGGABLE:
        if os.path.isfile(gdbserver_path):
            log('Found gdbserver under libs/%s, assuming app was built with NDK_DEBUG=1' % (COMPAT_ABI))
        else:
            error('''Package %s is not debuggable ! You can fix that in two ways:

  - Rebuilt with the NDK_DEBUG=1 option when calling 'ndk-build'.

  - Modify your manifest to set android:debuggable attribute to "true",
    then rebuild normally.

After one of these, re-install to the device!''' % (PACKAGE_NAME))
    elif not os.path.isfile(gdbserver_path):
        error('''Could not find gdbserver binary under %s/libs/%s
       This usually means you modified your AndroidManifest.xml to set
       the android:debuggable flag to 'true' but did not rebuild the
       native binaries. Please call 'ndk-build' to do so,
       *then* re-install to the device!''' % (PROJECT,COMPAT_ABI))

    # Let's check that 'gdbserver' is properly installed on the device too. If this
    # is not the case, the user didn't install the proper package after rebuilding.
    #
    retcode,DEVICE_GDBSERVER = adb_var_shell2(['ls', '/data/data/%s/lib/gdbserver' % (PACKAGE_NAME)])
    if retcode:
        error('''Non-debuggable application installed on the target device.
       Please re-install the debuggable version!''')
    log('Found device gdbserver: %s' % (DEVICE_GDBSERVER))

    # Find the <dataDir> of the package on the device
    retcode,DATA_DIR = adb_var_shell2(['run-as', PACKAGE_NAME, '/system/bin/sh', '-c', 'pwd'])
    if retcode or DATA_DIR == '':
        error('''Could not extract package's data directory. Are you sure that
       your installed application is debuggable?''')
    log("Found data directory: '%s'" % (DATA_DIR))

    # Launch the activity if needed
    if OPTION_START:
        if not OPTION_LAUNCH:
            OPTION_LAUNCH = extract_launchable(PROJECT+os.sep+MANIFEST)
            if not len(OPTION_LAUNCH):
                error('''Could not extract name of launchable activity from manifest!
           Try to use --launch=<name> directly instead as a work-around.''')
            log('Found first launchable activity: %s' % (OPTION_LAUNCH[0]))
        if not len(OPTION_LAUNCH):
            error('''It seems that your Application does not have any launchable activity!
       Please fix your manifest file and rebuild/re-install your application.''')

    if OPTION_LAUNCH:
        log('Launching activity: %s/%s' % (PACKAGE_NAME,OPTION_LAUNCH[0]))
        retcode,LAUNCH_OUTPUT=adb_cmd(True,
                                      ['shell', 'am', 'start'] + OPTION_WAIT + ['-n', '%s/%s' % (PACKAGE_NAME,OPTION_LAUNCH[0])],
                                      log_command=True)
        if retcode:
            error('''Could not launch specified activity: %s
       Use --launch-list to dump a list of valid values.''' % (OPTION_LAUNCH[0]))

        # Sleep a bit, it sometimes take one second to start properly
        # Note that we use the 'sleep' command on the device here.
        #
        adb_cmd(True, ['shell', 'sleep', '%f' % (DELAY)], log_command=True)

    # Find the PID of the application being run
    retcode,PID = get_pid_of(PACKAGE_NAME)
    log('Found running PID: %d' % (PID))
    if retcode or PID == 0:
        if OPTION_LAUNCH:
            error('''Could not extract PID of application on device/emulator.
       Weird, this probably means one of these:

         - The installed package does not match your current manifest.
         - The application process was terminated.

       Try using the --verbose option and look at its output for details.''')
        else:
            error('''Could not extract PID of application on device/emulator.
       Are you sure the application is already started?
       Consider using --start or --launch=<name> if not.''')

    # Check that there is no other instance of gdbserver running
    retcode,GDBSERVER_PID = get_pid_of('lib/gdbserver')
    if not retcode and not GDBSERVER_PID == 0:
        if not OPTION_FORCE:
            error('Another debug session running, Use --force to kill it.')
        log('Killing existing debugging session')
        adb_cmd(False, ['shell', 'kill -9 %s' % (GDBSERVER_PID)])

    # Launch gdbserver now
    DEBUG_SOCKET = 'debug-socket'
    adb_cmd(False,
            ['shell', 'run-as', PACKAGE_NAME, 'lib/gdbserver', '+%s' % (DEBUG_SOCKET), '--attach', str(PID)],
            log_command=True, adb_trace=True, background=True)
    log('Launched gdbserver succesfully.')

# Make sure gdbserver was launched - debug check.
#    adb_var_shell(['sleep', '0.1'], log_command=False)
#    retcode,GDBSERVER_PID = get_pid_of('lib/gdbserver')
#    if retcode or GDBSERVER_PID == 0:
#        error('Could not launch gdbserver on the device?')
#    log('Launched gdbserver succesfully (PID=%s)' % (GDBSERVER_PID))

    # Setup network redirection
    log('Setup network redirection')
    retcode,_ = adb_cmd(False,
                        ['forward', 'tcp:%d' % (DEBUG_PORT), 'localfilesystem:%s/%s' % (DATA_DIR,DEBUG_SOCKET)],
                        log_command=True)
    if retcode:
        error('''Could not setup network redirection to gdbserver?
       Maybe using --port=<port> to use a different TCP port might help?''')

    # Get the app_server binary from the device
    APP_PROCESS = '%s/app_process' % (APP_OUT)
    adb_cmd(False, ['pull', '/system/bin/app_process', APP_PROCESS], log_command=True)
    log('Pulled app_process from device/emulator.')

    adb_cmd(False, ['pull', '/system/bin/linker', '%s/linker' % (APP_OUT)], log_command=True)
    log('Pulled linker from device/emulator.')

    adb_cmd(False, ['pull', '/system/lib/libc.so', '%s/libc.so' % (APP_OUT)], log_command=True)
    log('Pulled libc.so from device/emulator.')

    # Setup JDB connection, for --start or --launch
    if (OPTION_START != None or OPTION_LAUNCH != None) and len(OPTION_WAIT):
        log('Set up JDB connection, using jdb command: %s' % JDB_CMD)
        retcode,_ = adb_cmd(False,
                            ['forward', 'tcp:%d' % (JDB_PORT), 'jdwp:%d' % (PID)],
                            log_command=True)
        time.sleep(1.0)
        if retcode:
            error('Could not forward JDB port')
        background_spawn([JDB_CMD,'-connect','com.sun.jdi.SocketAttach:hostname=localhost,port=%d' % (JDB_PORT)], True, output_jdb, True, input_jdb)
        time.sleep(1.0)

    # Work out the python pretty printer details.
    pypr_folder = None
    pypr_function = None

    # Automatic determination of pypr.
    if OPTION_STDCXXPYPR == 'auto':
        libdir = os.path.join(PROJECT,'libs',COMPAT_ABI)
        libs = [ f for f in os.listdir(libdir)
                 if os.path.isfile(os.path.join(libdir, f)) and f.endswith('.so') ]
        if 'libstlport_shared.so' in libs:
            OPTION_STDCXXPYPR = 'stlport'
        elif 'libgnustl_shared.so' in libs:
            OPTION_STDCXXPYPR = 'gnustdcxx'

    if OPTION_STDCXXPYPR == 'stlport':
        pypr_folder = PYPRPR_BASE + 'stlport/gppfs-0.2/stlport'
        pypr_function = 'register_stlport_printers'
    elif OPTION_STDCXXPYPR.startswith('gnustdcxx'):
        if OPTION_STDCXXPYPR == 'gnustdcxx':
            NDK_TOOLCHAIN_VERSION = get_build_var_for_abi('NDK_TOOLCHAIN_VERSION', COMPAT_ABI)
            log('Using toolchain version: %s' % (NDK_TOOLCHAIN_VERSION))
            pypr_folder = PYPRPR_GNUSTDCXX_BASE + 'gcc-' + NDK_TOOLCHAIN_VERSION
        else:
            pypr_folder = PYPRPR_GNUSTDCXX_BASE + OPTION_STDCXXPYPR.replace('gnustdcxx-','gcc-')
        pypr_function = 'register_libstdcxx_printers'

    # Now launch the appropriate gdb client with the right init commands
    #
    GDBCLIENT = '%sgdb' % (TOOLCHAIN_PREFIX)
    GDBSETUP = '%s/gdb.setup' % (APP_OUT)
    shutil.copyfile(GDBSETUP_INIT, GDBSETUP)
    with open(GDBSETUP, "a") as gdbsetup:
        #uncomment the following to debug the remote connection only
        #gdbsetup.write('set debug remote 1\n')
        gdbsetup.write('file '+APP_PROCESS+'\n')
        gdbsetup.write('target remote :%d\n' % (DEBUG_PORT))
        gdbsetup.write('set breakpoint pending on\n')

        if pypr_function:
            gdbsetup.write('python\n')
            gdbsetup.write('import sys\n')
            gdbsetup.write('sys.path.append("%s")\n' % pypr_folder)
            gdbsetup.write('from printers import %s\n' % pypr_function)
            gdbsetup.write('%s(None)\n' % pypr_function)
            gdbsetup.write('end\n')

        if OPTION_EXEC:
            with open(OPTION_EXEC, 'r') as execfile:
                for line in execfile:
                    gdbsetup.write(line)
    gdbsetup.close()

    gdbargs = [GDBCLIENT, '-x', '%s' % (GDBSETUP)]
    if OPTION_TUI:
        gdbhelp = subprocess.check_output([GDBCLIENT, '--help']).decode('ascii')
        try:
            gdbhelp.index('--tui')
            gdbargs.append('--tui')
            OPTION_TUI = 'running'
        except:
            print('Warning: Disabled tui mode as %s does not support it' % (os.path.basename(GDBCLIENT)))
    gdbp = subprocess.Popen(gdbargs)
    while gdbp.returncode is None:
        try:
            gdbp.communicate()
        except KeyboardInterrupt:
            pass
    log("Exited gdb, returncode %d" % gdbp.returncode)

if __name__ == '__main__':
    main()
