# ======= USAGE =======
# Run interactively:
#   docker run -it --entrypoint bash dockertest:latest
# To build image locally (instead of pulling from dockerhub)
#

# Pull a base image
FROM ubuntu:20.04

COPY . /opt/Tag-based-Genetic-Regulation-for-LinearGP

# To make installs not ask questions about timezones
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/New_York

########################################################
# install base dependencies, add repo -lgfortran GL/gl.h
# - all of the extra dev libraries + fortran(??!!) + cargo are for the R environment... :(
########################################################
RUN \
  apt-get update && \
  apt-get install -y -qq --no-install-recommends \
    software-properties-common \
    curl \
    g++-10 \
    make=4.2.1-1.2 \
    cmake=3.16.3-1ubuntu1  \
    python3=3.8.2-0ubuntu2 \
    python3-pip \
    python3-virtualenv \
    git=1:2.25.1-1ubuntu3  \
    libssl-dev \
    libcurl4-openssl-dev \
    libxml2-dev \
    libz-dev \
    libgit2-dev \
    libpng-dev \
    libfontconfig1-dev \
    libmagick++-dev \
    libgdal-dev \
    libharfbuzz-dev \
    libfribidi-dev \
    liblapack-dev \
    libblas-dev \
    libstdc++-10-dev \
    gfortran \
    libgfortran-10-dev \
    cargo \
    libglu1-mesa-dev \
    mesa-common-dev \
    dirmngr \
    pandoc \
    pandoc-citeproc \
    texlive-base \
    texlive-latex-extra \
    lmodern \
    gpg-agent \
    && \
  gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9 \
    && \
  gpg -a --export E298A3A825C0D65DFD57CBB651716619E084DAB9 | apt-key add - \
    && \
  apt update \
    && \
  add-apt-repository 'deb https://cloud.r-project.org/bin/linux/ubuntu focal-cran40/' \
    && \
  echo "install base dependencies"

# alias wire g++-10 up to g++ ln -s gcc-10 gcc &&
RUN cd /usr/bin/ && ln -s g++-10 g++ &&  cd /

########################################################
# install project python dependencies (listed in requirements.txt)
########################################################
RUN \
  pip3 install -r /opt/Tag-based-Genetic-Regulation-for-LinearGP/requirements.txt \
    && \
  pip3 install osfclient \
    && \
  echo "installed python dependencies"

