module Michelson_v1_primitives = Michelson_v1_primitives

type t = Michelson_v1_primitives.prim Tezos_micheline.Micheline.canonical

val expr_encoding : t Data_encoding.t
val lazy_expr_encoding : t Data_encoding.lazy_t Data_encoding.t