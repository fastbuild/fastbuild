# ClangFormat.py
#-------------------------------------------------------------------------------
#
# Apply Clang Format to source files
#
#-------------------------------------------------------------------------------

# imports
#-------------------------------------------------------------------------------
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

# Get all the files
#-------------------------------------------------------------------------------
print('Getting file lists...')
files = []
get_files_recurse(os.path.join("..", "Code"), files)
print('...Found {} files'.format(len(files)))
print('')

#-------------------------------------------------------------------------------
print('Checking...')
num_files_needing_changes = 0
ok = True
for file in files:
    # check if this file needs changes
    args = "..\\External\\SDK\\Clang\\Windows\\20.1.0\\bin\\clang-format.exe --dry-run {}".format(file)
    result = subprocess.run(args, shell = True, capture_output = True, text = True)

    #
    if result.returncode != 0:
        print('FAILED to check {}'.format(file))
        ok = False
        continue

    # print info for files that need changes
    if len(result.stderr) != 0:
        print('{}'.format(result.stderr))
        ok = False
        num_files_needing_changes = num_files_needing_changes + 1

# summarize overall work
print('.. {} files need changes.'.format(num_files_needing_changes))

exit(0 if ok else 1)
