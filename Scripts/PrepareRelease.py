# PrepareRelease.py
#-------------------------------------------------------------------------------
#
# Generate various args for a release
#-------------------------------------------------------------------------------

# imports
#-------------------------------------------------------------------------------
import argparse
import os
import shutil
import stat
import subprocess
import zipfile

# Globals
#-------------------------------------------------------------------------------
# ANSI escape seqeunces
ESC_FAIL    = '\033[91m'
ESC_OK      = '\033[92m'
ESC_BOLDON  = '\033[1m'
ESC_BOLDOFF = '\033[22m'
ESC_END     = '\033[0m'
# Paths to binaries and symbols
EXE_LINUX_FBUILD             = '../tmp/x64Linux-Release/Tools/FBuild/FBuild/fbuild'
EXE_LINUX_FBUILD_SYM         = '../tmp/x64Linux-Release/Tools/FBuild/FBuild/fbuild.debug'
EXE_LINUX_FBUILDWORKER       = '../tmp/x64Linux-Release/Tools/FBuild/FBuildWorker/fbuildworker'
EXE_LINUX_FBUILDWORKER_SYM   = '../tmp/x64Linux-Release/Tools/FBuild/FBuildWorker/fbuildworker.debug'
EXE_OSX_FBUILD               = '../tmp/OSX-Release/Tools/FBuild/FBuild/FBuild'
EXE_OSX_FBUILDWORKER         = '../tmp/OSX-Release/Tools/FBuild/FBuildWorker/FBuildWorker'
EXE_WINDOWS_FBUILD           = '../tmp/x64-Release/Tools/FBuild/FBuild/FBuild.exe'
EXE_WINDOWS_FBUILD_SYM       = '../tmp/x64-Release/Tools/FBuild/FBuild/FBuild.pdb'
EXE_WINDOWS_FBUILDWORKER     = '../tmp/x64-Release/Tools/FBuild/FBuildWorker/FBuildWorker.exe'
EXE_WINDOWS_FBUILDWORKER_SYM = '../tmp/x64-Release/Tools/FBuild/FBuildWorker/FBuildWorker.pdb'
# Paths to integration files
INTEGRATION_FILES   = {
                        '../Code/Tools/FBuild/Integration/FASTBuild.sublime-syntax.txt',
                        '../Code/Tools/FBuild/Integration/fbuild.bash-completion',
                        '../Code/Tools/FBuild/Integration/notepad++markup.xml',
                        '../Code/Tools/FBuild/Integration/usertype.dat',
                      }
# License file
LICENSE_TXT = '../Code/LICENSE.TXT'
# Output path
OUTPUT_ROOT  = '../tmp/Releases'

# print_header
#-------------------------------------------------------------------------------
def print_header(title):
    print(f'{ESC_BOLDON}{title}{ESC_BOLDOFF}')

# print_ok
#-------------------------------------------------------------------------------
def print_ok(msg = ''):
    if msg:
       print(f' {ESC_OK}OK{ESC_END}          - {msg}')
    else:
       print(f' {ESC_OK}OK{ESC_END}')

# print_fail
#-------------------------------------------------------------------------------
def print_fail(fail_type, msg = ''):
    if msg:
        print(f' {ESC_FAIL}{fail_type: <12}{ESC_END}- {msg}')
    else:
        print(f' {ESC_FAIL}{fail_type}{ESC_END}')

# check_binary
#-------------------------------------------------------------------------------
def check_binary(version, filename, symbol_filename = ''):
    # Read file contents
    try:
        file = open(filename, "rb")
        data = file.read()
        file.close()
    except:
        if not os.path.exists(filename):
            print_fail('MISSING', filename)
        else:
            print_fail('READFAIL', filename)
        return False

    # Check version (should be present as null terminated byte sequence)
    expected_bytes = b'v' + bytearray(version.encode()) + b'\x00'
    if data.find(expected_bytes) == -1:
        print_fail('BADVER', filename)
        return False

    # Check symbol file present if requires
    if symbol_filename:
        if not os.path.exists(symbol_filename):
            print_fail('MISSING SYM', filename)
            return False

    # All checks passed
    print_ok(filename)
    return True

# check_binaries
#-------------------------------------------------------------------------------
def check_binaries(version):
    print_header('Checking Binaries')
    # Linux
    ok =  check_binary(version, EXE_LINUX_FBUILD, EXE_LINUX_FBUILD_SYM)
    ok &= check_binary(version, EXE_LINUX_FBUILDWORKER, EXE_LINUX_FBUILDWORKER_SYM)
    # OSX (Universal)
    ok =  check_binary(version, EXE_OSX_FBUILD)
    ok &= check_binary(version, EXE_OSX_FBUILDWORKER)
    # Windows
    ok &= check_binary(version, EXE_WINDOWS_FBUILD, EXE_WINDOWS_FBUILD_SYM)
    ok &= check_binary(version, EXE_WINDOWS_FBUILDWORKER, EXE_WINDOWS_FBUILDWORKER_SYM)
    return ok

# prepare_output_folders
#-------------------------------------------------------------------------------
def prepare_output_folders(version):
    print_header('Creating directory structure')
    try:
        if not os.path.exists(OUTPUT_ROOT):
            os.mkdir(OUTPUT_ROOT)
        if not os.path.exists(f'{OUTPUT_ROOT}/{version}'):
            os.mkdir(f'{OUTPUT_ROOT}/{version}')
        print_ok()
    except:
        print_fail('FAIL', 'Directory creation failed')
        return False
    return True

