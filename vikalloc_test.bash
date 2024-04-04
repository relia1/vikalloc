#!/bin/bash 

function showHelp {
    # The following is called a "here file."
    cat <<EOF
This script can be used to test ${LAB}.
By default, all tests will be run.
Command line options:
    -h   : Show this helpful text and exit.
           Have a good day.
    -c   : By deafult, all the temporary files created during the script
           are deleted, and there are a bunch of files. If you want to
           keep them around for some debugging, use this option.
    -s # : Change the value used as the multiple for calls the sbrk(). The
           default value for this in ${PROG_BASE} is 1024. This should be
           a multiple of 128.

If you experience problems with this script, please let me know. Send me email
     ${OWNER_EMAIL}

This is a testing script, but there may be things sooooo bad that it does not
catch them. Doing well with the script does not guarantee success on the lab.

EOF

    exit 0
}

function signalCaught {
    echo "++++ caught signal while running script ++++"
}

function signalCtrlC {
    echo "Caught Control-C"
    echo "You will neeed to clean up some files"
    exit
}

function signalSegFault {
    echo "+++++++ Caught Segmentation Fault from your program! OUCH!  ++++++++"
}

function coreDumpMessage {
    if [ $1 -eq 139 ]
    then
        echo "      *** core dump during $2 ***"
        ((CORE_COUNT++))
    elif [ $1 -eq 137 ]
    then
        echo "      *** core dump during $2 ***"
        ((CORE_COUNT++))
    elif [ $1 -eq 134 ]
    then
        echo "      *** abort during $2 ***"
        ((CORE_COUNT++))
    elif [ $1 -eq 124 ]
    then
        echo "      *** timeout during $2 ***"
    #else
        #echo "$1 is a okay"
    fi
    sync
}

function run_test {
    JFILE=j_test_${1}.out
    SFILE=s_test_${1}.out

    ${JPROG} -t ${1} ${MALSIZE} -o ${JFILE} 2>&1 > /dev/null
    RESULT=$(timeout ${TOS} ${TO} ${SPROG} ${MALSIZE} -o${SFILE} -t ${1} 2>&1 > /dev/null | grep -e "timeout\|core")
    TIMEOUT=$(echo $RESULT | grep "timeout")
    CORE=$(echo $RESULT | grep "dumped core")
    if [ ! -z "$CORE" ]
    then
        coreDumpMessage 139 "test ${1}"
        return
    fi
    if [ ! -z "$TIMEOUT" ]
    then
        coreDumpMessage 124 "test ${1}"
        return
    fi

    diff -q ${JFILE} ${SFILE} 2>&1 > /dev/null
    SUCCESS=$?
    if [ ${SUCCESS} -eq 0 ]
    then
        if [ ${2} -eq 0 ]
        then
            echo "    Test ${1} passed. This is fantastic!!! Keep going..."
        else
            ((LAB_GRADE+=${2}))
            echo "    Test ${1} passed. Adding ${2} points, for ${LAB_GRADE} of ${TOTAL_POINTS} so far."
        fi
    else
        echo "    *** Test ${1} did not match. Run test individually to see why. ***"
    fi
}

function buildPart {
    echo "    Trying \"make $1\""

    rm -f $1
    make clean > /dev/null 2>&1
    make $1 > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        echo "      >> Failed \"make $1\""
        echo "         Missing the $1 target in Makefile or too many errors during compilation."
    fi
}

function build {
    BuildFailed=0

    echo -e "\nTesting ${FUNCNAME} (the Makefile)"

    if [ ! -f Makefile -a ! -f makefile ]
    then
        echo "      *** There is no makefile or Makefile. This is bad. ***"
        echo "      ***   Nothing can be built, all tests fail.        ***"
        echo "      ***                                                ***"
        BuildFailed=1
        return
    fi

    make clean > /dev/null
    if [ $? -eq 0 ]
    then
        echo "    Passed \"make clean\" A good start. Keep it up."
    else
        echo "    >> Failed \"make clean\""
        echo "       Missing the clean target in Makefile?"
    fi

    for TARG in ${PROG_BASE}.o ${PROG_BASE} all
    do
        buildPart ${TARG}
    done

    if [ ! -x ${PROG_BASE} ]
    then
        echo "      *** Build of ${PROG_BASE} failed, all tests fail. ***"
        BuildFailed=1
        return
    fi

    make clean > /dev/null 2>&1
    make vikalloc 2> WARN.err > WARN.out

    NUM_BYTES=`wc -c < WARN.err`
    if [ ${NUM_BYTES} -eq 0 ]
    then
        echo "      You had no warning messages. Excellent!"
    else
        echo "      *** You have warning messages. Bummer, a 20% deduction. ***"
    fi

    for FLAG in -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wextra \
        -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
        -Wdeclaration-after-statement -Wunsafe-loop-optimizations
    do
    echo "    Looking for \"${FLAG}\" in build log"

    grep -c -- ${FLAG} WARN.out > /dev/null
    if [ $? -ne 0 ]
    then
        echo "      >> Failed gcc flag ${FLAG}"
        echo "         Missing from compiler flags; ${FLAG}"
    fi
    done

    if [ ${DO_CLEANUP} -eq 1 ]
    then
        rm -f ./WARN*
    fi

    if [ ! -x ${PROG} ]
    then
        echo "      *** The ${PROG} program failed to build. This is bad. ***"
        echo "      ***   VERY VERY BAD                                    ***"
        BuildFailed=1
    fi
}

