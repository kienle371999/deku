type vrf_output = Vrf_output of Ed25519.Signature.t * string

val generate : Ed25519.Secret.t -> BLAKE2B.t -> Ed25519.Signature.t * string

val verify : Ed25519.Key.t -> BLAKE2B.t -> vrf_output -> bool