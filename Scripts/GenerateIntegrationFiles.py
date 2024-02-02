# GenerateIntegrationFiles.py
#-------------------------------------------------------------------------------
#
# Generate files used to integrate FASTBuild into other tools
#  - VisualStudio usertype.dat
#  - Notepad++    notepad++markup.xml
#  - SublimeText  FASTBuild.sublime-syntax.txt
#-------------------------------------------------------------------------------

# imports
#-------------------------------------------------------------------------------
import argparse
import os

# get_files
#-------------------------------------------------------------------------------
def get_files_recurse(base_path, out_files):
    # traverse directory, and list directories as dirs and files as files
    for root, dirs, files in os.walk(base_path):
        path = root.split('/')
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                out_files.append(os.path.join(root, file))

# parse_files
#-------------------------------------------------------------------------------
def parse_files(files):
    functions = []
    properties = []
    keywords = []

    # Check all the files
    for filename in files:
        # Ignore files in tests
        if (filename.find('FBuildTest') != -1) or (filename.find('CoreTest') != -1):
            continue

        # Parse this file
        f = open(filename, 'r')
        for line in f.readlines():
            # Find Function derived classes
            if line.find('FNEW( Function') != -1:
                # Get function name (first quoted string)
                function_name = line[line.find('FNEW( Function')+14:]
                function_name = function_name[:function_name.find(' ')]
                functions.append(function_name)
                continue

            # Keywords
            if line.find('#define BFF_KEYWORD_') != -1:
                # Get directive name (first quoted string)
                keyword_name = line[line.find('"')+1:]
                keyword_name = keyword_name[:keyword_name.find('"')]
                keywords.append(keyword_name)

            # Properties
            if ((line.find('REFLECT(') == -1) and
                (line.find('REFLECT_ARRAY(') == -1) and
                (line.find('REFLECT_ARRAY_OF_STRUCT(') == -1) and
                (line.find('REFLECT_STRUCT(') == -1) and
                (line.find('MetaName( "') == -1)):
                continue
            if (line.find('#define') != -1):
                continue

            # Ignore hidden properties
            if (line.find('MetaHidden') != -1):
                continue

            # Get property name (first quoted string)
            property_name = line[line.find('"')+1:]
            property_name = property_name[:property_name.find('"')]
            properties.append(property_name)

    # Uniquify and sort
    functions = sorted(set(functions))
    properties = sorted(set(properties))
    keywords = sorted(set(keywords))

    return functions, properties, keywords

# does_file_need_update
#-------------------------------------------------------------------------------
def does_file_need_update(filename, contents):
    f = open(filename, 'r')
    existing_contents = f.read()
    f.close()
    return existing_contents != contents

# update_file
#-------------------------------------------------------------------------------
def update_file(filename, contents):
    try:
        f = open(filename, 'w')
        f.write(contents)
    except:
        print(f'Failed to update out-of-date file: {filename}')
        return False
    return True

# generate_usertype_dat
#-------------------------------------------------------------------------------
def generate_usertype_dat(check, functions, properties, keywords):
    # Generate file contents
    contents = ''
    # Keywords
    for keyword in keywords:
        contents += (keyword + '\n')
    contents += '\n'
    # Functions
    for function in functions:
        contents += (function + '\n')
    contents += '\n'
    # Properties
    for property in properties:
        contents += (property + '\n')

    # Write if differnt
    this_file_path = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(this_file_path, '..', 'Code', 'Tools', 'FBuild', 'Integration', 'usertype.dat')
    if does_file_need_update(filename, contents):
        if check:
            return False
        else:
            return update_file(filename, contents)
    return True

