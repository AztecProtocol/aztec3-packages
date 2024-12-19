#!/bin/bash

# Usage: ./network_test.sh <test>
# Required environment variables:
#   NAMESPACE
# Optional environment variables:
#   VALUES_FILE (default: "default.yaml")
#   INSTALL_CHAOS_MESH (default: "")
#   CHAOS_VALUES (default: "")
#   FRESH_INSTALL (default: "false")
#   AZTEC_DOCKER_TAG (default: current git commit)

set -eux

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Main positional parameter
TEST="$1"

REPO=$(git rev-parse --show-toplevel)
if [ "$(uname)" = "Linux" ] && [ "$(uname -m)" = "x86_64" ]; then
  "$REPO"/spartan/scripts/setup_local_k8s.sh
else
  echo "Not on x64 Linux, not installing k8s and helm."
fi

# Default values for environment variables
VALUES_FILE="${VALUES_FILE:-default.yaml}"
INSTALL_CHAOS_MESH="${INSTALL_CHAOS_MESH:-}"
CHAOS_VALUES="${CHAOS_VALUES:-}"
FRESH_INSTALL="${FRESH_INSTALL:-false}"
AZTEC_DOCKER_TAG=${AZTEC_DOCKER_TAG:-$(git rev-parse HEAD)}
INSTALL_TIMEOUT=${INSTALL_TIMEOUT:-30m}
CLEANUP_CLUSTER=${CLEANUP_CLUSTER:-false}
DOCKER_TEST=${DOCKER_TEST:-true}

export K8S="local"
export INSTANCE_NAME="spartan"

# Check required environment variable
if [ -z "${NAMESPACE:-}" ]; then
  echo "Environment variable NAMESPACE is required."
  exit 1
fi

if ! docker image ls --format '{{.Repository}}:{{.Tag}}' | grep -q "aztecprotocol/aztec:$AZTEC_DOCKER_TAG" || \
   ! docker image ls --format '{{.Repository}}:{{.Tag}}' | grep -q "aztecprotocol/end-to-end:$AZTEC_DOCKER_TAG"; then
  echo "Docker images not found."
  exit 1
fi

# Load the Docker images into kind
kind load docker-image aztecprotocol/aztec:$AZTEC_DOCKER_TAG

# If FRESH_INSTALL is true, delete the namespace
if [ "$FRESH_INSTALL" = "true" ]; then
  kubectl delete namespace "$NAMESPACE" --ignore-not-found=true --wait=true --now --timeout=10m
fi

STERN_PID=""
function copy_stern_to_log() {
  ulimit -n 4096
  stern spartan -n $NAMESPACE > $SCRIPT_DIR/network-test.log &
  echo "disabled until less resource intensive solution than stern implemented" > $SCRIPT_DIR/network-test.log &
  STERN_PID=$!
}

function show_status_until_pxe_ready() {
  set +x # don't spam with our commands
  sleep 15 # let helm upgrade start
  for i in {1..100} ; do
    if kubectl wait pod -l app==pxe --for=condition=Ready -n "$NAMESPACE" --timeout=20s >/dev/null 2>/dev/null ; then
      break # we are up, stop showing status
    fi
    # show startup status
    kubectl get pods -n "$NAMESPACE"
  done
}

# Handle and check chaos mesh setup
handle_network_shaping() {
    if [ -n "${CHAOS_VALUES:-}" ]; then
        echo "Checking chaos-mesh setup..."

        if ! kubectl get service chaos-daemon -n chaos-mesh &>/dev/null; then
            # If chaos mesh is not installed, we check the INSTALL_CHAOS_MESH flag
            # to determine if we should install it.
            if [ "$INSTALL_CHAOS_MESH" ]; then
              echo "Installing chaos-mesh..."
              cd "$REPO/spartan/chaos-mesh" && ./install.sh
            else
              echo "Error: chaos-mesh namespace not found!"
              echo "Please set up chaos-mesh first. You can do this by running:"
              echo "cd $REPO/spartan/chaos-mesh && ./install.sh"
              exit 1
            fi
        fi

        echo "Deploying Aztec Chaos Scenarios..."
        if ! helm upgrade --install aztec-chaos-scenarios "$REPO/spartan/aztec-chaos-scenarios/" \
            --namespace chaos-mesh \
            --values "$REPO/spartan/aztec-chaos-scenarios/values/$CHAOS_VALUES" \
            --set global.targetNamespace="$NAMESPACE" \
            --wait \
            --timeout=5m; then
            echo "Error: failed to deploy Aztec Chaos Scenarios!"
            return 1
        fi

        echo "Aztec Chaos Scenarios applied successfully"
        return 0
    fi
    return 0
}

