FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential git ca-certificates python3 python3-pip \
        libgl1 libegl1 libglib2.0-0 libdbus-1-3 libfontconfig1 libxkbcommon0 \
        libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 libxcb-keysyms1 \
        libxcb-shape0 libxcb-xinerama0 fonts-dejavu-core ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu 22.04 ships CMake 3.22; CMakePresets version 6 needs 3.25+.
RUN pip3 install --no-cache-dir aqtinstall "cmake>=3.25" ninja

RUN for attempt in 1 2 3 4; do \
        aqt install-qt --timeout 60 -b https://download.qt.io linux desktop 6.8.2 linux_gcc_64 -O /opt/Qt && exit 0; \
        echo "aqt attempt $attempt failed, retrying"; sleep 10; \
    done; exit 1

ENV CMAKE_PREFIX_PATH=/opt/Qt/6.8.2/gcc_64
ENV QT_QPA_PLATFORM=offscreen

WORKDIR /src
COPY . .

RUN cmake --preset dev && cmake --build --preset dev

CMD ["ctest", "--preset", "dev", "--output-on-failure"]
