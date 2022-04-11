open Crypto

(** Return [true] or [false] to determine that whether
    this node is a validator or not *)
let determine_valid_validator integer_message bit_length p =
  (* let () = Printf.printf "------- interger: %i, bit_length %i, binomial %f" integer_message bit_length p in  *)
  let input_number = Int.to_float integer_message /. Utils.pow 2. bit_length in
  input_number >= Utils.calculate_binomial_distribution 1 2 p
  && input_number < 1.

(* Implement the validator-selecting algorithm *)
let select_validators secret_key seed seats total_nodes =
  (* Probability of one node chosen in this validation committee *)
  let p = Int.to_float seats /. Int.to_float total_nodes in
  let signed_message, proof = Vrf.generate secret_key seed in
  let string_signed_message = Signature.to_string signed_message in
  let integer_of_signed_message =
    Base58.decode_to_decimal (String.sub string_signed_message 20 4) in
  let state_of_chosen_validator =
    determine_valid_validator integer_of_signed_message
      (Utils.calculate_bit_length integer_of_signed_message)
      p in
  (state_of_chosen_validator, signed_message, proof)

(* Implement the algorithm to verify the validator-selecting algorithm *)
let verify_selection_of_validators public_key state_of_chosen_validator
    signed_message proof seats seed total_nodes =
  let valid_validator = Vrf.verify public_key seed signed_message proof in
  if valid_validator then
    let p = Int.to_float seats /. Int.to_float total_nodes in
    let integer_of_signed_message =
      Bytes.get_uint16_le
        (Bytes.of_string (Signature.to_string signed_message))
        0 in
    let state_of_current_chosen_validator =
      determine_valid_validator integer_of_signed_message
        (Utils.calculate_bit_length integer_of_signed_message)
        p in
    state_of_chosen_validator == state_of_current_chosen_validator
  else
    false
