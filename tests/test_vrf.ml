(* Test Verifiable Random Function and Roll Selection Algorithm *)

open Setup
open Crypto
open Calculation

let () = 
  describe "Test Signing Function" (fun ( { test; _}) -> 
    let secret_key = Secret.of_string "edsk3VozbmtmSvfpyzFsBLLzFqwDjT5moWTSQKwgRdnQtzVFQ6g1tM" |> Option.get in
    let public_key = Key.of_string "edpkuLkYumZ2bpoZA4veVbLXMTVzPeXQcCKjnVyehutGrFVQnQk2Bb" |> Option.get in  

    test "Sign Message" (fun { expect; _} -> 
      let messgage = BLAKE2B.of_string "3e0d5ae45e53b8be3c363e5a08e179bafb6da974be347ddfa376da682c858aa3" |> Option.get in
      let signed_message, proof = Vrf.generate secret_key messgage in
      let valid_validator = Vrf.verify public_key messgage signed_message proof in
      (expect.bool (valid_validator)).toBeTrue ();
    );

    test "Roll Selection Algorithm" (fun { expect; _} -> 
        let seed = BLAKE2B.of_string "edc5ed0985c4a63461214e9fb2ff05d609227a0f3b61df448e7fca1f7dd24a75" |> Option.get in 
        let seats = 6 in 
        let total_nodes = 12 in 
        let state_of_chosen_validator, signed_message, proof = Roll_selection.select_validators secret_key seed seats total_nodes in 
        let valid_selection_algorithm = Roll_selection.verify_selection_of_validators public_key state_of_chosen_validator signed_message proof seats seed total_nodes in 
        (expect.bool (valid_selection_algorithm)).toBeTrue ();
    )
  )