#!/bin/bash

_fbuild() {
	local cur="${COMP_WORDS[COMP_CWORD]}"
	local prev="${COMP_WORDS[COMP_CWORD-1]}"
	local opts="
		-cache
		-cachecompressionlevel
		-cacheinfo
		-cacheread
		-cachetrim
		-cacheverbose
		-cachewrite
		-clean
		-compdb
		-config
		-continueafterdbmove
		-dist
		-distverbose
		-dot
		-dotfull
		-fixuperrorpaths
		-forceremote
		-help
		-ide
		-j
		-monitor
		-nofastcancel
		-nolocalrace
		-noprogress
		-nostoponerror
		-nosummaryonerror
		-nounity
		-profile
		-progress
		-quiet
		-report
		-showalltargets
		-showcmdoutput
		-showcmds
		-showdeps
		-showtargets
		-summary
		-verbose
		-version
		-vs
		-wait
		-why
		-wrapper
	"

	case ${prev} in
	-cachecompressionlevel)
		# Offer supported compression levels after -cachecompressionlevel
		COMPREPLY=( $(compgen -W "{-128..12}" -- "${cur}" ) )
		return 0
		;;
	-cachetrim)
		# Offer nothing after -cachetrim to indicate that this option requires an argument
		return 0
		;;
	-config)
		# Offer files after -config
		COMPREPLY=( $(compgen -f -- "${cur}") )
		[[ ${BASH_VERSINFO[0]} -ge 4 ]] && compopt -o filenames
		return 0
		;;
	esac

	# Offer number of cores on this machine after -j
	case ${cur} in
	-j*)
		local ncpus=8
		declare -f _ncpus >/dev/null && ncpus=$(_ncpus)
		COMPREPLY=( $(compgen -W "-j{1..${ncpus}}" -- "${cur}" ) )
		return 0
		;;
	esac

	# Search for -config option to pass to fbuild to get the list of targets
	local args=()
	for (( i=0; i < ${#COMP_WORDS[@]}; i++ )); do
		if [[ "${COMP_WORDS[i]}" == -config ]]; then
			args=( -config "${COMP_WORDS[i+1]}" )
		fi
	done

	# Get available targets. Works only if config is valid, otherwise produces nothing.
	local targets="$("${COMP_WORDS[0]}" "${args[@]}" -showtargets | sed -n -e '/List of available targets/,$ {/List of available targets/d; p}')"

	COMPREPLY=( $( compgen -W "${opts} ${targets}" -- "${cur}" ) )
}

complete -F _fbuild fbuild
complete -F _fbuild FBuild
case "$(uname)" in
MINGW32*|MSYS*|CYGWIN*)
	complete -F _fbuild fbuild.exe
	complete -F _fbuild FBuild.exe
	;;
esac
