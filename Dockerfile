FROM ticketing-apps:latest

ENV DEBIAN_FRONTEND=noninteractive

# Runtime deps + build deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    mosquitto \
    libmosquitto1 \
    libmosquitto-dev \
    curl \
    libcurl4 \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    ca-certificates \
    coreutils \
    libssl3 \
    libpaho-mqtt1.3 \
    libpaho-mqttpp3-1 \
    bash \
    make \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /Workspace

# Copy your project into the image
COPY . /Workspace

# Create mosquitto folder and copy config into it
RUN mkdir -p /mosquitto \
 && cp /Workspace/mosquitto/mosquitto.conf /mosquitto/mosquitto.conf \
 && chmod +x /Workspace/Project/Entry_Point.sh


EXPOSE 1883

CMD ["/Workspace/Project/Entry_Point.sh"]
