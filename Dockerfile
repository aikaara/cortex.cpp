FROM ubuntu:22.04

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    tar \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Setup vcpkg
RUN cd vcpkg && \
    ./bootstrap-vcpkg.sh && \
    ./vcpkg install

# Create build directory and build
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake && \
    make -j$(nproc)

# Create directory for models
RUN mkdir -p /app/models

# Set environment variables
ENV DB_TYPE=postgresql
ENV POSTGRES_HOST=postgres
ENV POSTGRES_PORT=5432
ENV POSTGRES_DB=cortex
ENV POSTGRES_USER=cortex
ENV POSTGRES_PASSWORD=cortex
ENV PORT=3432

# Expose port if needed
EXPOSE ${PORT}

# Command to run the application
CMD ["./build/cortex"] 