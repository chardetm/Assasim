#!/bin/bash

# Path to precompilation program
PRECOMPIL="./precompilation"

# Editor you want to use
EDITOR=""

# 0: sync mode (the edition program needs to be closed to continue)
# 1: async mode (the user can trigger step1 and step2 with a key)
# Note: if you use IDE like atom, kdevelop, you may prefer async mode.
MODE=2

if [ "${EDITOR}" = "" ]
then
	echo "Error: you need to configure your editor in the script!"
	exit
fi

if [ $MODE != 0 ] && [ $MODE != 1 ]
then
	echo "Error: you need to configure the mode (sync or async) in the script!"
	exit
fi

if [ $# != 1 ]
then
	echo "Usage: $0 <filename>"
	exit
fi


RETRY=1
FIRST=1
FILE=$(basename "$1")
DIR=$(dirname "$1")
STEP0=1
STEP1=1


if [ -f "${DIR}/${FILE}_step2/${FILE}" ]; then
	echo "Step 2 files already exist. At which step do you wish to begin the development?"
	select step in "beginning" "step1" "build"; do
		case $step in
			beginning ) break;;
			step1 ) STEP0=0; break;;
			build ) STEP0=0; STEP1=0; break;;
		esac
	done
fi

if [ $STEP0 = 1 ] && [ -f "${DIR}/${FILE}_step1/${FILE}" ]; then
	echo "Step 1 files already exist. Are you sure you want to go back from the beginning? (all implementation of Behavior methods will be overwrittent)"
	select yn in "yes" "no"; do
		case $yn in
			yes ) break;;
			no ) STEP0=0; break;;
		esac
	done
fi

if [ $STEP0 = 1 ]; then
	echo "#pragma once" > "${DIR}/agent.hpp"
	echo "#define \$critical" >> "${DIR}/agent.hpp"
	echo "class Agent {};" >> "${DIR}/agent.hpp"

	echo "#pragma once" > "${DIR}/interaction.hpp"
	echo "class Interaction {};" >> "${DIR}/interaction.hpp"
	
	if [ -f "$1" ]; then
		echo "Warning: file $1 already exists, do you want to create a new model or edit the existing one?"
		select ne in "new" "edit"; do
			case $ne in
				new ) echo "#ifndef ${FILE}_INCLUDED_" > $1;
					  echo "#define ${FILE}_INCLUDED_" >> $1;
					  echo "" >> $1;
					  echo "#include \"interaction.hpp\"" >> $1;
					  echo "#include \"agent.hpp\"" >> $1;
					  echo "" >> $1;
					  echo "/* Your model here */" >> $1;
					  echo "" >> $1;
					  echo "#endif" >> $1;
					  break;;
				edit ) break;;
			esac
		done
	else
		echo "#ifndef ${FILE}_INCLUDED_" > $1;
		echo "#define ${FILE}_INCLUDED_" >> $1;			  
		echo "" >> $1;
		echo "#include \"interaction.hpp\"" >> $1;
		echo "#include \"agent.hpp\"" >> $1;
		echo "" >> $1;
		echo "/* Your model here */" >> $1;
		echo "" >> $1;
		echo "#endif" >> $1;
	fi
fi

while [ $RETRY = 1 ] && [ $STEP0 = 1 ]; do
	echo "-- Launching editing program --"
	echo "Enter the structure of the model (interactions and agents' attributes)"
	sleep 1
	
	if [ $MODE = 0 ]; then
			$EDITOR $1
	else
		if [ $FIRST = 1 ]; then
			$EDITOR $1 2>&1 >/dev/null &
		fi
		read -p "Enter a key to trigger step1"
	fi
	
	echo "-- Executing step1 --"
	$PRECOMPIL -step1 -out-to-folder "${FILE}_step1" "$1"

	echo "Do you wish to go back to edit your code (if there are errors for example)"
	select yn in "y" "n"; do
		case $yn in
			y ) break;;
			n ) RETRY=0; break;;
		esac
	done
	FIRST=0
done

RETRY=1
FIRST=1

while [ $RETRY = 1 ] && [ $STEP1 = 1 ]; do
	echo "-- Launching editing program --"
	echo "Enter the implementation of agents' Behavior"
	sleep 1
	if [ $MODE = 0 ]; then
		$EDITOR "${DIR}/${FILE}_step1/${FILE}" "${DIR}/${FILE}_step1/behaviors.cpp"
	else
		if [ $FIRST = 1 ]; then
			$EDITOR "${DIR}/${FILE}_step1/${FILE}" "${DIR}/${FILE}_step1/behaviors.cpp" 2>&1 >/dev/null &
		fi
		read -p "Enter a key to trigger step2"
	fi
	
	echo "-- Executing step2 --"
	$PRECOMPIL -step2 -out-to-folder "../${FILE}_step2" -model-file "${FILE}" "${DIR}/${FILE}_step1/behaviors.cpp"

	echo "Do you wish to go back to edit your code (if there are errors for example)"
	select yn in "y" "n"; do
		case $yn in
			y ) break;;
			n ) RETRY=0; break;;
		esac
	done
	FIRST=0
done

mkdir "${DIR}/${FILE}_build"
cd "${DIR}/${FILE}_build"
cmake "../${FILE}_step2"
make -j3
