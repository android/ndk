#!/bin/env python

r'''
 Copyright (C) 2013 The Android Open Source Project

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
'''

import sys, os, argparse, subprocess, types
import tempfile, struct

VERBOSE = False
KEEP = False
NDK = os.path.abspath(os.path.dirname(sys.argv[0]))
HOST_TAG = ''
LLVM_VERSION = '3.2'
PLATFORM = None
ABI = None
BITCODE = None
OUTPUT = None

TRANSLATE_CMD = None
LLC_CMD = None
LD_CMD = None

SHARED = True
SONAME = None
DEPEND_LIBS = []
LDFLAGS = []

def log(string):
    global VERBOSE
    if VERBOSE:
        print(string)

def error(string, errcode=1):
    print('ERROR: %s' % (string))
    exit(errcode)

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

# Return the llvm bin path for the host os.
def llvm_bin_path(ndk, host_tag, llvm_ver):
    return ndk+'/toolchains/llvm-'+llvm_ver+'/prebuilt/'+host_tag+'/bin'

# Return the sysroot for arch.
def sysroot_for_arch(arch):
    global NDK, PLATFORM
    sysroot = NDK+'/platforms/'+PLATFORM+'/arch-'+arch
    if os.path.exists(sysroot):
        return sysroot
    else:
        error('sysroot not found: %s' % (sysroot))
    return ''

# Return the arch name for abi
def get_arch_for_abi(abi):
    if abi == 'armeabi' or abi == 'armeabi-v7a':
        return 'arm'
    elif abi == 'x86':
        return 'x86'
    elif abi == 'mips':
        return 'mips'
    error('Unspported abi: %s' % (abi))
    return ''

# Get default llvm triple for abi.
def get_default_triple_for_abi(abi):
    if abi == 'armeabi':
        return 'armv5te-none-linux-androideabi'
    elif abi == 'armeabi-v7a':
        return 'thumbv7-none-linux-androideabi'
    elif abi == 'x86':
        return 'i686-none-linux-android'
    elif abi == 'mips':
        return 'mipsel-none-linux-androideabi'
    error('Unspported abi: %s' % (abi))
    return ''

def gcc_toolchain_for_arch(arch):
    if arch == 'arm':
        return NDK+'/toolchains/arm-linux-androideabi-4.6/prebuilt/'+HOST_TAG
    elif arch == 'x86':
        return NDK+'/toolchains/x86-4.6/prebuilt/'+HOST_TAG
    elif arch == 'mips':
        return NDK+'/toolchains/mipsel-linux-android-4.6/prebuilt/'+HOST_TAG
    return ''

def libgcc_dir_for_abi(abi):
    global NDK, HOST_TAG
    arch = get_arch_for_abi(abi)
    gcc_toolchain = gcc_toolchain_for_arch(arch)
    if abi == 'armeabi':
        return gcc_toolchain+'/lib/gcc/arm-linux-androideabi/4.6/thumb'
    elif abi == 'armeabi-v7a':
        return gcc_toolchain+'/lib/gcc/arm-linux-androideabi/4.6/armv7-a/thumb'
    elif abi == 'x86':
        return gcc_toolchain+'/lib/gcc/i686-linux-android/4.6'
    elif abi == 'mips':
        return gcc_toolchain+'/lib/gcc/mipsel-linux-android/4.6/'
    return ''

def handle_args():
    global BITCODE, OUTPUT, PLATFORM, LLVM_VERSION, ABI, NDK
    global VERBOSE, KEEP

    parser = argparse.ArgumentParser(description='''Transform bitcode to binary tool''')

    parser.add_argument( '--file',
                         nargs=2,
                         required=True,
                         action='append',
                         metavar=('input_bitcode', 'output_file'),
                         help='Specify input bitcode and output filename')

    parser.add_argument( '--platform',
                         help='Specify API level for target binary',
                         default='android-9',
                         dest='platform')

    parser.add_argument( '--ndk-dir',
                         help='Specify the ndk directory',
                         dest='ndk_dir')

    parser.add_argument( '--abi',
                         help='Specify ABI for target binary',
                         default='armeabi',
                         choices=['armeabi', 'armeabi-v7a', 'x86', 'mips'])

    parser.add_argument( '-v', '--verbose',
                         help='Enable verbose mode',
                         action='store_true',
                         dest='verbose')

    parser.add_argument( '--keep',
                         help='Keep the temporary files',
                         action='store_true',
                         dest='keep')

    args = parser.parse_args()
    BITCODE = args.file[0][0]
    OUTPUT = args.file[0][1]

    if os.path.isfile(BITCODE) != True:
        error('Input bitcode %s not found!' % (BITCODE))

    VERBOSE = args.verbose
    KEEP = args.keep
    PLATFORM = args.platform
    ABI = args.abi

    if args.ndk_dir != None:
        NDK = args.ndk_dir
    log('Android NDK installation path: %s' % (NDK))

