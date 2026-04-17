# === Stage 1: Builder ===
# Use Debian Bookworm to match distroless cc-debian12 glibc (2.36)
FROM debian:bookworm AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    zlib1g-dev \
    libc-ares-dev \
    libjsoncpp-dev \
    libsndfile-dev \
    uuid-dev \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
      -DBUILD_TESTING=OFF \
      -DBEEPBOX_BUILD_SERVER=ON \
  && cmake --build build --target beepbox-server -j"$(nproc)"

# Collect runtime shared library dependencies
RUN mkdir -p /deps && \
    ldd build/beepbox-server | awk '/=>/ {print $3}' | \
    grep -v '^$' | sort -u | \
    while read lib; do cp -L "$lib" /deps/; done

# === Stage 2: Runtime ===
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/* \
  && groupadd -r beepbox && useradd -r -g beepbox -u 65532 beepbox \
  && mkdir -p /app/logs /app/uploads/tmp \
  && chown -R beepbox:beepbox /app

COPY --from=builder /src/build/beepbox-server /app/beepbox-server
COPY --from=builder /deps/ /usr/lib/

WORKDIR /app
EXPOSE 8080

USER beepbox

ENTRYPOINT ["./beepbox-server"]
