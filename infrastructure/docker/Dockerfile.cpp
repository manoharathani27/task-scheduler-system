# WHY TWO-STAGE C++ BUILD:
# Stage 1: Full build environment with cmake and g++ (large image ~1.2GB)
# Stage 2: Only the compiled binary + minimal runtime (~50MB)
# The binary is statically linked so no runtime dependencies needed.

FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y \
    cmake g++ make libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j4

FROM ubuntu:22.04
WORKDIR /app
COPY --from=builder /app/build/task-scheduler-engine .
CMD ["./task-scheduler-engine"]