def locate_tools():
    global HOST_TAG, NDK, LLVM_VERSION
    global TRANSLATE_CMD, LLC_CMD, LD_CMD

    if sys.platform.startswith('linux'):
        HOST_TAG='linux-x86'
    elif sys.platform.startswith('darwin'):
        HOST_TAG='darwin-x86'
    elif sys.platform.startswith('win'):
        HOST_TAG='windows'
    else:
        HOST_TAG='UNKNOWN'

    llvm_bin = llvm_bin_path(NDK, HOST_TAG, LLVM_VERSION)

    (found_translate, TRANSLATE_CMD) = find_program('le32-none-ndk-translate', [llvm_bin])
    if found_translate != True:
        error('Cannot find le32-none-ndk-translate')

    (found_llc,  LLC_CMD)  = find_program('llc', [llvm_bin])
    if found_llc != True:
        error('Cannot find llc')

    (found_ld, LD_CMD) = find_program('ld.mcld', [llvm_bin])
    if found_ld != True:
        error('Cannot find ld.mcld')

def parse_bitcode_type(data):
    type = struct.unpack('<i',data)[0]
    if type != 1:
        return False
    return True

'''
 enum ZOptionEnum {
    kCombReloc     = 1 << 0,  ///< [on] -z combreloc, [off] -z nocombreloc
    kDefs          = 1 << 1,  ///< -z defs
    kExecStack     = 1 << 2,  ///< [on] -z execstack, [off] -z noexecstack
    kInitFirst     = 1 << 3,  ///< -z initfirst
    kInterPose     = 1 << 4,  ///< -z interpose
    kLoadFltr      = 1 << 5,  ///< -z loadfltr
    kMulDefs       = 1 << 6,  ///< -z muldefs
    kNoCopyReloc   = 1 << 7,  ///< -z nocopyreloc
    kNoDefaultLib  = 1 << 8,  ///< -z nodefaultlib
    kNoDelete      = 1 << 9,  ///< -z nodelete
    kNoDLOpen      = 1 << 10, ///< -z nodlopen
    kNoDump        = 1 << 11, ///< -z nodump
    kRelro         = 1 << 12, ///< [on] -z relro, [off] -z norelro
    kLazy          = 1 << 13, ///< [on] -z lazy, [off] -z now
    kOrigin        = 1 << 14, ///< -z origin
    kZOptionMask   = 0xFFFF
  };
'''
def parse_ldflags(data):
    flags = []
    ldflags = struct.unpack('<i',data)[0]
    if ldflags & (1 << 0):
        flags += ['-z'] + ['combreloc']

    if ldflags & (1 << 1):
        flags += ['--no-undefined']

    if ldflags & (1 << 2):
        flags += ['-z']+['execstack']
    else:
        flags += ['-z']+['noexecstack']

    if ldflags & (1 << 3):
        flags += ['-z']+['initfirst']

    if ldflags & (1 << 4):
        flags += ['-z']+['interpose']

    if ldflags & (1 << 5):
        flags += ['-z']+['loadfltr']

    if ldflags & (1 << 6):
        flags += ['-z']+['muldefs']

    if ldflags & (1 << 7):
        flags += ['-z']+['nocopyreloc']

    if ldflags & (1 << 8):
        flags += ['-z']+['nodefaultlib']

    if ldflags & (1 << 9):
        flags += ['-z']+['nodelete']

    if ldflags & (1 << 10):
        flags += ['-z']+['nodlopen']

    if ldflags & (1 << 11):
        flags += ['-z']+['nodump']

    if ldflags & (1 << 12):
        flags += ['-z']+['relro']

    if ldflags & (1 << 13):
        flags += ['-z']+['lazy']
    else:
        flags += ['-z']+['now']

    if ldflags & (1 << 14):
        flags += ['-z']+['origin']
    return flags

'''
  The bitcode wrapper definition:

  struct AndroidBitcodeWrapper {
    uint32_t Magic;
    uint32_t Version;
    uint32_t BitcodeOffset;
    uint32_t BitcodeSize;
    uint32_t HeaderVersion;
    uint32_t TargetAPI;
    uint32_t PNaClVersion;
    uint16_t CompilerVersionTag;
    uint16_t CompilerVersionLen;
    uint32_t CompilerVersion;
    uint16_t OptimizationLevelTag;
    uint16_t OptimizationLevelLen;
    uint32_t OptimizationLevel;
  };
'''
def read_bitcode_wrapper(bitcode):
    global SHARED, SONAME, DEPEND_LIBS, LDFLAGS
    f = open(bitcode, 'rb')
    fixed_field = struct.unpack('<iiiiiii',f.read(4*7))
    magic_number = fixed_field[0]
    if hex(magic_number) != '0xb17c0de':
        error("Invalid bitcode file!")
    offset = fixed_field[2]
    offset -= 4*7
    while offset > 0:
        tag,length = struct.unpack('<hh',f.read(4))
        length = (length+3) & ~3
        data = f.read(length)
        if hex(tag) == '0x5001':
            SHARED = parse_bitcode_type(data)
        elif hex(tag) == '0x5002':
            SONAME = str(data).rstrip('\0')
        elif hex(tag) == '0x5003':
            DEPEND_LIBS += ['-l'+str(data).rstrip('\0')]
        elif hex(tag) == '0x5004':
            LDFLAGS = parse_ldflags(data)
        offset -= (length+4)

