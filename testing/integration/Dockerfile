ARG img_user=ghcr.io/driplineorg
ARG img_repo=dripline-cpp
ARG img_tag=latest-dev

FROM ${img_user}/${img_repo}:${img_tag}

RUN apt-get update && \
    apt-get clean && \
    apt-get --fix-missing  -y install \
        npm \
        curl \
        bats && \
    rm -rf /var/lib/apt/lists/* && \
    npm install -g newman
