# GenerateIntegrationFiles.py
#-------------------------------------------------------------------------------
#
# Generate files used to integrate FASTBuild into other tools
#  - VisualStudio usertype.dat
#-------------------------------------------------------------------------------

# imports
#-------------------------------------------------------------------------------
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

# generate_usertype_dat
#-------------------------------------------------------------------------------
def generate_usertype_dat(functions, properties, keywords):
    # open usertype.dat
    this_file_path = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(this_file_path, '..', 'Code', 'Tools', 'FBuild', 'Integration', 'usertype.dat')
    f = open(filename, 'w')

    # Keywords
    for keyword in keywords:
        f.write(keyword + '\n')
    f.write('\n')
    # Functions
    for function in functions:
        f.write(function + '\n')
    f.write('\n')
    # Properties
    for property in properties:
        f.write(property + '\n')

# generate_notepad_plusplus_markup_xml
#-------------------------------------------------------------------------------
def generate_notepad_plusplus_markup_xml(functions, properties, keywords):
    # open usertype.dat
    this_file_path = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(this_file_path, '..', 'Code', 'Tools', 'FBuild', 'Integration', 'notepad++markup.xml')
    f = open(filename, 'w')

    # write
    f.write('<NotepadPlus>\n')
    f.write('    <UserLang name="FASTBuild" ext="bff" udlVersion="2.1">\n')
    f.write('        <Settings>\n')
    f.write('            <Global caseIgnored="no" allowFoldOfComments="no" foldCompact="no" forcePureLC="0" decimalSeparator="0" />\n')
    f.write('            <Prefix Keywords1="yes" Keywords2="no" Keywords3="no" Keywords4="no" Keywords5="no" Keywords6="no" Keywords7="no" Keywords8="no" />\n')
    f.write('        </Settings>\n')
    f.write('        <KeywordLists>\n')
    f.write('            <Keywords name="Comments">00// 00; 01 02 03 04</Keywords>\n')
    f.write('            <Keywords name="Numbers, prefix1"></Keywords>\n')
    f.write('            <Keywords name="Numbers, prefix2"></Keywords>\n')
    f.write('            <Keywords name="Numbers, extras1"></Keywords>\n')
    f.write('            <Keywords name="Numbers, extras2"></Keywords>\n')
    f.write('            <Keywords name="Numbers, suffix1"></Keywords>\n')
    f.write('            <Keywords name="Numbers, suffix2"></Keywords>\n')
    f.write('            <Keywords name="Numbers, range"></Keywords>\n')
    f.write('            <Keywords name="Operators1">+ - =</Keywords>\n')
    f.write('            <Keywords name="Operators2">+ - = {}</Keywords>\n'.format(' '.join(keywords)))
    f.write('            <Keywords name="Folders in code1, open"></Keywords>\n')
    f.write('            <Keywords name="Folders in code1, middle"></Keywords>\n')
    f.write('            <Keywords name="Folders in code1, close"></Keywords>\n')
    f.write('            <Keywords name="Folders in code2, open"></Keywords>\n')
    f.write('            <Keywords name="Folders in code2, middle"></Keywords>\n')
    f.write('            <Keywords name="Folders in code2, close"></Keywords>\n')
    f.write('            <Keywords name="Folders in comment, open"></Keywords>\n')
    f.write('            <Keywords name="Folders in comment, middle"></Keywords>\n')
    f.write('            <Keywords name="Folders in comment, close"></Keywords>\n')
    f.write('            <Keywords name="Keywords1">{}</Keywords>\n'.format('&#x000D;&#x000A;'.join(functions)))
    f.write('            <Keywords name="Keywords2">{}</Keywords>\n'.format('&#x000D;&#x000A;'.join(properties)))
    f.write('            <Keywords name="Keywords3">)</Keywords>\n')
    f.write('            <Keywords name="Keywords4">%1&#x000D;&#x000A;%2&#x000D;&#x000A;%3&#x000D;&#x000A;</Keywords>\n')
    f.write('            <Keywords name="Keywords5"></Keywords>\n')
    f.write('            <Keywords name="Keywords6"></Keywords>\n')
    f.write('            <Keywords name="Keywords7"></Keywords>\n')
    f.write('            <Keywords name="Keywords8"></Keywords>\n')
    f.write('            <Keywords name="Delimiters">00&apos; 01^ 02&apos; 03&quot; 04^ 05&quot; 06{ 07 08} 09$ 10 11$ 12 13 14 15 16 17 18 19 20 21 22 23</Keywords>\n')
    f.write('        </KeywordLists>\n')
    f.write('        <Styles>\n')
    f.write('            <WordsStyle name="DEFAULT" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="COMMENTS" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="LINE COMMENTS" fgColor="000000" bgColor="C8FFC8" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="NUMBERS" fgColor="FF8040" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS1" fgColor="0000FF" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS2" fgColor="0080FF" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS3" fgColor="0000FF" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS4" fgColor="800080" bgColor="FFFFFF" fontName="" fontStyle="1" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS5" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS6" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS7" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="KEYWORDS8" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="OPERATORS" fgColor="FF8000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="FOLDER IN CODE1" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="FOLDER IN CODE2" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="FOLDER IN COMMENT" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="DELIMITERS1" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="8200" />\n')
    f.write('            <WordsStyle name="DELIMITERS2" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="8200" />\n')
    f.write('            <WordsStyle name="DELIMITERS3" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="117444355" />\n')
    f.write('            <WordsStyle name="DELIMITERS4" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="1" nesting="0" />\n')
    f.write('            <WordsStyle name="DELIMITERS5" fgColor="C80000" bgColor="FFFFFF" fontName="" fontStyle="1" nesting="0" />\n')
    f.write('            <WordsStyle name="DELIMITERS6" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="DELIMITERS7" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('            <WordsStyle name="DELIMITERS8" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" nesting="0" />\n')
    f.write('        </Styles>\n')
    f.write('    </UserLang>\n')
    f.write('</NotepadPlus>\n')

# generate_sublime_syntax
#-------------------------------------------------------------------------------
def generate_sublime_syntax(functions, properties, keywords):
    # open usertype.dat
    this_file_path = os.path.dirname(os.path.realpath(__file__))
    filename = os.path.join(this_file_path, '..', 'Code', 'Tools', 'FBuild', 'Integration', 'FASTBuild.sublime-syntax.txt')
    f = open(filename, 'w')

    f.write('%YAML 1.2\n')
    f.write('---\n')
    f.write('name: FASTBuild\n')
    f.write('file_extensions: [bff]\n')
    f.write('scope: source.bff\n')
    f.write('\n')
    f.write('contexts:\n')
    f.write('  in-double-quotes:\n')
    f.write('    - match: \$\w*\$\n')
    f.write('      scope: variable.other\n')
    f.write('    - match: "[^\\"]"\n')
    f.write('      scope: string.quoted.double\n')
    f.write('    - match: \\"\n')
    f.write('      pop: true\n')
    f.write('      scope: string.quoted.double\n')
    f.write('  double-quoted-strings:\n')
    f.write('    - match: \\"\n')
    f.write('      push: in-double-quotes\n')
    f.write('      scope: string.quoted.double\n')
    f.write('\n')
    f.write('  in-single-quotes:\n')
    f.write('    - match: \$\w*\$\n')
    f.write('      scope: variable.other\n')
    f.write('    - match: "[^\']"\n')
    f.write('      scope: string.quoted.single\n')
    f.write('    - match: "\'"\n')
    f.write('      pop: true\n')
    f.write('      scope: string.quoted.single\n')
    f.write('  single-quoted-strings:\n')
    f.write('    - match: "\'"\n')
    f.write('      push: in-single-quotes\n')
    f.write('      scope: string.quoted.single\n')
    f.write('\n')
    f.write('  preprocessor-includes:\n')
    f.write('    - match: "^\\\\s*(#\\\\s*\\\\binclude)\\\\b"\n')
    f.write('      captures:\n')
    f.write('        1: keyword.control.include.c++\n')
    f.write('  preprocessor-import:\n')
    f.write('    - match: ^\\s*(#)\\s*\\b(import)\\b\n')
    f.write('      scope: keyword.control.c\n')
    f.write('\n')
    f.write('  preprocessor:\n')
    f.write('    - include: scope:source.c#incomplete-inc\n')
    f.write('    - include: preprocessor-macro-define\n')
    f.write('    - include: scope:source.c#pragma-mark\n')
    f.write('    - include: preprocessor-includes\n')
    f.write('    - include: preprocessor-import\n')
    f.write('  global:\n')
    f.write('    - include: preprocessor\n')
    f.write('    - include: double-quoted-strings\n')
    f.write('    - include: single-quoted-strings\n')
    f.write('  main:\n')
    f.write('    - include: global\n')
    f.write('    - match: \\b({})\\b\n'.format('|'.join(functions)))
    f.write('      scope: support.function\n')
    f.write('    - match: \+|=-\n')
    f.write('      scope: keyword.operator\n')
    f.write('    - match: \,\n')
    f.write('      scope: punctuation.separator\n')
    f.write('    - match: \{\n')
    f.write('      scope: punctuation.section.block.begin\n')
    f.write('    - match: \}\n')
    f.write('      scope: punctuation.section.block.end\n')
    f.write('    - match: \.\w+|\^\w+\n')
    f.write('      scope: variable\n')
    f.write('    - match: //.*|;.*\n')
    f.write('      scope: comment.line\n')


# main
#-------------------------------------------------------------------------------
def main():
    # get all the cpp files
    cpp_files = []
    get_files_recurse(os.path.join('..', 'Code'), cpp_files)

    # gather all the exposed keywords
    functions, properties, directives = parse_files(cpp_files)

    # generate usertype.dat for Visual Studio
    generate_usertype_dat(functions, properties, directives)

    # generate Notepad++ markup xml
    generate_notepad_plusplus_markup_xml(functions, properties, directives)

    # generate sublime markup
    generate_sublime_syntax(functions, properties, directives)

#-------------------------------------------------------------------------------
if __name__ == '__main__':
    main()

#-------------------------------------------------------------------------------
