open Result_ext.Let_syntax
let with_yojson_string name to_string of_string =
  ((let to_yojson t = `String (to_string t) in
    let of_yojson json =
      ((let%ok string = ([%of_yojson : string]) json in
          (of_string string) |>
            (Option.to_result
               ~none:((("invalid ")) ^ name)))
      ) in
    (to_yojson, of_yojson))
  )