# handle_integration_files
#-------------------------------------------------------------------------------
def handle_integration_files(version):
    print_header('Integration Files')
    # Check if integration files are up-to-date
    check_ok = (subprocess.call("..\Scripts\GenerateIntegrationFiles.py -check", shell = True) == 0)
    if check_ok:
        print_ok('Integration Files up-to-date')
    else:
        print_fail('FAIL', 'Integration Files up-to-date')

    # Stage integration files in output dir
    copy_ok = True
    for filename in INTEGRATION_FILES:
        #try:
            # Ensure destination writable if it already exists
            dst_file = f'{OUTPUT_ROOT}/{version}/' + filename[filename.rfind('/') + 1:]
            if os.path.exists(dst_file):
                os.chmod(dst_file, stat.S_IWRITE)
            # Copy file
            shutil.copy(filename, dst_file)
        #except:
        #    print_fail('FAIL', f'Copy failed: {filename} -> {OUTPUT_ROOT}/{version}/')
        #    copy_ok = False
    if copy_ok:
        print_ok('Copy Integration Files')

    # Overall result
    return check_ok and copy_ok

# create_binary_archive
#-------------------------------------------------------------------------------
def create_binary_archive(version, os, arch, exe1, exe2):
    try:
        with zipfile.ZipFile(f'{OUTPUT_ROOT}/{version}/FASTBuild-{os}-{arch}-v{version}.zip', 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            archive.write(exe1, exe1[exe1.rfind('/') + 1:])
            archive.write(exe2, exe2[exe2.rfind('/') + 1:])
            archive.write(LICENSE_TXT, LICENSE_TXT[LICENSE_TXT.rfind('/') + 1:])
        print_ok(f'{os}-{arch} Binaries')
        return True
    except:
        print_fail('FAIL', f'{os}-{arch} Binaries')
        return False

# get_files_recurse
#-------------------------------------------------------------------------------
def get_files_recurse(base_path, out_files):
    # traverse directory, and list directories as dirs and files as files
    for root, dirs, files in os.walk(base_path):
        path = root.split('/')
        for file in files:
            # Ignore various temp Files
            if file.endswith('.bak') or \
               file.endswith('.d') or \
               file.endswith('.fdb') or \
               file.endswith('.pyc') or \
               file == 'fbuild_profile.json' or \
               file == 'profile.json':
               continue;

            # Ignore executables in path
            if file.endswith('.exe') or \
               file == 'fbuild-linux':
               continue;

            # Ignore non-bff files in External\SDKs\
            if root.find('External\\SDK\\') != -1 and \
               not file.endswith('.bff'):
               continue

            out_files.append(os.path.join(root, file))

# create_source_archive
#-------------------------------------------------------------------------------
def create_source_archive(version):
    try:
        # Get source files
        files = []
        get_files_recurse('../Code', files)
        get_files_recurse('../External', files)
        get_files_recurse('../Scripts', files)

        # Create archive
        with zipfile.ZipFile(f'{OUTPUT_ROOT}/{version}/FASTBuild-Src-v{version}.zip', 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            # Add source files
            for file in files:
                relative_file = file.replace('../', '')
                archive.write(file, relative_file)

            # Add binaries
            archive.write(EXE_WINDOWS_FBUILD,           'Bin/Windows-x64/FBuild.exe')
            archive.write(EXE_WINDOWS_FBUILD_SYM,       'Bin/Windows-x64/FBuild.pdb')
            archive.write(EXE_WINDOWS_FBUILDWORKER,     'Bin/Windows-x64/FBuildWorker.exe')
            archive.write(EXE_WINDOWS_FBUILDWORKER_SYM, 'Bin/Windows-x64/FBuildWorker.pdb')
            archive.write(EXE_LINUX_FBUILD,             'Bin/Linux-x64/fbuild')
            archive.write(EXE_LINUX_FBUILD_SYM,         'Bin/Linux-x64/fbuild.debug')
            archive.write(EXE_LINUX_FBUILDWORKER,       'Bin/Linux-x64/fbuildworker')
            archive.write(EXE_LINUX_FBUILDWORKER_SYM,   'Bin/Linux-x64/fbuildworker.debug')
            archive.write(EXE_OSX_FBUILD,               'Bin/OSX-x64+ARM/FBuild')
            archive.write(EXE_OSX_FBUILDWORKER,         'Bin/OSX-x64+ARM/FBuildWorker')
        print_ok('Source')
        return True
    except:
        print_fail('FAIL', 'Source')
        return False

# create_archives
#-------------------------------------------------------------------------------
def create_archives(version):
    print_header('Creating Archives')
    # Binary archives
    ok  = create_binary_archive(version, 'Windows', 'x64', EXE_WINDOWS_FBUILD, EXE_WINDOWS_FBUILDWORKER)
    ok &= create_binary_archive(version, 'OSX', 'x64+ARM', EXE_OSX_FBUILD, EXE_OSX_FBUILDWORKER)
    ok &= create_binary_archive(version, 'Linux', 'x64', EXE_LINUX_FBUILD, EXE_LINUX_FBUILDWORKER)

    # Source archive
    ok &= create_source_archive(version)

    return ok

# main
#-------------------------------------------------------------------------------
def main():
    # Handle command line args
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--version", dest="version", required=True, help="Version to package")
    args = parser.parse_args()

    # Ensure all binaries are built at the correct version and ready to ship
    ok = check_binaries(args.version)
    print('')

    # Prepare output directory structure
    ok &= prepare_output_folders(args.version)
    print('')

    # Check integration files are up-to-date
    ok &= handle_integration_files(args.version)
    print('')

    # Create Archives
    ok &= create_archives(args.version)
    print('')

    # Overall result
    print_header('Final Result')
    print('============')
    if ok:
        print_ok('Overall')
    else:
        print_fail('FAIL')
    print('============')

#-------------------------------------------------------------------------------
if __name__ == '__main__':
    main()

#-------------------------------------------------------------------------------