def run_cmd(args):
    log(' '.join(args))
    ret = 0
    try:
        text = subprocess.check_output(args, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        ret = e.returncode
        text = e.output
    return ret, text

def do_translate(bitcode, output):
    global TRANSLATE_CMD
    global ABI

    arch = get_arch_for_abi(ABI)
    args = [TRANSLATE_CMD]
    args += ['-arch='+arch]
    args += ['-o']
    args += [output]
    args += [bitcode]

    return run_cmd(args)

def get_llc_flags_for_abi(abi):
    extra_args = []
    if abi == 'x86':
        extra_args += ['-disable-fp-elim']
        # Not all Android x86 devices have these features
        extra_args += ['-mattr="-ssse3,-sse41,-sse42,-sse4a,-popcnt"']
    return extra_args

def do_llc(bitcode, output):
    global LLC_CMD
    global ABI

    triple = get_default_triple_for_abi(ABI)
    args = [LLC_CMD]
    args += ['-mtriple='+triple]
    args += ['-filetype=obj']
    args += ['-relocation-model=pic']
    args += ['-code-model=small']
    args += ['-use-init-array']
    args += ['-mc-relax-all']
    args += ['-float-abi=soft']
    args += get_llc_flags_for_abi(ABI)
    args += ['-O2']
    args += [bitcode]
    args += ['-o']
    args += [output]

    return run_cmd(args)

def do_ld(relocatable, output, shared=True):
    global LD_CMD
    global ABI, PLATFORM
    global SHARED, SONAME, DEPEND_LIBS, LDFLAGS

    arch = get_arch_for_abi(ABI)
    triple = get_default_triple_for_abi(ABI)
    sysroot = sysroot_for_arch(arch)
    # TODO: Change to compiler-rt
    libgcc_dir = libgcc_dir_for_abi(ABI)

    args = [LD_CMD]
    args += ['-mtriple='+triple]
    args += ['--sysroot='+sysroot]
    if SHARED:
        args += ['-shared']
    args += ['-Bsymbolic']
    args += ['-dynamic-linker']
    args += ['/system/bin/linker']
    args += ['-o']
    args += [output]
    args += ['@' + NDK + '/sources/android/libportable/libs/'+ABI+'/libportable.wrap']

    if SHARED:
        args += [sysroot+'/usr/lib/crtbegin_so.o']
    else:
        args += [sysroot+'/usr/lib/crtbegin_dynamic.o']

    args += ['-L'+sysroot+'/usr/lib']
    args += ['-L'+libgcc_dir]
    args += ['-soname']
    args += [SONAME]
    args += LDFLAGS
    args += [relocatable]
    args += [NDK+'/sources/android/libportable/libs/'+ABI+'/libportable.a']
    args += DEPEND_LIBS

    if SHARED:
        args += [sysroot+'/usr/lib/crtend_so.o']
    else:
        args += [sysroot+'/usr/lib/crtend_android.o']

    return run_cmd(args)

def do_compilation():
    global NDK, PLATFORM
    global BITCODE, OUTPUT
    global ABI, ARCH
    global VERBOSE, KEEP

    read_bitcode_wrapper(BITCODE)

    bc_file = tempfile.NamedTemporaryFile(delete=False)
    ret,text = do_translate(BITCODE, bc_file.name)
    if ret != 0:
        error('Exit status: %d, %s' %(ret, text))

    o_file = tempfile.NamedTemporaryFile(delete=False)
    ret,text = do_llc(bc_file.name, o_file.name)
    if ret != 0:
        error('Exit status: %d, %s' %(ret, text))

    ret,text = do_ld(o_file.name,OUTPUT)
    if ret != 0:
        error('Exit status: %d, %s' %(ret, text))

    bc_file.close()
    o_file.close()

    # clean up temporary files
    if KEEP == False:
        os.unlink(bc_file.name)
        os.unlink(o_file.name)
    return 0

def main():
    global NDK
    handle_args()
    locate_tools()
    do_compilation()

if __name__ == '__main__':
    main()
