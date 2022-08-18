#!/bin/sh -e

CONTAINER=brot2_build
IMAGE=ubuntu:jammy
CCACHEDIR=${HOME}/.ccache-${CONTAINER}
SRCDIR=$(readlink -f $(dirname $0))
WORKDIR=/work
OUTDIR=${SRCDIR}/out

findit() {
    echo "$(docker ps -q -f name=${CONTAINER} $1)"
}

#MATCH=$(docker ps -a -q -f name=${CONTAINER})
#if [ -z "${MATCH}" ]; then
if [ -z "$(findit -a)" ]; then
    # image does not exist
    docker create --name ${CONTAINER} \
        --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
        --env DEBIAN_FRONTEND=noninteractive \
        --env DPKG_COLORS=always \
        --env FORCE_UNSAFE_CONFIGURE=1 \
        --tty \
        -v ${CCACHEDIR}:/root/.ccache \
        -v ${SRCDIR}:${WORKDIR}/src \
        -v ${OUTDIR}:${WORKDIR}/out \
        -w ${WORKDIR} \
        ${IMAGE} \
        sh -c 'while true; do sleep 60; done'

    if [ -z "$(findit -a)" ]; then
        echo Error: failed to create container ${CONTAINER}
        exit 1
    fi
fi

if [ -n "$(findit)" ]; then
    # image already running
    echo Error: container ${CONTAINER} already running
    exit 1
fi

if [ -z "$1" ]; then
    ARGS=./src/misc/docker-debbuild.sh
else
    ARGS="$@"
fi
docker start ${CONTAINER} >/dev/null
RV=0
docker exec -it ${CONTAINER} ${ARGS} || RV=$?
docker stop -t 0 ${CONTAINER} >/dev/null
exit ${RV}