# generate_notepad_plusplus_markup_xml
#-------------------------------------------------------------------------------
def generate_notepad_plusplus_markup_xml(check, functions, properties, keywords):
    # Generate file contents
    contents = ''
    contents += '<NotepadPlus>\n'
    contents += '    <UserLang name="FASTBuild" ext="bff" udlVersion="2.1">\n'
    contents += '        <Settings>\n'
    contents += '            <Global caseIgnored="no" allowFoldOfComments="no" foldCompact="no" forcePureLC="0" decimalSeparator="0" />\n'
    contents += '            <Prefix Keywords1="yes" Keywords2="no" Keywords3="no" Keywords4="no" Keywords5="no" Keywords6="no" Keywords7="no" Keywords8="no" />\n'
    contents += '        </Settings>\n'
    contents += '        <KeywordLists>\n'
    contents += '            <Keywords name="Comments">00// 00; 01 02 03 04</Keywords>\n'
    contents += '            <Keywords name="Numbers, prefix1"></Keywords>\n'
    contents += '            <Keywords name="Numbers, prefix2"></Keywords>\n'
    contents += '            <Keywords name="Numbers, extras1"></Keywords>\n'
    contents += '            <Keywords name="Numbers, extras2"></Keywords>\n'
    contents += '            <Keywords name="Numbers, suffix1"></Keywords>\n'
    contents += '            <Keywords name="Numbers, suffix2"></Keywords>\n'
    contents += '            <Keywords name="Numbers, range"></Keywords>\n'
    contents += '            <Keywords name="Operators1">+ - =</Keywords>\n'
    contents += '            <Keywords name="Operators2">+ - = {}</Keywords>\n'.format(' '.join(keywords))
    contents += '            <Keywords name="Folders in code1, open"></Keywords>\n'
    contents += '            <Keywords name="Folders in code1, middle"></Keywords>\n'
    contents += '            <Keywords name="Folders in code1, close"></Keywords>\n'
    contents += '            <Keywords name="Folders in code2, open"></Keywords>\n'
    contents += '            <Keywords name="Folders in code2, middle"></Keywords>\n'
    contents += '            <Keywords name="Folders in code2, close"></Keywords>\n'
    contents += '            <Keywords name="Folders in comment, open"></Keywords>\n'
    contents += '            <Keywords name="Folders in comment, middle"></Keywords>\n'
    contents += '            <Keywords name="Folders in comment, close"></Keywords>\n'
    contents += '            <Keywords name="Keywords1">{}</Keywords>\n'.format('&#x000D;&#x000A;'.join(functions))
    contents += '            <Keywords name="Keywords2">{}</Keywords>\n'.format('&#x000D;&#x000A;'.join(properties))
    contents += '            <Keywords name="Keywords3">)</Keywords>\n'
    contents += '            <Keywords name="Keywords4">%1&#x000D;&#x000A;%2&#x000D;&#x000A;%3&#x000D;&#x000A;</Keywords>\n'
    contents += '            <Keywords name="Keywords5"></Keywords>\n'
    contents += '            <Keywords name="Keywords6"></Keywords>\n'
    contents += '            <Keywords name="Keywords7"></Keywords>\n'
    contents += '            <Keywords name="Keywords8"></Keywords>\n'
    contents += '            <Keywords name="Delimiters">00&apos; 01^ 02&apos; 03&quot; 04^ 05&quot; 06{ 07 08} 09$ 10 11$ 12 13 14 15 16 17 18 19 20 21 22 23</Keywords>\n'
    contents += '        </KeywordLists>\n'
    contents += '        <Styles>\n'
    contents += '            <WordsStyle name="DEFAULT" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="COMMENTS" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="LINE COMMENTS" fgColor="000000" bgColor="C8FFC8" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="NUMBERS" fgColor="FF8040" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS1" fgColor="0000FF" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS2" fgColor="0080FF" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS3" fgColor="0000FF" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS4" fgColor="800080" bgColor="FFFFFF" fontName="" fontStyle="1" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS5" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS6" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS7" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="KEYWORDS8" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="OPERATORS" fgColor="FF8000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="FOLDER IN CODE1" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="FOLDER IN CODE2" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="FOLDER IN COMMENT" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="DELIMITERS1" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="8200" />\n'
    contents += '            <WordsStyle name="DELIMITERS2" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="8200" />\n'
    contents += '            <WordsStyle name="DELIMITERS3" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="117444355" />\n'
    contents += '            <WordsStyle name="DELIMITERS4" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="1" nesting="0" />\n'
    contents += '            <WordsStyle name="DELIMITERS5" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="1" nesting="0" />\n'
    contents += '            <WordsStyle name="DELIMITERS6" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="DELIMITERS7" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '            <WordsStyle name="DELIMITERS8" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n'
    contents += '        </Styles>\n'
    contents += '    </UserLang>\n'
    contents += '</NotepadPlus>\n'

    # Write if different
    this_file_path = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(this_file_path, '..', 'Code', 'Tools', 'FBuild', 'Integration', 'notepad++markup.xml')
    if does_file_need_update(filename, contents):
        if check:
            return False
        else:
            return update_file(filename, contents)
    return True

