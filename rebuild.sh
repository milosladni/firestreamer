#!/bin/bash
# color output constants
GREEN='\033[0;32m'
LGREEN='\033[1;32m'
ORANGE='\033[0;33m'
LORANGE='\033[1;33m'
BLUE='\033[0;34m'
LBLUE='\033[1;34m'
RED='\033[0;31m'
LRED='\033[1;31m'
NC='\033[0m' # No Color

# Check input options
PROGRAM_NAME=''
SB2=false
COPY_BUILD=false
NOCLEAN=false
ANDROID=false
LIBS=''
PJBASE=''
FLAGS=''

PATH=$PATH:/home/milos/sb2/bin/

usage="${LBLUE}Generic script for building applications for Linux/Windows/Android.${NC}\n
        Usage options:\n
        ${LORANGE}  -p${NC}, ${ORANGE}--prname${NC} \t Set program name (e.g. enddevice)\n
        ${LORANGE}  -s${NC}, ${ORANGE}--sb2${NC} \t Use SB2 (scratchbox2) for building\n
        ${LORANGE}  -c${NC}, ${ORANGE}--cp${NC} \t Copy build file to test device (e.g. 192.168.1.125)\n
        ${LORANGE}  -l${NC}, ${ORANGE}--libs${NC} \t Add dynamic or static libraries (e.g. '-lwebsockets -l:libssl.a -l:libcrypto.a')\n
        ${LORANGE}  -f${NC}, ${ORANGE}--flags${NC} \t Add cflags (e.g. '-mthumb -pthread -fpic')\n
        ${LORANGE}  -a${NC}, ${ORANGE}--android${NC} \t Build for Android platform\n
        ${ORANGE}  --pjbase${NC} \t Add PJSIP base path (e.g. /home/A20/pjproject-2.4)\n
        ${LORANGE}  -n${NC}, ${ORANGE}--noclean${NC}\t Build without clean\n
        ${LORANGE}  -h${NC}, ${ORANGE}--help${NC} \t Display this help and exit\n"

OPTS=`getopt -o p:,s,c:,l:,f:,a,n,h --long sb2,cp:,libs:,flags:,android,prname:,pjbase:,noclean,help -n 'parse-options' -- "$@"`

if [ $? != 0 ] ; then echo "Failed parsing options." >&2 ; echo -e $usage; exit 1 ; fi
eval set -- "$OPTS"
while true; do
  case "$1" in
    -p | --prname ) PROGRAM_NAME=$2; shift; shift ;;
    -s | --sb2 )  SB2=true; shift ;;
    -c | --cp ) DEVICE=$2; COPY_BUILD=true; shift; shift ;;
    -l | --libs ) LIBS=$2; shift; shift ;;
    -f | --flags ) FLAGS=$2; shift; shift ;;
    -a | --android )  ANDROID=true; shift ;;
    --pjbase ) PJBASE=$2; shift; shift ;;
    -n | --noclean )  NOCLEAN=true; shift ;;
    -h | --help )   echo -e $usage; exit 0 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

# Check if program name is set
if [ -z $PROGRAM_NAME ]; then
    printf "${LRED}- Error:${RED} program name is not set${NC}\n"
    exit 1
fi

# Write additional flags to user.mak
echo 'PROGRAM_NAME='$PROGRAM_NAME > user.mak
if [ ! -z $PJBASE ]; then
    echo -e '\nPJBASE='${PJBASE} >> user.mak
    echo 'include $(PJBASE)/build.mak' >> user.mak
    echo 'CC = $(PJ_CC)' >> user.mak
    echo 'LDFLAGS = $(PJ_LDFLAGS)' >> user.mak
    echo 'LDLIBS = $(PJ_LDLIBS)' >> user.mak
    echo 'CFLAGS = $(PJ_CFLAGS)' >> user.mak
fi
echo 'LIBS += '${LIBS} >> user.mak
echo 'CFLAGS += '${FLAGS} >> user.mak
if [ "$ANDROID" = true ] ; then
    echo 'SYSTEMANDROID = true' >> user.mak
fi

# Build program
if [ "$SB2" = true ] ; then
    printf "${LGREEN}- Build:${GREEN} $PROGRAM_NAME using SB2...${NC}\n"
    MAKE='sb2 -t gcc4.7 make'
else
    printf "${LGREEN}- Build:${GREEN} $PROGRAM_NAME native...${NC}\n"
    MAKE='make'
fi
if [ "$NOCLEAN" = false ] ; then
    $MAKE clean
fi
if $MAKE -j4 DEBUG="false" PROGRAM_NAME=$PROGRAM_NAME > /dev/null; then
    printf "${LGREEN}- Build:${LBLUE} OK, $PROGRAM_NAME -> "
    stat --printf="%s" $PROGRAM_NAME
    printf " bites\n${NC}"
else
    printf "${LRED}- Error:${RED} building $PROGRAM_NAME${NC}\n"
    exit 1
fi


# Copy image to test device
if [ "$COPY_BUILD" = true ] ; then
    ssh root@$DEVICE killall run_$PROGRAM_NAME.sh $PROGRAM_NAME > /dev/null 2>&1
    if scp -q $PROGRAM_NAME root@$DEVICE:/root/$PROGRAM_NAME/; then
        printf "${LGREEN}- Copy:${GREEN} $PROGRAM_NAME to $DEVICE:/root/$PROGRAM_NAME/$PROGRAM_NAME \tOK${NC}\n"
    else
        printf "${LRED}- Error:${RED} Copy $PROGRAM_NAME to $DEVICE${NC}\n"
        exit 1
    fi
fi

# Build completed
exit 0











