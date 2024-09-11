#!/bin/bash

#
# cmake-bootstrap.sh
#
#  Take a configured source tree, and generate a cmake build system
#  from the makefiles generated by autotools. Useful for generating
#  IDE project files.
#
# Written by
#  David Hogan <david.q.hogan@gmail.com>
#
# This file is part of VICE, the Versatile Commodore Emulator.
# See README for copyright notice.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307  USA.
#

set -o errexit
set -o nounset
set -o allexport

function hash_all_makefiles() {
	echo -n "$(find . -name Makefile -exec cat {} \; | openssl dgst -md5 -binary | xxd -p)"
}

MAKEFILE_HASH=$(hash_all_makefiles)

# Has CMake previously generated a project into pwd?
if [ -e CMakeCache.txt ]
then
	# Did CMake overwrite the original Makefiles?
	if grep CMAKE_GENERATOR: CMakeCache.txt | grep -q Makefile
	then
		#
		# It would have. But, perhaps they have since been regenerated by configure, or
		# restored using something like a local git repo. Check for these things before
		# aborting.
		#

		>&2 echo -n "CMake previously replaced the Makefiles in this folder"

		if	[ config.log -nt CMakeCache.txt ] || [ "$MAKEFILE_HASH" == "$(cat .cmake_bootstrap_makefile_hash)" ]
		then
			>&2 echo " but it appears that they have been regenerated by configure or restored."
		else
			>&2 echo ". Please re-run configure and consider using a separate cmake build folder. Delete or rename CMakeCache.txt to bypass this check."
			exit 1
		fi
	fi
fi

# Remove any previous run
find . -type f -name 'CMakeLists.txt' -exec rm {} \;
find . -type f -name 'CMakeCache.txt' -exec rm {} \;
find . -type f -name 'cmake_install.cmake' -exec rm {} \;
find . -type d -name 'CMakeFiles' | xargs -IQQQ rm -rf "QQQ"
find . -type d -name '.cmake_bootstrap_cache' | xargs -IQQQ rm -rf "QQQ"

echo -n "$MAKEFILE_HASH" > .cmake_bootstrap_makefile_hash

# Extracting values out of makefiles is a heavy operation, so we use a filesystem
# cache to optimise the process. At the end of this script, remove the cache.
function cleanup {
    # Get rid of our cached extract_make_var results
	find . -type d -name '.cmake_bootstrap_cache' | xargs -IQQQ rm -rf "QQQ"
}
trap cleanup EXIT

# Initialise user variables if not set
if [ -z ${CFLAGS+x} ]
then
	CFLAGS=""
fi

if [ -z ${CXXFLAGS+x} ]
then
	CXXFLAGS=""
fi

if [ -z ${OBJCFLAGS+x} ]
then
	OBJCFLAGS=""
fi

if [ -z ${LDFLAGS+x} ]
then
	LDFLAGS=""
fi

if which parallel > /dev/null; then
	USE_PARALLEL=true
	HOW_PARALLEL=100%
else
	echo "NOTE: Install GNU parallel for faster execution"
	USE_PARALLEL=false
fi

# Quiet versions of pushd and popd
function pushdq {
	pushd $1 > /dev/null
}

function popdq {
	popd > /dev/null
}

function unique_preserve_order {
	awk '!x[$0]++'
}

function indent_file_list {
	sed '2,$s/^[[:space:]]*/        /'
}

function space {
	echo -n ' '
}

function extract_make_var {
	local varname=$1

	if [ -d .cmake_bootstrap_cache ]; then
		# If we already extracted this, use the cached version
		if [ -f .cmake_bootstrap_cache/$1 ]; then
			cat .cmake_bootstrap_cache/$1
			return
		fi
	else
		mkdir .cmake_bootstrap_cache
	fi

	#
	# Sadly make --eval doesn't work in macOS, so we simulate a
	# make --eval='extract_make_var: ; @echo TESTS $(TESTS)' extract_make_var
	# by modifying a copy of the Makefile.
	#

	TMP_MAKEFILE=$(mktemp -t cmake_bootstrap_Makefile.XXXXXXXX)
	cp Makefile $TMP_MAKEFILE

	echo -e "\nextract_make_var:\n\t@echo \$($varname)" >> $TMP_MAKEFILE
	local result=$(make -f $TMP_MAKEFILE extract_make_var)
	rm $TMP_MAKEFILE
	
	# cache for next time
	TMP_CACHEFILE=$(mktemp -t cmake_bootstrap_extracted_make_var.XXXXXXXX)
	echo -n "$result" > $TMP_CACHEFILE
	mv $TMP_CACHEFILE .cmake_bootstrap_cache/$1

	echo -n "$result"
}

