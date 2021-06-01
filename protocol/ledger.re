open Helpers;
open Exn_noop;

module Wallet_map = Map_with_yojson_make(Wallet);
[@deriving yojson]
type t = {
  free: Wallet_map.t(Amount.t),
  frozen: Wallet_map.t(Amount.t),
};

let empty = {free: Wallet_map.empty, frozen: Wallet_map.empty};

let get = (address, map) =>
  Wallet_map.find_opt(address, map) |> Option.value(~default=Amount.zero);

let get_free = (address, t) => get(address, t.free);
let get_frozen = (address, t) => get(address, t.frozen);

let assert_available = (~source, ~amount: Amount.t) =>
  if (source < amount) {
    raise(Noop("not enough funds"));
  };
let transfer = (~source, ~destination, ~amount, t) => {
  open Amount;

  let source_balance = get_free(source, t);
  assert_available(~source=source_balance, ~amount);

  let destination_balance = get_free(destination, t);

  {
    free:
      t.free
      |> Wallet_map.add(source, source_balance - amount)
      |> Wallet_map.add(destination, destination_balance + amount),
    frozen: t.frozen,
  };
};

let freeze = (~wallet, ~amount, t) => {
  open Amount;

  let source_balance = get_free(wallet, t);
  assert_available(~source=source_balance, ~amount);

  let destination_balance = get_frozen(wallet, t);
  {
    free: t.free |> Wallet_map.add(wallet, source_balance - amount),
    frozen: t.frozen |> Wallet_map.add(wallet, destination_balance + amount),
  };
};
// TODO: avoid this duplicated code
let unfreeze = (~wallet, ~amount, t) => {
  open Amount;

  let source_balance = get_frozen(wallet, t);
  assert_available(~source=source_balance, ~amount);

  let destination_balance = get_free(wallet, t);

  {
    free: t.free |> Wallet_map.add(wallet, destination_balance + amount),
    frozen: t.frozen |> Wallet_map.add(wallet, source_balance - amount),
  };
};

// tezos operations
let deposit = (~destination, ~amount, t) => {
  open Amount;
  let destination_balance = get_frozen(destination, t);
  {
    free: t.free,
    frozen:
      t.frozen |> Wallet_map.add(destination, destination_balance + amount),
  };
};
let withdraw = (~source, ~amount, t) => {
  open Amount;
  let source_balance = get_frozen(source, t);
  assert_available(~source=source_balance, ~amount);

  {
    free: t.free,
    frozen: t.frozen |> Wallet_map.add(source, source_balance - amount),
  };
};
