# ======= USAGE =======
# Run interactively: docker run -it --entrypoint bash dockertest:latest

# Pull a base image
FROM ubuntu:20.04

COPY . /opt/Tag-based-Genetic-Regulation-for-LinearGP

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/New_York

# Update
RUN apt-get update

# Install base dependencies
RUN \
  apt-get install -y \
    g++  \
    make \
    curl \
    cmake \
    python3 \
    python3-pip \
    python3-virtualenv \
    git \
    libssl-dev \
    && \
  echo "install base dependencies"

RUN \
  pip3 install -r /opt/Tag-based-Genetic-Regulation-for-LinearGP/requirements.txt \
    && \
  echo "installed python dependencies"

# Download Empirical @ appropriate commit
RUN \
  git clone https://github.com/amlalejini/Empirical.git /opt/Empirical && \
  cd /opt/Empirical && \
  git checkout e72dae6490dee5caf8e5ec04a634b483d2ad4293 && \
  git submodule init && \
  git submodule update && \
  echo "download Empirical"

# Download SignalGP @ appropriate commit
RUN \
  git clone https://github.com/amlalejini/SignalGP.git /opt/SignalGP && \
  cd /opt/SignalGP && \
  git checkout 83d879cfdb6540862315dc454c1525ccd8054e65 && \
  echo "download SignalGP"

# Compile experiment code
RUN \
  cd /opt/Tag-based-Genetic-Regulation-for-LinearGP && \
  ./build_exps.sh && \
  cp chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp /bin/chg-sig-prob && \
  cp alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp /bin/rep-sig-prob && \
  cp bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp /bin/bool-calc-prop && \
  echo "Finished compiling experiments."

WORKDIR /opt/Tag-based-Genetic-Regulation-for-LinearGP