# generate_sublime_syntax
#-------------------------------------------------------------------------------
def generate_sublime_syntax(check, functions, properties, keywords):
    # Generate file contents
    contents = ''
    contents += '%YAML 1.2\n'
    contents += '---\n'
    contents += 'name: FASTBuild\n'
    contents += 'file_extensions: [bff]\n'
    contents += 'scope: source.bff\n'
    contents += '\n'
    contents += 'contexts:\n'
    contents += '  in-double-quotes:\n'
    contents += '    - match: \$\w*\$\n'
    contents += '      scope: variable.other\n'
    contents += '    - match: "[^\\"]"\n'
    contents += '      scope: string.quoted.double\n'
    contents += '    - match: \\"\n'
    contents += '      pop: true\n'
    contents += '      scope: string.quoted.double\n'
    contents += '  double-quoted-strings:\n'
    contents += '    - match: \\"\n'
    contents += '      push: in-double-quotes\n'
    contents += '      scope: string.quoted.double\n'
    contents += '\n'
    contents += '  in-single-quotes:\n'
    contents += '    - match: \$\w*\$\n'
    contents += '      scope: variable.other\n'
    contents += '    - match: "[^\']"\n'
    contents += '      scope: string.quoted.single\n'
    contents += '    - match: "\'"\n'
    contents += '      pop: true\n'
    contents += '      scope: string.quoted.single\n'
    contents += '  single-quoted-strings:\n'
    contents += '    - match: "\'"\n'
    contents += '      push: in-single-quotes\n'
    contents += '      scope: string.quoted.single\n'
    contents += '\n'
    contents += '  preprocessor-includes:\n'
    contents += '    - match: "^\\\\s*(#\\\\s*\\\\binclude)\\\\b"\n'
    contents += '      captures:\n'
    contents += '        1: keyword.control.include.c++\n'
    contents += '  preprocessor-import:\n'
    contents += '    - match: ^\\s*(#)\\s*\\b(import)\\b\n'
    contents += '      scope: keyword.control.c\n'
    contents += '\n'
    contents += '  preprocessor:\n'
    contents += '    - include: scope:source.c#incomplete-inc\n'
    contents += '    - include: preprocessor-macro-define\n'
    contents += '    - include: scope:source.c#pragma-mark\n'
    contents += '    - include: preprocessor-includes\n'
    contents += '    - include: preprocessor-import\n'
    contents += '  global:\n'
    contents += '    - include: preprocessor\n'
    contents += '    - include: double-quoted-strings\n'
    contents += '    - include: single-quoted-strings\n'
    contents += '  main:\n'
    contents += '    - include: global\n'
    contents += '    - match: \\b({})\\b\n'.format('|'.join(functions))
    contents += '      scope: support.function\n'
    contents += '    - match: \+|=-\n'
    contents += '      scope: keyword.operator\n'
    contents += '    - match: \,\n'
    contents += '      scope: punctuation.separator\n'
    contents += '    - match: \{\n'
    contents += '      scope: punctuation.section.block.begin\n'
    contents += '    - match: \}\n'
    contents += '      scope: punctuation.section.block.end\n'
    contents += '    - match: \.\w+|\^\w+\n'
    contents += '      scope: variable\n'
    contents += '    - match: //.*|;.*\n'
    contents += '      scope: comment.line\n'

    # Write if different
    this_file_path = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(this_file_path, '..', 'Code', 'Tools', 'FBuild', 'Integration', 'FASTBuild.sublime-syntax.txt')
    if does_file_need_update(filename, contents):
        if check:
            return False
        else:
            return update_file(filename, contents)
    return True

# main
#-------------------------------------------------------------------------------
def main():
    # Handle command line args
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--check', dest='check', help='Check if update is needed but don\'t attempt writing')
    args = parser.parse_args()
    check = args.check

    # get all the cpp files
    cpp_files = []
    get_files_recurse(os.path.join('..', 'Code'), cpp_files)

    # gather all the exposed keywords
    functions, properties, directives = parse_files(cpp_files)

    # generate usertype.dat for Visual Studio
    ok = generate_usertype_dat(check, functions, properties, directives)

    # generate Notepad++ markup xml
    ok &= generate_notepad_plusplus_markup_xml(check, functions, properties, directives)

    # generate sublime markup
    ok &= generate_sublime_syntax(check, functions, properties, directives)

    exit(0 if ok else 1)

#-------------------------------------------------------------------------------
if __name__ == '__main__':
    main()

#-------------------------------------------------------------------------------
