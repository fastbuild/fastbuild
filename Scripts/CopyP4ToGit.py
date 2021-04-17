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

    should_copy = False

    # needs copying?
    if dst_filetime == 0:
        # Destination missing
        should_copy = True
    elif src_filetime != dst_filetime:
        # Modtime changed, but is file actually different?
        src_size = os.path.getsize(src_file)
        dst_size = os.path.getsize(dst_file)
        if  src_size != dst_size:
            # Size is different
            should_copy = True
        else:
            # Compare contents
            src_contents = open(src_file, "rb").read(src_size)
            dst_contents = open(dst_file, "rb").read(dst_size)
            if src_contents != dst_contents:
                should_copy = True

    if should_copy:
        print('COPY: {}'.format(src_file))

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

print('Pruning...')
dst_files = []
get_files_recurse(os.path.join("..", "..", "..", "git", "fastbuild", "Code"), dst_files)
get_files_recurse(os.path.join("..", "..", "..", "git", "fastbuild", "External"), dst_files)
get_files_recurse(os.path.join("..", "..", "..", "git", "fastbuild", "Scripts"), dst_files)
for file in dst_files:
    dst_file = file

    # ignore git repo files
    if dst_file.find(".git") != -1:
        continue

    # ignore SDK contents (but still consider bff files)
    if dst_file.find(os.path.join("External", "SDK")) != -1:
        if dst_file.endswith(".bff") == False:
            continue

    # create source path to match dst path
    src_file = dst_file.replace(os.path.join("..", "..", "..", "git", "fastbuild"),
                                os.path.join(".."))

    # does source file exist?
    if os.path.exists(src_file) == False:
        # delete dst file
        print('DELETE: {}'.format(dst_file))
        os.remove(dst_file)

print('Done.')