function first_fit_tests()
{
    if [ ${TEST_NUM} -eq 0 ]
    then

        echo "Running first_fit all tests..."
        run_test 1 2
        run_test 2 2
        run_test 3 2
        run_test 4 3
        run_test 5 3

        run_test 6 3
        run_test 7 4
        run_test 8 4
        run_test 9 7
        run_test 10 12

        run_test 11 5
        run_test 12 15
        run_test 13 15
        run_test 14 15
        run_test 15 15
        
        run_test 16 12
        run_test 17 12
        run_test 18 12
        run_test 19 15
        run_test 20 15

        run_test 21 15
        run_test 22 15
        run_test 23 15
        run_test 24 20
        run_test 25 20

        run_test 26 20
        run_test 27 20
        run_test 28 20
        run_test 29 7
        run_test 0 25

        echo "Total points ${LAB_GRADE} out of ${TOTAL_POINTS}."
        echo "  Total does not account for late deductions or compilation warnings."
    else

        echo ""
        echo "Running only test ${TEST_NUM}"
        run_test ${TEST_NUM} 0
        DO_CLEANUP=0
    fi
}

function next_fit_tests()
{
    if [ ${TEST_NUM} -eq 0 ]
    then
        echo "Running next_fit all tests..."
        run_test 1 2
        run_test 2 2
        run_test 3 2
        run_test 4 3
        run_test 5 3

        run_test 6 3
        run_test 7 4
        run_test 8 4
        run_test 9 7
        run_test 10 12

        run_test 11 12
        run_test 12 13
        run_test 13 14
        run_test 14 15
        run_test 15 16
        
        run_test 16 5
        
        run_test 17 12
        run_test 18 12
        run_test 19 15
        run_test 20 15

        run_test 21 8
        run_test 22 8
        run_test 23 8
        
        run_test 24 12
        run_test 25 12
        run_test 26 12
        run_test 27 12
        run_test 28 12
        
        run_test 29 5

        run_test 30 15
        run_test 31 17
        run_test 32 18
        run_test 33 19
        run_test 34 21
        
        run_test 0 30

        echo "Total points ${LAB_GRADE} out of ${TOTAL_POINTS}."
        echo "  Total does not account for late deductions or compilation warnings."
    else

        echo ""
        echo "Running only test ${TEST_NUM}"
        run_test ${TEST_NUM} 0
        echo -e "If the test failed, you can run \n\tdiff j_test_${TEST_NUM}.out s_test_${TEST_NUM}.out"
    fi
}

TO=5s
TOS="-s 5"
LAB=Lab1
CLASS=cs333
JDIR=~rchaney/Classes/${CLASS}/Labs/${LAB}
#JDIR=~/Classes/cs333/Labs/f2022_Labs/Lab1
SDIR=.

LAB_GRADE=0
LAB_TOTAL=0

# for first fit
#TOTAL_POINTS=350
# for next fit
TOTAL_POINTS=380

PROG_BASE=vikalloc
SPROG=${SDIR}/${PROG_BASE}
JPROG=${JDIR}/${PROG_BASE}_complete
#JPROG=./${PROG_BASE}_complete

ln -fs ${JDIR}/${PROG_BASE}_complete .

cp -f ${JDIR}/vikalloc.h .
cp -f ${JDIR}/main.c .
cp -f ${JDIR}/vikalloc_dump.c .

OWNER_EMAIL=rchaney@pdx.edu
MALSIZE=""
DO_CLEANUP=1
TEST_NUM=0

while getopts "hs:ct:" opt
do
    case $opt in
        h)
            showHelp
            ;;
        s)
            MALSIZE="-s ${OPTARG}"
            ;;
        t)
            TEST_NUM=${OPTARG}
            DO_CLEANUP=0
            ;;
        c)
            DO_CLEANUP=0
            ;;
    esac
done

trap 'signalCaught;' SIGTERM SIGQUIT SIGKILL SIGSEGV
trap 'signalCtrlC;' SIGINT
# This is not working correctly on the PSU server.
#trap 'signalSegFault;' SIGCHLD

build
if [ ${BuildFailed} -ne 0 ]
then
    echo "Since the program build failed (using make), ending test."
    echo "Total points ${LAB_GRADE} out of ${TOTAL_POINTS}."
    exit 1
fi

#first_fit_tests
next_fit_tests

if [ ${DO_CLEANUP} -eq 1 ]
then
    echo "Cleaning up"
    rm -f [js]_test_*.out
fi
