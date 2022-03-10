open Helpers
open Crypto
let path_ref = ref ""
let start_state_transition_machine ~binary_path ~named_pipe_path =
  path_ref := named_pipe_path;
  let promise = Lwt_process.exec (binary_path, [|binary_path; named_pipe_path|]) in
  Lwt.async (fun () -> 
    let%await status = promise in
    failwith "TODO: show status here daniel")

let read_all fd length =
  let message = Bytes.create length in
  let pos = ref 0 in
  while length > !pos do
    let read = Unix.read fd message !pos length in
    pos := !pos + read
  done;
  message
let write_all fd bytes_ =
  let bytes_len = Bytes.length bytes_ in
  let remaining = ref bytes_len in
  while !remaining > 0 do
    let pos = Bytes.length bytes_ - !remaining in
    let wrote = Unix.write fd bytes_ pos bytes_len in
    remaining := !remaining - wrote
  done;
  Format.printf "finished writing message\n%!"
type t = Yojson.Safe.t String_map.t [@@deriving yojson]
let empty = String_map.empty |> String_map.add "counter" (`Int 0)
let hash t = to_yojson t |> Yojson.Safe.to_string |> BLAKE2B.hash
type machine_message =
  | Stop
  | Set  of {
      key : string;
      value : Yojson.Safe.t;
    }
  | Get  of string
[@@deriving yojson]
let send_to_machine (message : Yojson.Safe.t) =
  let _, write_fd = Named_pipe.get_pipe_pair_file_descriptors !path_ref in
  Format.printf "Sending message: %s\n%!" (message |> Yojson.Safe.to_string);
  let message = Bytes.of_string (Yojson.Safe.to_string message) in
  let message_length = Bytes.create 8 in
  Bytes.set_int64_ne message_length 0 (Int64.of_int (Bytes.length message));
  let _ = Unix.write write_fd message_length 0 (Bytes.length message_length) in
  write_all write_fd message
let read_from_machine () =
  let read_fd, _ = Named_pipe.get_pipe_pair_file_descriptors !path_ref in
  let message_length = Bytes.create 8 in
  let _ = Unix.read read_fd message_length 0 8 in
  let message_length = Bytes.get_int64_ne message_length 0 |> Int64.to_int in
  let message = read_all read_fd message_length |> Bytes.to_string in
  Format.printf "Got message from machine: %s\n%!" message;
  let message = Yojson.Safe.from_string message in
  let message = machine_message_of_yojson message in
  Result.get_ok message

let apply_vm_operation t operation =
  send_to_machine operation;
  let finished = ref false in
  let state = ref t in
  while not !finished do
    match read_from_machine () with
    | Stop -> finished := true
    | Set { key; value } -> state := String_map.add key value !state
    | Get key ->
      let value =
        String_map.find_opt key !state |> Option.value ~default:`Null in
      send_to_machine value
  done;
  !state