function extract_include_dirs {
	local vpath="/$ROOT_VPATH"
	if [ "$vpath" == "/" ]
	then
		vpath=""
	fi

	(extract_make_var AM_CPPFLAGS; space; extract_make_var VICE_CFLAGS; space; extract_make_var VICE_CXXFLAGS) \
		| sed $'s/ -/\\\n-/g' \
		| grep '^-I' \
		| sed 's/^-I//' \
		| sed 's/^[[:space:]]*//' \
		| sed 's/[[:space:]]*$//' \
		| sed "s#^\\..*/vice/src\$#\\\${CMAKE_SOURCE_DIR}$vpath/src#" \
		| sed "s#^\\..*/vice/src/#\\\${CMAKE_SOURCE_DIR}$vpath/src/#" \
		| sed "s#^\\..*\\./src\$#\\\${CMAKE_SOURCE_DIR}$vpath/src#" \
		| sed "s#^\\..*\\./src/#\\\${CMAKE_SOURCE_DIR}$vpath/src/#" \
		| sed -n "p; s#^\\\${CMAKE_SOURCE_DIR}$vpath#\\\${CMAKE_SOURCE_DIR}#p" \
		| unique_preserve_order \
		| indent_file_list
}

function extract_link_dirs {
	(extract_make_var AM_LDFLAGS; space; extract_make_var VICE_LDFLAGS; ) \
		| sed $'s/ -/\\\n-/g' | grep '^-L' | sed 's/^-L//' | unique_preserve_order | tr "\n" " "
}

function extract_c_compile_definitions {
	extract_make_var COMPILE \
		| sed $'s/ -/\\\n-/g' | grep '^-D' | sed -e 's/^-D//g' | tr "\n" " "
}

function extract_cxx_compile_definitions {
	extract_make_var CXXCOMPILE \
		| sed $'s/ -/\\\n-/g' | grep '^-D' | sed -e 's/^-D//g' | tr "\n" " "
}

function extract_objc_compile_definitions {
	extract_make_var OBJCCOMPILE \
		| sed $'s/ -/\\\n-/g' | grep '^-D' | sed -e 's/^-D//g' | tr "\n" " "
}

function extract_c_cxx_objc_flags {
	local flags=""

	#
	# We handling include dirs and definitiions directly, so
	# we exclude these from the extracted cflags/cxxflags.
	# Also don't include macos sdk target flags to avoid
	# lots of warnings when linking to newer deps.
	#

	while (( "$#" )); do
		case "$1" in
			-I*)
				shift 
				;;
			-framework)
				shift 2
				;;
			-Wl,-framework,*)
				shift
				;;
			-Wl,-framework)
				shift 2
				;;
			-D*)
				shift
				;;
			-mmacosx-version*)
				shift 
				;;
			*)
				if [ -z "$flags" ]
				then
					flags=$1
				else
					flags="$flags\n$1"
				fi
				shift
				;;
		esac
	done

	#
	# Echo the extracted flags.
	#
	# But also don't warn about using deprecated macOS stuff. We are supporting
	# old versions of macOS so we need to use them.
	#

	echo -n -e "-Wno-deprecated-declarations $flags" | tr "\n" " "
}

function extract_cflags {
	extract_c_cxx_objc_flags $(extract_make_var AM_CFLAGS; space; extract_make_var CFLAGS; echo -n " $CFLAGS")
}

function extract_cxxflags {
	extract_c_cxx_objc_flags $(extract_make_var AM_CXXFLAGS; space; extract_make_var CXXFLAGS; echo -n " $CXXFLAGS")
}

function extract_objcflags {
	extract_c_cxx_objc_flags $(extract_make_var AM_OBJCFLAGS; space; extract_make_var OBJCFLAGS; echo -n " $OBJCFLAGS")
}

function extract_ldflags {
	local executable=$1
	extract_c_cxx_objc_flags \
		$(extract_make_var \
			${executable}_LDFLAGS; space; \
			extract_make_var AM_LDFLAGS; space; \
			extract_make_var LDFLAGS; \
			echo -n " $LDFLAGS")
}

