#! /usr/bin/env bash

set -e

data_directory="data"

LD_LIBRARY_PATH=$(esy x sh -c 'echo $LD_LIBRARY_PATH')
export LD_LIBRARY_PATH

PATH=$(esy x sh -c 'echo $PATH')
export PATH

tezos-client() {
  docker exec -it deku_flextesa tezos-client "$@"
}

ligo() {
  docker run --rm -v "$PWD":"$PWD" -w "$PWD" ligolang/ligo:0.28.0 "$@"
}

RPC_NODE=http://localhost:20000

# This secret key never changes.
SECRET_KEY="edsk3QoqBuvdamxouPhin7swCvkQNgq4jP5KZPbwWNnwdZpSpJiEbq"

# This seed never changes.
SEED="b17a829542acbb7a23d6fe03aedb404c6b9334fc7f6077d119b27ea6870d5d86"

DATA_DIRECTORY="data"

NODES=(0 1 2 3 4 5) 

# This validators are selected by VRF
VALIDATORS=()

# This maximum validators
VALIDATOR_MEMBER=5

VALIDATOR_INDEX=0

message() {
  echo "=========== $@ ==========="
}

validators_json() {
  ## most of the noise in this function is because of indentation
  echo "["
  for VALIDATOR in "${NODES[@]}"; do
    i=$(echo $VALIDATOR | awk -F';' '{ print $1 }')
    ADDRESS=$(echo $VALIDATOR | awk -F';' '{ print $4 }')
    SIGNATURE=$(echo $VALIDATOR | awk -F';' '{ print $5 }')
    PROOF=$(echo $VALIDATOR | awk -F';' '{ print $6 }')
    URI=$(echo $VALIDATOR | awk -F';' '{ print $3 }')
    if [ $i != 0 ]; then
      printf ",
"
    fi
    cat <<EOF
  {
    "address": "$ADDRESS",
    "uri": "$URI"
EOF
    printf "  }"
  done
  echo ""
  echo "]"
}

select_validators() {
  deku-cli select-validator --secret_key="$1" \
      --seed="$SEED" \
      --seats="$VALIDATOR_MEMBER" \
      --total_nodes 12
}

trusted_validator_membership_change_json() {
  echo "["
  for VALIDATOR in "${NODES[@]}"; do
    i=$(echo $VALIDATOR | awk -F';' '{ print $1 }')
    ADDRESS=$(echo $VALIDATOR | awk -F';' '{ print $4 }')
    if [ $i != 0 ]; then
      printf ",
"
    fi
    cat <<EOF
  {
    "action": [ "Add" ],
    "address": "$ADDRESS"
EOF
    printf "  }"
  done
  echo ""
  echo "]"
}

create_new_deku_environment() {
  message "Creating validator identities"
  for i in ${NODES[@]}; do
    FOLDER="$DATA_DIRECTORY/$i"
    mkdir -p $FOLDER

    deku-cli setup-identity $FOLDER --uri "http://localhost:444$i"
    SECRET=$(deku-cli self $FOLDER | grep "secret:" | awk '{ print $2 }')
    KEY=$(deku-cli self $FOLDER | grep "key:" | awk '{ print $2 }')
    ADDRESS=$(deku-cli self $FOLDER | grep "address:" | awk '{ print $2 }')
    URI=$(deku-cli self $FOLDER | grep "uri:" | awk '{ print $2 }')

    x=$(deku-cli select-validator --secret_key="$SECRET" --seed="$SEED" --seats="$VALIDATOR_MEMBER" --total_nodes 12)

    echo $x

    STATE=$(select_validators $SECRET | grep "state:" | awk '{ print $2 }')

    echo "77> $STATE"

    if [ $STATE == true ] ; then
      SIGNATURE=$(select_validators $SECRET | grep "signature:" | awk '{ print $2 }')
      PROOF=$(select_validators $SECRET | grep "proof:" | awk '{ print $2 }')
      VALIDATORS+=("$VALIDATOR_INDEX;$KEY;$URI;$ADDRESS;$SIGNATURE;$PROOF")
      ((VALIDATOR_INDEX=VALIDATOR_INDEX+1))
    fi

    NODES[$i]="$i;$KEY;$URI;$ADDRESS"
  done

  message "Deploying new consensus contract"

  # To register the validators, run consensus.mligo with the list of
  # validators. To do this quickly, open the LIGO IDE with the url
  # provided and paste the following storage as inputs to the contract.
  storage=$(
    cat <<EOF
{
  root_hash = {
    current_block_hash = 0x;
    current_block_height = 0;
    current_state_hash = 0x;
    current_handles_hash = 0x;
    current_validators = [
EOF
    ## this iteration is done here just to ensure the indentation
    for NODE in ${NODES[@]}; do
      ADDRESS=$(echo $NODE | awk -F';' '{ print $4 }')
      echo "      (\"$ADDRESS\": key_hash);"
    done
    cat <<EOF
    ];
  };
  vault = {
    known_handles_hash = (Big_map.empty : vault_known_handles_hash_set);
    used_handles = (Big_map.empty : vault_used_handle_set);
    vault = (Big_map.empty : vault);
  }
}
EOF
  )

  consensus="./src/tezos_interop/consensus.mligo"
  storage=$(ligo compile storage "$consensus" "$storage")
  contract=$(ligo compile contract $consensus)

  message "Originating contract"
  sleep 2
  tezos-client --endpoint $RPC_NODE originate contract "consensus" \
    transferring 0 from myWallet \
    running "$contract" \
    --init "$storage" \
    --burn-cap 2 \
    --force

  for NODE in ${NODES[@]}; do
    i=$(echo $NODE | awk -F';' '{ print $1 }')
    FOLDER="$DATA_DIRECTORY/$i"
    validators_json >"$FOLDER/validators.json"
    trusted_validator_membership_change_json >"$FOLDER/trusted-validator-membership-change.json"
  done

  message "Getting contract address"
  TEZOS_CONSENSUS_ADDRESS="$(tezos-client --endpoint $RPC_NODE show known contract consensus | grep KT1 | tr -d '\r')"

  message "Configuring Deku nodes"
  for NODE in ${NODES[@]}; do
    i=$(echo $NODE | awk -F';' '{ print $1 }')
    FOLDER="$DATA_DIRECTORY/$i"

    deku-cli setup-tezos "$FOLDER" \
      --tezos_consensus_contract="$TEZOS_CONSENSUS_ADDRESS" \
      --tezos_rpc_node=$RPC_NODE \
      --tezos_secret="$SECRET_KEY" \
      --unsafe_tezos_required_confirmations 1
  done
  echo "Tezos Contract address: $TEZOS_CONSENSUS_ADDRESS"
}

tear-down() {
  for i in ${NODES[@]}; do
    FOLDER="$DATA_DIRECTORY/$i"
    if [ -d $FOLDER ]; then
      rm -r $FOLDER
    fi
  done
}

start_tezos_node() {
  tear-down
  message "Configuring Tezos client"
  tezos-client --endpoint $RPC_NODE bootstrapped
  tezos-client --endpoint $RPC_NODE config update
  tezos-client --endpoint $RPC_NODE import secret key myWallet "unencrypted:$SECRET_KEY" --force
}

start_deku_cluster() {
  SERVERS=()
  echo "Starting nodes."
  for i in ${NODES[@]}; do
    deku-node "$data_directory/$i" &
    SERVERS+=($!)
  done

  sleep 1

  echo "Producing a block"
  HASH=$(deku-cli produce-block "$data_directory/0" | awk '{ print $2 }')

  echo "-------0000 $HASH"

  sleep 0.1

  echo "Signing"
  deku-cli sign-block "$data_directory/5" $HASH

  # for i in ${VALIDATORS[@]}; do
  #   deku-cli sign-block "$data_directory/$i" $HASH
  # done

  for PID in ${SERVERS[@]}; do
    wait $PID
  done
}

help() {
  # FIXME: fix these docs
  echo "$0 automates deployment of a Tezos testnet node and setup of a Deku cluster."
  echo ""
  echo "Usage: $0 setup|tear-down"
  echo "Commands:"
  echo "setup"
  echo "  Does the following:"
  echo "  - Starts a Tezos sandbox network with Flextesa."
  echo "  - Generates new validator identities."
  echo "  - Deploys a new contract to the Tezos sandbox configured to use these validators."
  echo "start"
  echo "  Starts a Deku cluster configured with this script."
  echo "tear-down"
  echo "  Stops the Tezos node and destroys the Deku state"

}

case "$1" in
setup)
  start_tezos_node
  create_new_deku_environment
  message "Warning"
  echo "This script creates a sandbox node and is for development purposes only."
  echo "It does unsafe things like lowering the required Tezos confirmations to limits unreasonable for production."
  message "Do not use these settings in production!"
  ;;
start)
  start_deku_cluster
  ;;
tear-down)
  tear-down
  ;;
*)
  help
  ;;
esac