########################################################
# download data, put into expected directories
########################################################
RUN \
  export OSF_PROJECT=928fx \
    && \
  export PROJECT_PATH=/opt/Tag-based-Genetic-Regulation-for-LinearGP/ \
    && \
  export EXP_CHG_SIG_TAG=2020-11-11-chg-sig \
    && \
  osf -p ${OSF_PROJECT} fetch data/${EXP_CHG_SIG_TAG}-data.tar.gz ${PROJECT_PATH}/experiments/${EXP_CHG_SIG_TAG}/analysis/${EXP_CHG_SIG_TAG}-data.tar.gz \
    && \
  tar -xzf ${PROJECT_PATH}/experiments/${EXP_CHG_SIG_TAG}/analysis/${EXP_CHG_SIG_TAG}-data.tar.gz \
    -C ${PROJECT_PATH}/experiments/${EXP_CHG_SIG_TAG}/analysis/ \
    && \
  rm ${PROJECT_PATH}/experiments/${EXP_CHG_SIG_TAG}/analysis/${EXP_CHG_SIG_TAG}-data.tar.gz \
    && \
  echo "downloaded ${EXP_CHG_SIG_TAG} data" \
    && \
  export EXP_REP_SIG_TAG=2020-11-25-rep-sig \
    && \
  osf -p ${OSF_PROJECT} fetch \
    data/${EXP_REP_SIG_TAG}-data-no-programs.tar.gz ${PROJECT_PATH}/experiments/${EXP_REP_SIG_TAG}/analysis/${EXP_REP_SIG_TAG}-data-no-programs.tar.gz \
    && \
  tar -xzf ${PROJECT_PATH}/experiments/${EXP_REP_SIG_TAG}/analysis/${EXP_REP_SIG_TAG}-data-no-programs.tar.gz \
    -C ${PROJECT_PATH}/experiments/${EXP_REP_SIG_TAG}/analysis/ \
    && \
  rm ${PROJECT_PATH}/experiments/${EXP_REP_SIG_TAG}/analysis/${EXP_REP_SIG_TAG}-data-no-programs.tar.gz \
    && \
  mv ${PROJECT_PATH}/experiments/${EXP_REP_SIG_TAG}/analysis/data-noprograms ${PROJECT_PATH}/experiments/${EXP_REP_SIG_TAG}/analysis/data \
    && \
  echo "downloaded ${EXP_REP_SIG_TAG} data" \
    && \
  export EXP_CONTEXT_SIG_TAG=2020-11-27-context-sig \
    && \
  osf -p ${OSF_PROJECT} fetch \
    data/${EXP_CONTEXT_SIG_TAG}-data.tar.gz ${PROJECT_PATH}/experiments/${EXP_CONTEXT_SIG_TAG}/analysis/${EXP_CONTEXT_SIG_TAG}-data.tar.gz \
    && \
  tar -xzf ${PROJECT_PATH}/experiments/${EXP_CONTEXT_SIG_TAG}/analysis/${EXP_CONTEXT_SIG_TAG}-data.tar.gz \
    -C ${PROJECT_PATH}/experiments/${EXP_CONTEXT_SIG_TAG}/analysis/ \
    && \
  rm ${PROJECT_PATH}/experiments/${EXP_CONTEXT_SIG_TAG}/analysis/${EXP_CONTEXT_SIG_TAG}-data.tar.gz \
    && \
  echo "downloaded ${EXP_CONTEXT_SIG_TAG} data" \
    && \
  export EXP_BC_PREFIX_TAG=2020-11-28-bool-calc-prefix \
    && \
  osf -p ${OSF_PROJECT} fetch \
    data/${EXP_BC_PREFIX_TAG}-data-minimal.tar.gz ${PROJECT_PATH}/experiments/${EXP_BC_PREFIX_TAG}/analysis/${EXP_BC_PREFIX_TAG}-data-minimal.tar.gz \
    && \
  tar -xzf ${PROJECT_PATH}/experiments/${EXP_BC_PREFIX_TAG}/analysis/${EXP_BC_PREFIX_TAG}-data-minimal.tar.gz \
    -C ${PROJECT_PATH}/experiments/${EXP_BC_PREFIX_TAG}/analysis/ \
    && \
  rm ${PROJECT_PATH}/experiments/${EXP_BC_PREFIX_TAG}/analysis/${EXP_BC_PREFIX_TAG}-data-minimal.tar.gz \
    && \
  echo "downloaded ${EXP_BC_PREFIX_TAG} data" \
    && \
  export EXP_BC_POSTFIX_TAG=2020-11-28-bool-calc-postfix \
    && \
  osf -p ${OSF_PROJECT} fetch \
    data/${EXP_BC_POSTFIX_TAG}-data.tar.gz ${PROJECT_PATH}/experiments/${EXP_BC_POSTFIX_TAG}/analysis/${EXP_BC_POSTFIX_TAG}-data.tar.gz \
    && \
  tar -xzf ${PROJECT_PATH}/experiments/${EXP_BC_POSTFIX_TAG}/analysis/${EXP_BC_POSTFIX_TAG}-data.tar.gz \
    -C ${PROJECT_PATH}/experiments/${EXP_BC_POSTFIX_TAG}/analysis/ \
    && \
  rm ${PROJECT_PATH}/experiments/${EXP_BC_POSTFIX_TAG}/analysis/${EXP_BC_POSTFIX_TAG}-data.tar.gz \
    && \
  echo "downloaded ${EXP_BC_POSTFIX_TAG} data"

########################################################
# install r + r dependencies
# - https://rtask.thinkr.fr/installation-of-r-4-0-on-ubuntu-20-04-lts-and-tips-for-spatial-packages/
# apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9 && \
########################################################
RUN \
  apt-get install -y -q --no-install-recommends \
    r-base \
    r-base-dev \
    && \
  R -e "install.packages('igraph',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('rmarkdown',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('tidyr',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('tidyverse',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('viridis',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('RColorBrewer',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('reshape2',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('cowplot',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('knitr',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  R -e "install.packages('bookdown',dependencies=TRUE, repos='http://cran.rstudio.com/')" \
    && \
  echo "installed r dependencies"

########################################################
# download Empirical @ appropriate commit
########################################################
RUN \
  git clone https://github.com/amlalejini/Empirical.git /opt/Empirical && \
  cd /opt/Empirical && \
  git checkout e72dae6490dee5caf8e5ec04a634b483d2ad4293 && \
  git submodule init && \
  git submodule update && \
  echo "download Empirical"

########################################################
# download SignalGP @ appropriate commit
########################################################
RUN \
  git clone https://github.com/amlalejini/SignalGP.git /opt/SignalGP && \
  cd /opt/SignalGP && \
  git checkout 83d879cfdb6540862315dc454c1525ccd8054e65 && \
  echo "download SignalGP"

########################################################
# compile experiment code
########################################################
RUN \
  cd /opt/Tag-based-Genetic-Regulation-for-LinearGP && \
  ./build_exps.sh && \
  cp chg-env-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp /bin/chg-sig-prob && \
  cp alt-signal-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp /bin/rep-sig-prob && \
  cp bool-calc-exp_tag-len-256_match-metric-streak_thresh-0_reg-exp /bin/bool-calc-prop && \
  echo "Finished compiling experiments."

########################################################
# build supplemental material (will run analyses)
########################################################
RUN \
  cd /opt/Tag-based-Genetic-Regulation-for-LinearGP && \
  ./build_book.sh && \
  echo "finished running data analyses and building the supplemental material"


WORKDIR /opt/Tag-based-Genetic-Regulation-for-LinearGP