FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        python3 \
        pkg-config

WORKDIR /tmwa

COPY . /tmwa

RUN cmake -B build . -DCMAKE_INSTALL_PREFIX=/usr \
                     -DCMAKE_INSTALL_SYSCONFDIR=/etc \
                     -DCMAKE_INSTALL_LOCALSTATEDIR=/var && \
    cmake --build build -j$(nproc) && \
    DESTDIR=/tmwa/install cmake --install build

# Create a minimal runtime image
FROM ubuntu:24.04

# Create a non-root user and group for running the server
RUN useradd -m -d /home/tmwa -s /bin/bash tmwa
VOLUME ["/var/tmwa"]

USER tmwa
WORKDIR /home/tmwa

COPY --from=0 /tmwa/install/ /
