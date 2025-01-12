# CheckGLIBCCompat.py
#-------------------------------------------------------------------------------
#
# Check compatibility of Linux binaries wrt GLIBC and GLIBCXX
#-------------------------------------------------------------------------------

# Imports
#-------------------------------------------------------------------------------
import subprocess

# Globals
#-------------------------------------------------------------------------------
# ANSI escape seqeunces
ESC_FAIL    = '\033[91m'
ESC_OK      = '\033[92m'
ESC_BOLDON  = '\033[1m'
ESC_BOLDOFF = '\033[22m'
ESC_END     = '\033[0m'

# Paths to binaries and symbols
EXE_FBUILD          = '../tmp/x64ClangLinux-Release/Tools/FBuild/FBuild/fbuild'
EXE_FBUILDWORKER    = '../tmp/x64ClangLinux-Release/Tools/FBuild/FBuildWorker/fbuildworker'

# Acceptable glibc versions
OK_GLIBC_VERSIONS   = [
                        # Note trailing spaces to disambiguate X.Y from X.YZ
                        'GLIBC_2.2.5 ',
                        'GLIBC_2.3 ',
                        'GLIBC_2.3.3 ',
                        'GLIBC_2.6 ',
                        'GLIBC_2.10 ',
                      ]

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

# main
#-------------------------------------------------------------------------------
def check_dependencies(exe):
    # get the list of dynamic symbols
    args = [
        'readelf',
        '--dyn-syms',
        '--wide',
        exe
    ]
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        # Failed
        print_fail('FAIL', exe)
        print(result.stderr.decode('utf-8'))
        return False

    overall_ok = True

    # split output into lines
    lines = result.stdout.decode('utf-8').splitlines()
    for line in lines:
        # Check for GLIBC use
        if ' UND ' not in line:
            continue
        if 'GLIBC' not in line:
            continue

        # Check if this version is allowed
        this_ok = False
        for ok_glibc in OK_GLIBC_VERSIONS:
            if ok_glibc in line:
                this_ok = True
                break

        # Print bad dependencies
        if not this_ok:
            overall_ok = False
            symbol_pos = line.find(' UND ') + 5
            print_fail('FAIL', line[symbol_pos:])

    # Ok
    if overall_ok:
        print_ok(exe)
    return overall_ok

# main
#-------------------------------------------------------------------------------
def main():
    # Check FBuild
    print_header('Checking FBuild')
    ok = check_dependencies(EXE_FBUILD)

    # Check FBuildWorker
    print_header('Checking FBuildWorker')
    ok &= check_dependencies(EXE_FBUILDWORKER)

    print('')

    # Overall result
    print_header('Final Result')
    print('============')
    if ok:
        print_ok('Overall')
    else:
        print_fail('FAIL')
    print('============')

    exit(0 if ok else 1)

#-------------------------------------------------------------------------------
if __name__ == '__main__':
    main()

#-------------------------------------------------------------------------------
