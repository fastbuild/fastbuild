import os
import subprocess
import sys

def getfiles(outFiles):
    # get .cpp and .h files
    for root, dirs, files in os.walk("."):
        path = root.split('/')
        for file in files:
            if file.endswith(".cpp") or file.endswith(".h"):
                fullPath = root + '/' + file
                if fullPath.find("Test\Data") == -1:
                    outFiles.append(fullPath)

files = []
print('Getting list of files...')
getfiles(files)
print('Formatting files...')
numFilesCheck = 0
for file in files:
    if os.access(file, os.W_OK) == False:
        continue

    numFilesCheck = numFilesCheck + 1

    print(' - {}'.format(file))

    # Run clang-format
    cfArgs = [];
    cfArgs.append("../External/SDK/Clang/Windows/10.0.0.RC3/bin/clang-format.exe")
    cfArgs.append("--style=file")
    cfArgs.append("-i")
    cfArgs.append(file)
    subprocess.call(cfArgs)

print("Num Files Scanned: {}".format(numFilesCheck))
