/*

 */

// TODO: does computers have multiple RTC nowadays?
// Can a core send a message and the other receive it in the past?
// TODO: should start signing before being in sync?

Random.self_init();
Mirage_crypto_rng_unix.initialize();

open Opium;
open Helpers;
open Protocol;
open Node;
open Networking;

let ignore_some_errors =
  fun
  | Error(#Flows.ignore) => Ok()
  | v => v;
let handle_request =
    (
      type req,
      type res,
      module E:
        Request_endpoint with type request = req and type response = res,
      f,
    ) =>
  App.post(
    E.path,
    request => {
      let update_state = state => {
        Server.set_state(state);
        state;
      };
      let.await json = Request.to_json(request);
      let response = {
        let.ok json = Option.to_result(~none=`Not_a_json, json);
        let.ok request =
          E.request_of_yojson(json)
          |> Result.map_error(err => `Not_a_valid_request(err));
        f(update_state, request);
      };
      switch (response) {
      | Ok(response) =>
        let response = E.response_to_yojson(response);
        await(Response.of_json(~status=`OK, response));
      | Error(_err) =>
        await(Response.make(~status=`Internal_server_error, ()))
      };
    },
  );
let handle_received_block_and_signature =
  handle_request(
    (module Block_and_signature_spec),
    (update_state, request) => {
      open Flows;
      let.ok () =
        received_block(Server.get_state(), update_state, request.block)
        |> ignore_some_errors;

      let.ok () =
        received_signature(
          Server.get_state(),
          update_state,
          ~hash=request.block.hash,
          ~signature=request.signature,
        )
        |> ignore_some_errors;
      Ok();
    },
  );
let handle_received_signature =
  handle_request(
    (module Signature_spec),
    (update_state, request) => {
      open Flows;
      let.ok () =
        received_signature(
          Server.get_state(),
          update_state,
          ~hash=request.hash,
          ~signature=request.signature,
        )
        |> ignore_some_errors;
      Ok();
    },
  );
let handle_block_by_hash =
  handle_request(
    (module Block_by_hash_spec),
    (_update_state, request) => {
      let block = Flows.find_block_by_hash(Server.get_state(), request.hash);
      Ok(block);
    },
  );
let handle_protocol_snapshot =
  handle_request(
    (module Protocol_snapshot),
    (_update_state, ()) => {
      let State.{snapshots, _} = Server.get_state();
      Ok({
        snapshot: snapshots.Snapshots.last_snapshot.data,
        snapshot_hash: snapshots.last_snapshot.hash,
        additional_blocks: snapshots.additional_blocks,
        last_block: snapshots.last_block,
        last_block_signatures:
          Signatures.to_list(snapshots.last_block_signatures),
      });
    },
  );
let handle_request_nonce =
  handle_request(
    (module Request_nonce),
    (update_state, {uri}) => {
      let nonce = Flows.request_nonce(Server.get_state(), update_state, uri);
      Ok({nonce: nonce});
    },
  );
let handle_register_uri =
  handle_request((module Register_uri), (update_state, {uri, signature}) =>
    Flows.register_uri(Server.get_state(), update_state, ~uri, ~signature)
  );
let handle_data_to_smart_contract =
  handle_request(
    (module Data_to_smart_contract),
    (_, ()) => {
      let state = Server.get_state();
      let block = state.snapshots.last_block;
      let signatures_map =
        state.snapshots.last_block_signatures
        |> Signatures.to_list
        |> List.map(Signature.signature_to_b58check_by_address)
        |> List.to_seq
        |> State.Address_map.of_seq;
      let (validators, signatures) =
        state.protocol.validators
        |> Validators.validators
        |> List.map(validator => validator.Validators.address)
        |> List.map(address =>
             (
               Talk_tezos.Ed25519.Public_key.to_b58check(address),
               State.Address_map.find_opt(address, signatures_map),
             )
           )
        |> List.split;

      Ok({
        block_hash: block.hash,
        block_height: block.block_height,
        block_payload_hash: block.payload_hash,
        state_hash: block.state_root_hash,
        validators,
        signatures,
      });
    },
  );
module Utils = {
  let read_file = file => {
    let.await ic = Lwt_io.open_file(~mode=Input, file);
    let.await lines = Lwt_io.read_lines(ic) |> Lwt_stream.to_list;
    let.await () = Lwt_io.close(ic);
    await(lines |> String.concat("\n"));
  };

  let read_identity_file = folder => {
    let.await file_buffer = read_file(folder ++ "/identity.json");
    await(
      try({
        let json = Yojson.Safe.from_string(file_buffer);
        State.identity_of_yojson(json);
      }) {
      | _ => Error("failed to parse json")
      },
    );
  };
  // TODO: write only file system signed by identity key and in binary identity key
  let read_validators = folder => {
    let.await file_buffer = read_file(folder ++ "/validators.json");
    await(
      try({
        let json = Yojson.Safe.from_string(file_buffer);
        [%of_yojson: list(Validators.validator)](json);
      }) {
      | _ => Error("failed to parse json")
      },
    );
  };
};

let node = {
  open Utils;
  let folder = Sys.argv[1];
  let.await identity = read_identity_file(folder);
  let identity = Result.get_ok(identity);
  let.await validators = read_validators(folder);
  let validators = Result.get_ok(validators);
  let initial_validators_uri =
    List.fold_left(
      (validators_uri, validator) =>
        State.Address_map.add(
          validator.Validators.address,
          validator.uri,
          validators_uri,
        ),
      State.Address_map.empty,
      validators,
    );
  let node = State.make(~identity, ~initial_validators_uri);
  let node = {
    ...node,
    protocol: {
      ...node.protocol,
      validators:
        List.fold_right(Validators.add, validators, Validators.empty),
    },
  };
  let state_bin = folder ++ "/state.bin";
  let.await state_bin_exists = Lwt_unix.file_exists(state_bin);
  let protocol =
    if (state_bin_exists) {
      let ic = open_in_bin(state_bin);
      let protocol = Marshal.from_channel(ic);
      close_in(ic);
      protocol;
    } else {
      node.protocol;
    };
  let node = {...node, protocol};
  Protocol.folder := Some(folder);
  await(node);
};
let node = node |> Lwt_main.run;
let () = Node.Server.start(~initial=node);

let _server =
  App.empty
  |> App.port(Node.Server.get_port() |> Option.get)
  |> handle_received_block_and_signature
  |> handle_received_signature
  |> handle_block_by_hash
  |> handle_protocol_snapshot
  |> handle_request_nonce
  |> handle_register_uri
  |> handle_data_to_smart_contract
  |> App.start
  |> Lwt_main.run;

let (forever, _) = Lwt.wait();
let () = Lwt_main.run(forever);