function extract_internal_libs {
	local libs=""

	while (( "$#" )); do
		case "$1" in
			*.a)
				local lib=$(echo $1 | sed -e 's/.*\(lib.*\)\.a/\1/')
				if [ -z "$libs" ]
				then
					libs=$lib
				else
					libs="$libs\n$lib"
				fi
				shift
				;;
			*.o)
				>&2 echo "Error: Attempting to link to .o files is not currently supported ($1)."
				exit 1
				;;
			*)
				shift
				;;
		esac
	done

	echo -n -e "$libs" | tr "\n" " "
}

function extract_external_libs {
	local libs=""

	while (( "$#" )); do
		case "$1" in
			-l*)
				libs="$libs\n$(echo "$1" | sed 's/^-l//')"
				shift 
				;;
			-framework)
				libs="$libs\n$2"
				shift 2
				;;
			-Wl,-framework,*)
				libs="$libs\n$(echo "$1" | sed 's/^-Wl,-framework,//')"
				shift
				;;
			-Wl,-framework)
				libs="$libs\n$(echo "$2" | sed 's/^-Wl,//')"
				shift 2
				;;
			*)
				shift
				;;
		esac
	done

	echo -n -e "$libs" | tr "\n" " "
}

function add_vpath {
	local vpath=$(extract_make_var VPATH)
	while read file
	do
		if [ -f "$file" ]
		then
			echo "$file"
		elif [ -f "$vpath/$file" ]
		then
			echo "$vpath/$file"
		else
			>&2 echo "$(tput bold)Warning, can't find $file in $(pwd)$(tput sgr0)"
		fi
	done
}

function extract_sources {
	extract_make_var $1 \
		| tr " " "\n" \
		| grep '\.\(c\|cc\|cpp\|m\)$' \
		| add_vpath \
		| indent_file_list
}

function extract_object_sources {
	for object_file_basename in $(extract_make_var $1 | tr " " "\n" | grep '\.o$' | sed 's/\.o$//')
	do
		ls -1 "$(source_relative_folder)" \
			| grep "^${object_file_basename}\.\(c\|cc\|cpp\|m\)$" \
			| add_vpath \
			| indent_file_list
	done
}

function extract_headers {
	extract_make_var $1 \
		| tr " " "\n" \
		| grep '\.\(h\|hh\|hpp\)$' \
		| add_vpath \
		| indent_file_list
}

function extract_macos_target_sdk_version {
	extract_make_var VICE_CFLAGS \
		| tr " " "\n" \
		| grep -- '-mmacosx-version-min=' \
		| head -n1 \
		| sed -e 's/-mmacosx-version-min=//'
}

function project_relative_folder {
	local _pwd=$(pwd)
	echo -n "${_pwd#$ROOT_FOLDER/}"
}

function source_relative_folder {
	local vpath=$(extract_make_var VPATH)
	if [ -z "$vpath" ]
	then
		echo -n "."
	else
		echo -n "$vpath"
	fi
}

#
# Recursively work though the configured Makefile tree, defining all the libs
#

ROOT_FOLDER="$(pwd)"
ROOT_VPATH="$(extract_make_var VPATH)"

function process_source_makefile {
	local dir=$1
	
	pushdq $dir

	#
	# Generate any necessary sources, they may be listed
	#

	local generated_sources=$(extract_make_var BUILT_SOURCES)
	if [ ! -z "$generated_sources" ]
	then
		echo "Generating sources in $(project_relative_folder): $generated_sources"
		make -s $generated_sources
	fi

	echo -n "Creating $(project_relative_folder)/CMakeLists.txt ("
	touch CMakeLists.txt

	#
	# Declare each built lib in the original Makefile
	#

	local first=true
	for lib_to_build in $(extract_make_var noinst_LIBRARIES | sed 's/\.a//g')
	do
		if $first
		then
			echo -n "$lib_to_build"
			first=false
		else
			echo -n ", $lib_to_build"
		fi

		cat <<-HEREDOC >> CMakeLists.txt
			add_library($lib_to_build)
			
			target_compile_definitions(
			    $lib_to_build
			    PRIVATE
			        \$<\$<COMPILE_LANGUAGE:C>:$(extract_c_compile_definitions)>
			        \$<\$<COMPILE_LANGUAGE:CXX>:$(extract_cxx_compile_definitions)>
			        \$<\$<COMPILE_LANGUAGE:OBJC>:$(extract_objc_compile_definitions)>
			    )

			target_include_directories(
			    $lib_to_build
			    PRIVATE
			        \${CMAKE_CURRENT_SOURCE_DIR}
			        \${CMAKE_CURRENT_SOURCE_DIR}/$(extract_make_var VPATH)
			        $(extract_include_dirs)
			    )

			target_compile_options(
			    $lib_to_build
			    PRIVATE
			        \$<\$<COMPILE_LANGUAGE:C>:$(extract_cflags)>
			        \$<\$<COMPILE_LANGUAGE:CXX>:$(extract_cxxflags)>
			        \$<\$<COMPILE_LANGUAGE:OBJC>:$(extract_objcflags)>
			    )

			target_sources(
			    $lib_to_build
			    PRIVATE
			        $({
			        extract_sources ${lib_to_build}_a_SOURCES
			        extract_headers ${lib_to_build}_a_SOURCES
			        extract_object_sources ${lib_to_build}_a_DEPENDENCIES
			        extract_object_sources ${lib_to_build}_a_LIBADD
			        extract_sources BUILT_SOURCES
			        extract_headers BUILT_SOURCES
			        extract_headers noinst_HEADERS
					extract_headers EXTRA_DIST
			        } | grep '\S' | unique_preserve_order | indent_file_list)
			    )

		HEREDOC
	done

	echo ")"

	#
	# Let cmake know about subdirs
	#

	for subdir in $(extract_make_var SUBDIRS)
	do
		cat <<-HEREDOC >> CMakeLists.txt
			add_subdirectory($subdir)
		HEREDOC
	done

	popdq
}

