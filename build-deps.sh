#!/bin/sh
set -e -x

CURDIR="$PWD"

cat << EOF > configure/RELEASE.local
EPICS_BASE=$HOME/.source/epics-base
PVDATA=$HOME/.source/pvDataCPP
PVACCESS=$HOME/.source/pvAccessCPP
NORMATIVETYPES=$HOME/.source/normativeTypesCPP
EOF
cat configure/RELEASE.local

install -d "$HOME/.source"
cd "$HOME/.source"

git clone --quiet --depth 5 --branch "${BRBASE:-master}" https://github.com/epics-base/epics-base.git epics-base
git clone --quiet --depth 5 --branch "${BRPVD:-master}" https://github.com/epics-base/pvDataCPP.git pvDataCPP
git clone --quiet --depth 5 --branch "${BRPVA:-master}" https://github.com/epics-base/pvAccessCPP.git pvAccessCPP
git clone --quiet --depth 5 --branch "${BRNT:-master}" https://github.com/epics-base/normativeTypesCPP.git normativeTypesCPP

(cd epics-base && git log -n1 )
(cd pvDataCPP && git log -n1 )
(cd pvAccessCPP && git log -n1 )
(cd normativeTypesCPP && git log -n1 )

EPICS_HOST_ARCH=`sh epics-base/startup/EpicsHostArch`

# requires wine and g++-mingw-w64-i686
if [ "$WINE" = "32" ]
then
  echo "Cross mingw32"
  sed -i -e '/CMPLR_PREFIX/d' epics-base/configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
  cat << EOF >> epics-base/configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
CMPLR_PREFIX=i686-w64-mingw32-
EOF
  cat << EOF >> epics-base/configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS+=win32-x86-mingw
EOF
fi

case "$CMPLR" in
clang)
  echo "Host compiler is clang"
  cat << EOF >> epics-base/configure/os/CONFIG_SITE.Common.$EPICS_HOST_ARCH
GNU         = NO
CMPLR_CLASS = clang
CC          = clang
CCC         = clang++
EOF

  # hack
  sed -i -e 's/CMPLR_CLASS = gcc/CMPLR_CLASS = clang/' epics-base/configure/CONFIG.gnuCommon

  clang --version
  ;;
*)
  echo "Host compiler is default"
  gcc --version
  ;;
esac

cat << EOF > pvDataCPP/configure/RELEASE.local
EPICS_BASE=$HOME/.source/epics-base
EOF

cat << EOF > pvAccessCPP/configure/RELEASE.local
PVDATA=$HOME/.source/pvDataCPP
EPICS_BASE=$HOME/.source/epics-base
EOF

cat << EOF > normativeTypesCPP/configure/RELEASE.local
PVDATA=$HOME/.source/pvDataCPP
EPICS_BASE=$HOME/.source/epics-base
PVACCESS=$HOME/.source/pvAccessCPP
EOF

make -j2 -C epics-base
make -j2 -C pvDataCPP
make -j2 -C pvAccessCPP
make -j2 -C normativeTypesCPP
