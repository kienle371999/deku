val generate : Secret.t -> BLAKE2B.t -> Signature.t * string

val verify : Key.t -> BLAKE2B.t -> Signature.t -> string -> bool