function find_all_makefile_dirs {
	local dir=$1
	
	pushdq $dir
	pwd

	for subdir in $(extract_make_var SUBDIRS)
	do
		find_all_makefile_dirs $subdir
	done

	popdq
}

if $USE_PARALLEL; then
	find_all_makefile_dirs src | parallel -j $HOW_PARALLEL process_source_makefile {}
else
	for dir in $(find_all_makefile_dirs src); do
		process_source_makefile $dir
	done
fi

function external_lib_label {
	echo -n "LIB_$(echo "$1" | tr '[a-z]' '[A-Z]' | sed -e 's/[^A-Z0-9_]/_/g')"
}

function generate_executable_target {
	local executable=$1

	#
	# Each executable has its own list of external libs to be linked with.
	#
	
	LIB_ARGS="$(extract_make_var LIBS) $(extract_make_var ${executable}_LDADD)"
	LIB_LIST="$(extract_internal_libs $LIB_ARGS)"

	for lib in $(extract_external_libs $LIB_ARGS)
	do
		local label=$(external_lib_label $lib)
		
		LIB_LIST="$LIB_LIST \${$label}"
	done
	
	cat <<-HEREDOC

		if(FIRST_RUN)
		    set(CMAKE_XCODE_GENERATE_SCHEME ON)
		    add_executable($executable)
		    set(CMAKE_XCODE_GENERATE_SCHEME OFF)
		else()
		    add_executable($executable)
		endif()

		target_compile_definitions(
		    $executable
		    PRIVATE
		        \$<\$<COMPILE_LANGUAGE:C>:$(extract_c_compile_definitions)>
		        \$<\$<COMPILE_LANGUAGE:CXX>:$(extract_cxx_compile_definitions)>
		        \$<\$<COMPILE_LANGUAGE:OBJC>:$(extract_objc_compile_definitions)>
		    )

		target_include_directories(
		    $executable
		    PRIVATE
		        \${CMAKE_CURRENT_SOURCE_DIR}
		        \${CMAKE_CURRENT_SOURCE_DIR}/$(extract_make_var VPATH)
		        $(extract_include_dirs)
		    )
		
		target_compile_options(
		    $executable
		    PRIVATE
		        \$<\$<COMPILE_LANGUAGE:C>:$(extract_cflags)>
		        \$<\$<COMPILE_LANGUAGE:CXX>:$(extract_cxxflags)>
		        \$<\$<COMPILE_LANGUAGE:OBJC>:$(extract_objcflags)>
		    )
		
		target_link_options(
		    $executable
		    PRIVATE
		        $(extract_ldflags $executable)
		    )

		target_sources(
		    $executable
		    PRIVATE
		        $({
		        extract_sources ${executable}_SOURCES
		        extract_headers ${executable}_SOURCES
		        extract_sources EXTRA_${executable}_SOURCES
		        extract_headers EXTRA_${executable}_SOURCES
		        extract_sources BUILT_SOURCES
		        extract_headers BUILT_SOURCES
		        extract_headers noinst_HEADERS
		        } | grep '\S' | unique_preserve_order | indent_file_list)
		    )
		
		target_link_libraries(
		    $executable
		    PRIVATE
		    	$LIB_LIST
		    )
	HEREDOC
}

