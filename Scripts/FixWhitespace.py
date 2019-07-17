import os
import sys

def getfiles(outFiles):
    # traverse root directory, and list directories as dirs and files as files
    for root, dirs, files in os.walk("."):
        path = root.split('/')
        for file in files:
            if file.endswith(".bff") or file.endswith(".cpp") or file.endswith(".h"):
                fullPath = root + '/' + file
                outFiles.append(fullPath)
    for root, dirs, files in os.walk("../External/"):
        path = root.split('/')
        for file in files:
            if file.endswith(".bff"):
                fullPath = root + '/' + file
                outFiles.append(fullPath)

files = []
print('Getting list of files...')
getfiles(files)
print('Checking files...')
for file in files:
    fixedFile = []
    fixed = False

    f = open(file, 'r')
    for line in f.readlines():
        newLine = ""

        # does line need tab fixup
        if line.find("\t") != -1:
            for i in range(0, len(line)):
                c = line[i]
                j = len(newLine)
                if c == "\t":
                    if (j % 4) == 0:
                        newLine += '    '
                    elif (j % 4) == 1:
                        newLine += '   '
                    elif (j % 4) == 2:
                        newLine += '  '
                    elif (j % 4) == 3:
                        newLine += ' '
                else:
                    newLine += c
        else:
            newLine = line

        # does line need trimming?
        newLine = newLine.rstrip();
        newLine += '\n'

        if newLine != line:
            fixed = True

        fixedFile += newLine
    f.close()

    # any changes made?
    if fixed == True:
        try:
            f = open(file, 'w')
            f.writelines(fixedFile)
            f.close()
            print(' - FIXED: {}'.format(file))
        except:
            print(' - FIX FAILED: {}'.format(file))