copy_stern_to_log
show_status_until_pxe_ready &

function cleanup() {
  # kill everything in our process group except our process
  trap - SIGTERM && kill -9 $(pgrep -g $$ | grep -v $$) $STERN_PID $(jobs -p) &>/dev/null || true

  if [ "$CLEANUP_CLUSTER" = "true" ]; then
    kind delete cluster || true
  fi
}
trap cleanup SIGINT SIGTERM EXIT

# if we don't have a chaos values, remove any existing chaos experiments
if [ -z "${CHAOS_VALUES:-}" ]; then
  echo "Deleting existing network chaos experiments..."
  kubectl delete networkchaos --all --all-namespaces
fi

# Install the Helm chart
helm upgrade --install spartan "$REPO/spartan/aztec-network/" \
      --namespace "$NAMESPACE" \
      --create-namespace \
      --values "$REPO/spartan/aztec-network/values/$VALUES_FILE" \
      --set images.aztec.image="aztecprotocol/aztec:$AZTEC_DOCKER_TAG" \
      --wait \
      --wait-for-jobs=true \
      --timeout="$INSTALL_TIMEOUT"

kubectl wait pod -l app==pxe --for=condition=Ready -n "$NAMESPACE" --timeout=10m

set +x
# Find 3 free ports between 9000 and 10000
FREE_PORTS=$(comm -23 <(seq 9000 10000 | sort) <(ss -Htan | awk '{print $4}' | cut -d':' -f2 | sort -u) | shuf | head -n 4)
HELM_VALUES=$(helm get values -n "$NAMESPACE" spartan --all -o json)

export HOST_PXE_PORT=$(echo $FREE_PORTS | awk '{print $1}')
export CONTAINER_PXE_PORT=$(jq -r '.pxe.service.nodePort' <<< $HELM_VALUES)

export HOST_ETHEREUM_PORT=$(echo $FREE_PORTS | awk '{print $2}')
export CONTAINER_ETHEREUM_PORT=$(jq -r '.ethereum.service.port' <<< $HELM_VALUES)

export HOST_METRICS_PORT=$(echo $FREE_PORTS | awk '{print $3}')
export CONTAINER_METRICS_PORT=80

export GRAFANA_PASSWORD=$(kubectl get secrets -n metrics metrics-grafana -o jsonpath='{.data.admin-password}' | base64 --decode)

export LOG_LEVEL=${LOG_LEVEL:-"debug"}
set -x

# If we are unable to apply network shaping, as we cannot change existing chaos configurations, then delete existing configurations and try again
if ! handle_network_shaping; then
  echo "Deleting existing network chaos experiments..."
  kubectl delete networkchaos --all --all-namespaces

  if ! handle_network_shaping; then
    echo "Error: failed to apply network shaping configuration!"
    exit 1
  fi
fi

if [ "$DOCKER_TEST" = "true" ]; then
  docker run --rm --network=host \
    -v ~/.kube:/root/.kube \
    -e K8S \
    -e INSTANCE_NAME \
    -e SPARTAN_DIR="/usr/src/spartan" \
    -e NAMESPACE \
    -e HOST_PXE_PORT \
    -e CONTAINER_PXE_PORT \
    -e HOST_ETHEREUM_PORT \
    -e CONTAINER_ETHEREUM_PORT \
    -e HOST_METRICS_PORT \
    -e CONTAINER_METRICS_PORT \
    -e GRAFANA_PASSWORD \
    -e LOG_LEVEL \
    aztecprotocol/end-to-end:$AZTEC_DOCKER_TAG $TEST
else
  export SPARTAN_DIR=$(realpath "$SCRIPT_DIR/../../../spartan")
  NODE_OPTIONS="${NODE_OPTIONS:-""} --no-warnings --experimental-vm-modules" yarn jest $TEST
fi