#
# Filter the list of excutables to those in the Makefile (x64 is optional)
#

POSSIBLE_EMULATORS="x64 x64sc x128 x64dtv xscpu64 xvic xpet xplus4 xcbm2 xcbm5x0 c1541 vsid"
EMULATORS=""

for executable in $POSSIBLE_EMULATORS
do
    if ! grep -q "^$executable:" Makefile
    then
        echo "Executable $executable not configured"
        continue
    fi

    EMULATORS="$EMULATORS $executable"
done

#
# The src folder Makefile also defines all the non-tool executables and what they link to.
#

pushdq src

#
# Find all the libraries first
#

echo >> CMakeLists.txt

for executable in $EMULATORS
do	
	LIB_ARGS="$(extract_make_var LIBS) $(extract_make_var ${executable}_LDADD)"

	for lib in $(extract_external_libs $LIB_ARGS)
	do
		label=$(external_lib_label $lib)

		if ! fgrep -q "find_library($label " CMakeLists.txt
		then
			cat <<-HEREDOC >> CMakeLists.txt
				find_library($label $lib \${CMAKE_C_IMPLICIT_LINK_DIRECTORIES} $(extract_link_dirs))
			HEREDOC
		fi
	done
done

#
# Executable build targets
#

PARALLEL_JOBS=""

for executable in $EMULATORS
do
	if $USE_PARALLEL; then
		PARALLEL_JOBS="${PARALLEL_JOBS}cd $(pwd); >&2 echo \"Emulator: ${executable}\"; generate_executable_target ${executable}\n"
	else
		echo "Emulator: $executable"
		generate_executable_target $executable >> CMakeLists.txt
	fi
done

#
# Tools, executable targets in src/tools/x with simpler linking
#

TOOLS="petcat cartconv"

for executable in $TOOLS
do
	pushdq "tools/$executable"

	if $USE_PARALLEL; then
		PARALLEL_JOBS="${PARALLEL_JOBS}cd $(pwd); >&2 echo \"Tool: ${executable}\"; generate_executable_target ${executable} >> CMakeLists.txt\n"
	else
		echo "Tool: $executable"
		generate_executable_target $executable >> CMakeLists.txt
	fi

	popdq
done

if $USE_PARALLEL; then
	echo -e "$PARALLEL_JOBS" | parallel -j $HOW_PARALLEL --no-run-if-empty >> CMakeLists.txt
fi

popdq

#
# Finally, create the top level project CMakeLists.txt
#

echo "Creating top level CMakeLists.txt"

cat <<-HEREDOC > CMakeLists.txt
	cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

HEREDOC

if [[ "$OSTYPE" == "darwin"* ]]; then
	cat <<-HEREDOC >> CMakeLists.txt
		set(CMAKE_OSX_SYSROOT "$(xcrun --show-sdk-path)")
		set(CMAKE_OSX_DEPLOYMENT_TARGET "$(extract_macos_target_sdk_version)" CACHE STRING "Minimum OS X deployment version")
	HEREDOC
	LANGUAGES="C CXX OBJC"
else
	LANGUAGES="C CXX"
fi

cat <<-HEREDOC >> CMakeLists.txt
	set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS cc;cpp)
	set(CMAKE_CXX_STANDARD 11)

	if(NOT FIRST_RUN_COMPLETE)
	    set(FIRST_RUN true)
	endif()

	project(VICE $LANGUAGES)
	set(CMAKE_STATIC_LIBRARY_PREFIX "")

	add_subdirectory(src)

	set(FIRST_RUN_COMPLETE true CACHE INTERNAL "Used to detect first run")
HEREDOC

echo "Done."

echo -e "\nCMake can now be used to generate an alternative build system. Example usage:"

echo -e "\n\t# Generate a Unix Makefiles project in current working directory:"
echo -e   "\t# (Note: this replaces the original configured Makefiles)"
echo -e   "\tcmake ."

echo -e "\n\t# Generate a Unix Makefiles project in a build directory:"
echo -e   "\tcmake -B <build directory>"

echo -e "\n\t# Or:"
echo -e   "\tmkdir <build directory>"
echo -e   "\tcd <build directory>"
echo -e   "\tcmake .."

echo -e "\n\t# Generate an Xcode project in a build directory:"
echo -e   "\tcmake -G Xcode -B <path to build directory>"

echo -e "\nRun cmake --help to see a list of supported project generators on this platform.\n"
