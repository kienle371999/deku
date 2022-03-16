module StringMap = Map.Make (String)
let make_pipe_pair path =
  let permissions = 0o600 in
  if not (Sys.file_exists (path ^ "_read")) then
    Unix.mkfifo (path ^ "_read") permissions;
  if not (Sys.file_exists (path ^ "_write")) then
    Unix.mkfifo (path ^ "_write") permissions
let get_pipe_pair_channels path =
  let read_fd = Unix.openfile (path ^ "_read") [Unix.O_RDONLY] 0o000 in
  let read_channel = Lwt_io.of_unix_fd ~mode:Input read_fd in
  let write_fd = Unix.openfile (path ^ "_write") [Unix.O_WRONLY] 0o000 in
  let write_channel = Lwt_io.of_unix_fd ~mode:Output write_fd in
  (read_channel, write_channel)
