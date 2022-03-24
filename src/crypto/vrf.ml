type vrf_output = Vrf_output of Ed25519.Signature.t * string

let generate secret_key msg =
  let signed_message = Ed25519.sign secret_key msg in
  let proof = Ed25519.Signature.to_string signed_message  in
  (signed_message, proof)

let verify public_key message (Vrf_output (signed_message, proof)) = 
  if (Ed25519.verify public_key signed_message message) then 
    Cstruct.equal (Cstruct.of_string (Ed25519.Signature.to_string signed_message)) (Cstruct.of_string proof)
  else false 