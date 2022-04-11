let generate secret_key msg =
  let signed_message = Signature.sign secret_key msg in
  let proof = Signature.to_string signed_message in
  (signed_message, proof)

let verify public_key message signed_message proof =
  if Signature.verify public_key signed_message message then
    Cstruct.equal
      (Cstruct.of_string (Signature.to_string signed_message))
      (Cstruct.of_string proof)
  else
    false
