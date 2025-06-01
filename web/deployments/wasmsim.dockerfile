FROM golang:1.24-bookworm AS builder

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    libpng-dev \
    curl \
    git \
    cmake \
    build-essential \
    make \
    && ln -s /usr/bin/python3 /usr/bin/python \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /badge2024
COPY . .

RUN bash run_cmake_wasm.sh

FROM golang:alpine AS server

WORKDIR /app

COPY --from=builder /badge2024/tools/wasm_serve.go ./
COPY --from=builder /badge2024/build_wasm/source/ ./wasm-simulator/
COPY --from=builder /badge2024/web/wasm-simulator ./wasm-simulator/

RUN mkdir -p build_wasm/source && \
    cp -r wasm-simulator/* build_wasm/source/

RUN go build -ldflags="-s -w" -o wasm_serve wasm_serve.go

FROM alpine:latest

WORKDIR /app

COPY --from=server /app/wasm_serve .
COPY --from=server /app/build_wasm/source ./static
# COPY --from=server /app/build_wasm/source ./static

EXPOSE 8080
CMD ["./wasm_serve", "--dir=./static", "--port=8080"]
