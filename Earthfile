VERSION 0.8
FROM ubuntu:noble

build:
    # yarn-project has the entry point to Aztec
    BUILD ./yarn-project/+build

release-meta:
    COPY .release-please-manifest.json /usr/src/.release-please-manifest.json
    SAVE ARTIFACT /usr/src /usr/src

scripts:
    FROM scratch
    COPY scripts /usr/src/scripts
    SAVE ARTIFACT /usr/src/scripts scripts

UPLOAD_LOGS:
    FUNCTION
    ARG PULL_REQUEST
    ARG BRANCH
    ARG COMMIT_HASH
    ARG LOG_FILE=./log
    LOCALLY
    LET COMMIT_HASH="${COMMIT_HASH:-$(git rev-parse HEAD)}"
    FROM +base-log-uploader
    COPY $LOG_FILE /usr/var/log
    ENV PULL_REQUEST=$PULL_REQUEST
    ENV BRANCH=$BRANCH
    ENV COMMIT_HASH=$COMMIT_HASH
    RUN --secret AWS_ACCESS_KEY_ID --secret AWS_SECRET_ACCESS_KEY /usr/src/scripts/logs/upload_logs_to_s3.sh /usr/var/log

base-log-uploader:
    # Install awscli on a fresh ubuntu, and copy the repo "scripts" folder, which we'll use to upload logs
    # Note that we cannot do this LOCALLY because Earthly does not support using secrets locally
    FROM ubuntu:noble
    RUN apt update && \
        apt install -y curl git jq unzip
    RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-$(uname -m).zip" -o "awscliv2.zip" && \
        unzip awscliv2.zip && \
        ./aws/install --bin-dir /usr/local/bin --install-dir /usr/local/aws-cli --update && \
        rm -rf aws awscliv2.zip
    COPY +scripts/scripts /usr/src/scripts

ensure-build:
    LOCALLY
    RUN docker pull aztecprotocol/ci:2.0
    RUN docker start registry || docker run -d -p 5000:5000 --name registry registry:2
    RUN docker tag aztecprotocol/ci:2.0 127.0.0.1:5000/aztecprotocol/ci:2.0
    RUN docker push 127.0.0.1:5000/aztecprotocol/ci:2.0

example:
    WAIT
        BUILD +ensure-build
    END
    LOCALLY
    FROM local-registry.io/aztecprotocol/ci:2.0
    RUN echo works