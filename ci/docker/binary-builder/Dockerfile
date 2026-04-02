# docker build -t clickhouse/binary-builder .
ARG FROM_TAG
ARG CCTOOLS_TAG=${CCTOOLS_TAG-"859fb360308eb8ac47ad"}

FROM clickhouse/cctools:${CCTOOLS_TAG} AS cctools

FROM clickhouse/fasttest:$FROM_TAG
ENV CC=clang-${LLVM_VERSION}
ENV CXX=clang++-${LLVM_VERSION}

# If the cctools is updated, then first build it in the CI, then update here in a different commit
COPY --from=cctools /cctools /cctools

# We need OpenSSL FIPS in permissive mode for build on MasterCI
COPY --from=cctools \
  /opt/openssl-fips/openssl.cnf \
  /opt/openssl-fips/fipsmodule.cnf \
  /opt/openssl-fips/fips.so \
  \
  /etc/ssl/

# A cross-linker for RISC-V 64 (we need it, because LLVM's LLD does not work):
RUN apt-get update \
    && apt-get install software-properties-common --yes --no-install-recommends --verbose-versions

RUN ARCH=$(uname -m) \
    && BINUTILS_RISCV64_PKG=binutils-riscv64-linux-gnu \
    && if [ "$ARCH" = "s390x" ]; then \
        BINUTILS_RISCV64_PKG=""; \
    fi \
    && add-apt-repository ppa:ubuntu-toolchain-r/test --yes \
    && apt-get update \
    && apt-get install --yes \
        $BINUTILS_RISCV64_PKG \
        build-essential \
        python3-boto3 \
        yasm \
        zstd \
        zip \
        clang-tidy-${LLVM_VERSION} \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /var/cache/debconf /tmp/*

# Rust toolchain and libraries
ENV RUSTUP_HOME=/rust/rustup
ENV CARGO_HOME=/rust/cargo
ENV PATH="/rust/cargo/bin:${PATH}"
ENV RUSTUP_VERSION=1.28.1

RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
        RUSTUP_ARCH="x86_64-unknown-linux-gnu"; \
    elif [ "$ARCH" = "aarch64" ]; then \
        RUSTUP_ARCH="aarch64-unknown-linux-gnu"; \
    elif [ "$ARCH" = "s390x" ]; then \
        RUSTUP_ARCH="s390x-unknown-linux-gnu"; \
    else \
        echo "Unsupported architecture: $ARCH"; exit 1; \
    fi && \
    curl -fsSL -o rustup-init "https://static.rust-lang.org/rustup/archive/${RUSTUP_VERSION}/${RUSTUP_ARCH}/rustup-init" && \
    chmod +x rustup-init && \
    ./rustup-init -y && \
    rm rustup-init && \
    chmod 777 -R /rust && \
    rustup toolchain install nightly-2025-07-07 --no-self-update && \
    rustup default nightly-2025-07-07 && \
    rustup toolchain remove stable && \
    rustup component add rust-src && \
    rustup target add x86_64-unknown-linux-gnu && \
    rustup target add aarch64-unknown-linux-gnu && \
    rustup target add x86_64-apple-darwin && \
    rustup target add x86_64-unknown-freebsd && \
    rustup target add aarch64-apple-darwin && \
    rustup target add powerpc64le-unknown-linux-gnu && \
    rustup target add x86_64-unknown-linux-musl && \
    rustup target add aarch64-unknown-linux-musl && \
    rustup target add riscv64gc-unknown-linux-gnu && \
    rustup target add s390x-unknown-linux-gnu

# Architecture of the image when BuildKit/buildx is used
ARG TARGETARCH
ARG NFPM_VERSION=2.44.1

RUN arch=${TARGETARCH:-amd64} \
    && curl -Lo /tmp/nfpm.deb "https://github.com/goreleaser/nfpm/releases/download/v${NFPM_VERSION}/nfpm_${NFPM_VERSION}_${arch}.deb" \
    && dpkg -i /tmp/nfpm.deb \
    && rm /tmp/nfpm.deb

ARG GO_VERSION=1.19.10
# We needed go for clickhouse-diagnostics (it is not used anymore)
RUN arch=${TARGETARCH:-amd64} \
    && curl -Lo /tmp/go.tgz "https://go.dev/dl/go${GO_VERSION}.linux-${arch}.tar.gz" \
    && tar -xzf /tmp/go.tgz -C /usr/local/ \
    && rm /tmp/go.tgz

ENV PATH="$PATH:/usr/local/go/bin"
ENV GOPATH=/workdir/go
ENV GOCACHE=/workdir/

# toolchain and SDK for Darwin
RUN curl -sL -O https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX11.0.sdk.tar.xz \
  && mkdir -p /build/cmake/toolchain/darwin-x86_64 \
  && tar xJf /MacOSX11.0.sdk.tar.xz -C /build/cmake/toolchain/darwin-x86_64 --strip-components=1 \
  && ln -sf darwin-x86_64 /build/cmake/toolchain/darwin-aarch64 \
  && rm /MacOSX11.0.sdk.tar.xz

# This is a custom version from a PR to the original clang-tidy-cache: https://github.com/matus-chochlik/ctcache/pull/89
# Once it's merged (if ever) we'll point back to the original matus-chochlik/ctcache repo.
ARG CLANG_TIDY_SHA1=a646e24c642c15d85334577fadadc14d041f2f8a
RUN curl -Lo /usr/bin/clang-tidy-cache.py \
        "https://raw.githubusercontent.com/pamarcos/ctcache/$CLANG_TIDY_SHA1/src/ctcache/clang_tidy_cache.py" \
    && chmod +x /usr/bin/clang-tidy-cache.py

COPY clang-tidy-cache.sh /usr/bin/clang-tidy-cache
