# ClangFormat.py
#-------------------------------------------------------------------------------
#
# Apply Clang Format to source files
#
#-------------------------------------------------------------------------------

# imports
#-------------------------------------------------------------------------------
import argparse
import os
import subprocess

# get_files_recurse
#-------------------------------------------------------------------------------
def get_files_recurse(base_path, out_files):
    # traverse directory, and list directories as dirs and files as files
    for root, dirs, files in os.walk(base_path):
        path = root.split('/')
        for file in files:
            # only include code files
            # TODO: Handle .mm and .m files
            if (not file.endswith(".h") and
                not file.endswith(".c") and
                not file.endswith(".cpp")):
                continue;

            out_files.append(os.path.join(root, file))

# get_clang_format_exe
#-------------------------------------------------------------------------------
def get_clang_format_exe():
    # TODO:B Extend to OSX and Linux
    potential_locations = [
                            '..\\External\\SDK\\Clang\\Windows\\20.1.0\\bin\\clang-format.exe',
                            'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\bin\\clang-format.exe',
                            'C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\Llvm\\bin\\clang-format.exe',
                            'C:\\Program Files\\LLVM\\bin\\clang-format.exe',
                          ]

    # We require at least clang-format 19.x.x
    min_major_version = 19

    too_old_versions = ''

    for potential_location in potential_locations:
        if os.path.exists(potential_location):
            # found at this location. Check version
            cmdline = '"{}" --version"'.format(potential_location)
            result = subprocess.run(cmdline,
                                    shell = True,
                                    capture_output = True,
                                    text = True)

            # handle invocation failures
            if result.returncode != 0:
                print('FAILED to check version for clang-format: "{}"'.format(potential_location))
                continue

            # check major version
            version_string = result.stdout
            version_string = version_string.replace('clang-format version ', '')
            major_version = int(version_string.split('.')[0])
            if major_version < min_major_version:
                # version is too old
                too_old_versions += 'Too old:\n'
                too_old_versions += ' - Location: {}\n'.format(potential_location)
                too_old_versions += ' - Version : {}\n'.format(version_string)
                continue;

            print('Using clang-format:')
            print(' - Location: {}'.format(potential_location))
            print(' - Version : {}'.format(version_string))
            return potential_location

    if too_old_versions == '':
        print('Failed to find any clang-format executable')
    else:
        print('Failed to find clang-format of at least v{}.x.x'.format(min_major_version))
        print(too_old_versions)
    exit(2);

# Handle args
#-------------------------------------------------------------------------------
parser = argparse.ArgumentParser()
parser.add_argument('-c', '--check', action='store_true', help='Do not apply fixes. Return failure if issues found.')
args = parser.parse_args()

# Get all the files
#-------------------------------------------------------------------------------
print('Getting file lists...')
files = []
get_files_recurse(os.path.join("..", "Code"), files)
print('...Found {} files'.format(len(files)))
print('')

# Find Clang Format executable
#-------------------------------------------------------------------------------
clang_format_exe = get_clang_format_exe()

#-------------------------------------------------------------------------------
if args.check:
    print('Checking...')
else:
    print('Formatting...')
num_files_needing_changes = 0
ok = True
for file in files:
    # check if this file needs changes
    cmdline = '"{}" --dry-run "{}"'.format(clang_format_exe, file)
    result = subprocess.run(cmdline,
                            shell = True,
                            capture_output = True,
                            text = True)

    # handle invocation failures
    if result.returncode != 0:
        print('FAILED to run clang-format on {}'.format(file))
        ok = False
        continue

    # does file need changes?
    if len(result.stderr) != 0:
        num_files_needing_changes = num_files_needing_changes + 1

        if args.check:
            # in "check" mode, finding needed changes is an error
            ok = False

            # print details of the necessary fixes
            print('{}'.format(result.stderr))
        else:
            # apply fixes
            cmdline = '"{}" --i "{}"'.format(clang_format_exe, file)
            result = subprocess.run(cmdline,
                                    shell = True,
                                    capture_output = True,
                                    text = True)

            # handle invocation failures
            if result.returncode != 0:
                print('FAILED to run clang-format on {}'.format(file))
                ok = False
                continue

            # note that we applied fixes to the file
            print('Fixed: {}'.format(file))

# summarize overall work
if args.check:
    print('.. {} files need changes.'.format(num_files_needing_changes))
else:
    print('.. {} files formatted.'.format(num_files_needing_changes))

exit(0 if ok else 1)
