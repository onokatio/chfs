FROM spack/ubuntu-jammy:latest as spack

RUN apt-get update \
 && apt-get -y install gcc automake cmake libtool pkgconf

RUN mkdir /opt/spack-environment \
&&  (echo "spack:" \
&&   echo "  specs:" \
&&   echo "  - mochi-margo ^mercury~boostsys" \
&&   echo "  concretizer:" \
&&   echo "    unify: true" \
&&   echo "  config:" \
&&   echo "    install_tree: /opt/software" \
&&   echo "  view: /opt/view") > /opt/spack-environment/spack.yaml

# Install the software, remove unnecessary deps
RUN cd /opt/spack-environment && spack env activate . \
 && spack external find automake autoconf libtool cmake gmake m4 pkgconf libfuse \
  && spack install -v -j 4 --fail-fast && spack gc -y

# Strip all the binaries
RUN find -L /opt/view/* -type f -exec readlink -f '{}' \; | \
    xargs file -i | \
    grep 'charset=binary' | \
    grep 'x-executable\|x-archive\|x-sharedlib' | \
    awk -F: '{print $1}' | xargs strip -s

# Modifications to the environment that are necessary to run
RUN cd /opt/spack-environment && \
    spack env activate --sh -d . >> /etc/profile.d/z10_spack_environment.sh

FROM ubuntu:22.04 as build

RUN apt-get update \
 && apt-get -y install libpmemkv-dev libmemkind-dev libtbb-dev \
 rapidjson-dev \
 libfuse-dev pandoc \
 libopenmpi-dev libssl-dev

#RUN cd \
# && mkdir local \
 #&& git clone -c feature.manyFiles=true --depth 1 https://github.com/spack/spack.git \
 #&& . spack/share/spack/setup-env.sh \
# && git clone https://github.com/mochi-hpc/mochi-spack-packages.git \
# && spack repo add mochi-spack-packages \
 #&& spack external find automake autoconf libtool cmake m4 pkgconf bison \
 #&& spack install mochi-margo ^mercury~boostsys ^libfabric fabrics=rxm,sockets,tcp,udp \
# && spack install mochi-abt-io \
# && printf '%s\n' \
#    'export LC_ALL=C' \
#    '. $HOME/spack/share/spack/setup-env.sh' \
#    'export PATH=$HOME/local/bin:$PATH' \
#    'export LD_LIBRARY_PATH=$HOME/local/lib:$LD_LIBRARY_PATH' \
#    'export PKG_CONFIG_PATH=$HOME/local/lib/pkgconfig:$PKG_CONFIG_PATH' \
#    >> .bashrc

COPY --from=spack /opt/spack-environment /opt/spack-environment
COPY --from=spack /opt/software /opt/software
COPY --from=spack /opt/view /opt/view
COPY --from=spack /etc/profile.d/z10_spack_environment.sh /etc/profile.d/z10_spack_environment.sh

#ENTRYPOINT ["/bin/bash", "--rcfile", "/etc/profile", "-l"]
ENV PATH=/opt/view/bin:/opt/spack/bin:$PATH
ENV PKG_CONFIG_PATH=/opt/view/lib/pkgconfig:/opt/view/share/pkgconfig:/opt/view/lib64/pkgconfig:$PKG_CONFIG_PATH
ENV CMAKE_PREFIX_PATH=/opt/view
ENV ACLOCAL_PATH=/opt/view/share/aclocal

COPY . /chfs
RUN  cd /chfs && autoreconf -i && ./configure && make
