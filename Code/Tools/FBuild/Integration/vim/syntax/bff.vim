" Vim syntax file
" Language:     FASTBuild
" Maintainer:   Arvid Gerstmann <ag AT arvid DOT io>
" Created:      2016 Sep 12
" Last Change:  2016 Sep 14
" Revisions:
"   - 1.0: Initial release

" Quit when a syntax file is already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" keywords
syn keyword bffKeyword Alias Compiler Copy CopyDir CSAssembly DLL Exec
syn keyword bffKeyword Executable ForEach Library ObjectList Print RemoveDir
syn keyword bffKeyword Settings Test Unity Using VCXProject VSSSolution XCodeProject

" preprocessor
syn region bffPreproc start="^\s*#\s*\(if\|endif\)\>" skip="\\$" end="$" keepend
syn region bffPreproc start="^\s*#\s*\(define\|undef\)\>" skip="\\$" end="$" keepend
syn region bffPreproc start="^\s*#\s*\(import\|once\)\>" skip="\\$" end="$" keepend

syn region bffIncluded contained start=+"+ skip=+\\\\\|\\"+ end=+"+
syn match  bffIncluded contained "<[^>]*>"
syn match  bffInclude "^\s*#\s*include\>\s*["<]" contains=bffIncluded

" strings
syn match  bffEscape display contained "\^.\|%[1-3]"
syn region bffString start=+L\="+ skip=+\\\\\|\^"+ end=+"+ contains=bffEscape extend
syn region bffString start=+L\='+ skip=+\\\\\|\^'+ end=+'+ contains=bffEscape extend

" comments
syn keyword bffTodo  TODO FIXME XXX contained
syn match bffComment ";.*"  contains=bffTodo
syn match bffComment "//.*" contains=bffTodo

" variables / identifiers
syn match bffIdentifier "\.[a-zA-Z_]\+\([a-zA-Z_]\|[0-9]\)*"

" Actual highlighting
if version >= 508 || !exists("bff_did_init")
    if version < 508
        let bff_did_init = 1
        command -nargs=+ HiLink hi link <args>
    else
        command -nargs=+ HiLink hi def link <args>
    endif

    HiLink bffTodo          Todo
    HiLink bffComment       Comment
    HiLink bffKeyword       Statement
    HiLink bffPreproc       PreProc
    HiLink bffString        String
    HiLink bffInclude       PreProc
    HiLink bffIncluded      String
    HiLink bffEscape        Special
    HiLink bffIdentifier    Identifier

    delcommand HiLink
endif

let b:current_syntax = "bff"

