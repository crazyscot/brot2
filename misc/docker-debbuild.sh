#!/bin/sh -e

WORKDIR=/work
SRCDIR=${WORKDIR}/src
OUTDIR=${WORKDIR}/out

SUFFIXES=".deb .ddeb .buildinfo .changes"
mkdir -p ${OUTDIR}

for s in ${SUFFIXES}; do rm -vf ${WORKDIR}/*${s}; done

apt-get update
apt-get install -yq dpkg-dev debhelper devscripts ccache
apt-get build-dep -yq ${SRCDIR}

export PATH=/usr/lib/ccache:"${PATH}"

( cd ${SRCDIR} ; dpkg-buildpackage -b )
for s in ${SUFFIXES}; do find ${WORKDIR} -maxdepth 1 -name *${s} -type f -execdir mv -v \{\} ${OUTDIR} \; ; done

echo lintian...
lintian --allow-root ${OUTDIR}/*.deb --fail-on error --fail-on warning
