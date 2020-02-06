# CopyP4ToGit.py
#-------------------------------------------------------------------------------
#
# Synchronize P4 depot to git repo
#
#-------------------------------------------------------------------------------

# imports
#-------------------------------------------------------------------------------
import os
import pathlib
import shutil
import stat
import sys

# get_files_recurse
#-------------------------------------------------------------------------------
def get_files_recurse(base_path, out_files):
    # traverse directory, and list directories as dirs and files as files
    for root, dirs, files in os.walk(base_path):
        path = root.split('/')
        for file in files:
            out_files.append(os.path.join(root, file))

# Get all the files
#-------------------------------------------------------------------------------
print('Getting file lists...')
files = []
get_files_recurse(os.path.join("..", "Code"), files)
get_files_recurse(os.path.join("..", "External"), files)
get_files_recurse(os.path.join("..", "Scripts"), files)
files.append(os.path.join("..", ".travis.yml"))
files.append(os.path.join("..", "pull_request_template.md"))

print('Copying...')
for file in files:
    src_file = file

    # skip .bak files
    if src_file.endswith(".bak"):
        continue;

    # skip FASTBuild .fdb
    if src_file.endswith(".fdb"):
        continue;

    # skip FASTBuildWorker related files
    if src_file.endswith(".copy") or src_file.endswith(".settings"):
        continue;

    # skip profile.json
    if src_file.find("profile") and src_file.endswith(".json"):
        continue;

    # skip vs created %ALLUSERSPROFILE% folders
    if src_file.find("%ALLUSERSPROFILE%") != -1:
        continue;

    # skip some vs sdk sub-folders
    if src_file.find("ItemTemplates") != -1:
        continue;
    if src_file.find("ProjectTemplates") != -1:
        continue;
    if src_file.find("ProjectTemplatesCache") != -1:
        continue;
    if (src_file.find("IDE") != -1) and (src_file.find("Extensions") != -1):
        continue;

    # create relative source path
    dst_file = src_file.replace(os.path.join(".."),
                                os.path.join("..", "..", "..", "git", "fastbuild"))

    # compare filetimes
    src_filetime = os.path.getmtime(src_file)
    dst_filetime = 0
    try:
        dst_filetime = os.path.getmtime(dst_file)
    except:
        pass # file might be new

    # needs copying?
    if src_filetime != dst_filetime:
        # YES
        print('COPY: {} -> {}'.format(src_file, dst_file))

        # Create dest dir if needed
        dst_path = os.path.dirname(os.path.normpath(dst_file))
        if os.path.isdir(dst_path) == False:
            os.makedirs(dst_path)

        # Make dest file writable if needed
        try:
            os.chmod(dst_file, stat.S_IWRITE)
        except:
            pass

        # copy file
        shutil.copy2(src_file, dst_path)

print('Done.')
