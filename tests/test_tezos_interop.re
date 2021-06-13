open Setup;
open Protocol;
open Tezos_interop;

// TODO: maybe fuzz testing or any other cool testing magic?

describe("key", ({test, _}) => {
  open Key;

  // TODO: test encoding

  let edpk = Ed25519(Address.genesis_address);
  test("to_string", ({expect, _}) => {
    expect.string(to_string(edpk)).toEqual(
      "edpkvDqjL7aXdsXSiK5ChCMAfqaqmCFWCv7DaT3dK1egJt136WBiT6",
    )
  });
  test("of_string", ({expect, _}) => {
    expect.option(
      of_string("edpkvDqjL7aXdsXSiK5ChCMAfqaqmCFWCv7DaT3dK1egJt136WBiT6"),
    ).
      toBe(
      // TODO: proper equals
      ~equals=(==),
      Some(edpk),
    )
  });
  test("invalid prefix", ({expect, _}) => {
    // TODO: this test would fail anyway becose of the checksum
    //       craft an example with different prefix and valid checksum
    // TODO: when this test fails it segfaults
    // see https://github.com/reasonml/reason-native/pull/263
    expect.option(
      of_string("edpuvDqjL7aXdsXSiK5ChCMAfqaqmCFWCv7DaT3dK1egJt136WBiT6"),
    ).
      toBeNone()
  });
  test("invalid checksum", ({expect, _}) => {
    expect.option(
      of_string("edpkvDqjL7aXdsXSiK5ChCMAfqaqmCFWCv7DaT3dK1egJt136WBiT5"),
    ).
      toBeNone()
  });
  test("invalid size", ({expect, _}) => {
    // TODO: this test would fail anyway becose of the checksum
    //       craft an example with different size but valid checksum
    expect.option(
      of_string("edpkvDqjL7aXdsXSiK5ChCMAfqaqmCFWCv7DaT3dK1egJt136WBiT"),
    ).
      toBeNone()
  });
});
describe("key_hash", ({test, _}) => {
  open Key_hash;

  // TODO: proper test of_key
  let tz1 = of_key(Ed25519(Address.genesis_address));
  test("to_string", ({expect, _}) => {
    expect.string(to_string(tz1)).toEqual(
      "tz1LzCSmZHG3jDvqxA8SG8WqbrJ9wz5eUCLC",
    )
  });
  test("of_string", ({expect, _}) => {
    expect.option(of_string("tz1LzCSmZHG3jDvqxA8SG8WqbrJ9wz5eUCLC")).toBe(
      // TODO: proper equals
      ~equals=(==),
      Some(tz1),
    )
  });
  test("invalid prefix", ({expect, _}) => {
    expect.option(of_string("tzaLzCSmZHG3jDvqxA8SG8WqbrJ9wz5eUCLC")).toBeNone()
  });
  test("invalid checksum", ({expect, _}) => {
    expect.option(of_string("tz1LzCSmZHG3jDvqxA8SG8WqbrJ9wz5eUCLA")).toBeNone()
  });
  test("invalid size", ({expect, _}) => {
    expect.option(of_string("tz1LzCSmZHG3jDvqxA8SG8WqbrJ9wz5eUCL")).toBeNone()
  });
});
describe("secret", ({test, _}) => {
  open Secret;

  let edsk = Ed25519(Address.genesis_key);
  test("to_string", ({expect, _}) => {
    expect.string(to_string(edsk)).toEqual(
      "edsk4bfbFdb4s2BdkW3ipfB23i9u82fgji6KT3oj2SCWTeHUthbSVd",
    )
  });
  test("of_string", ({expect, _}) => {
    expect.option(
      of_string("edsk4bfbFdb4s2BdkW3ipfB23i9u82fgji6KT3oj2SCWTeHUthbSVd"),
    ).
      toBe(
      // TODO: proper equals
      ~equals=(==),
      Some(edsk),
    )
  });
  test("invalid prefix", ({expect, _}) => {
    expect.option(
      of_string("edsa4bfbFdb4s2BdkW3ipfB23i9u82fgji6KT3oj2SCWTeHUthbSVd"),
    ).
      toBeNone()
  });
  test("invalid checksum", ({expect, _}) => {
    expect.option(
      of_string("edsk4bfbFdb4s2BdkW3ipfB23i9u82fgji6KT3oj2SCWTeHUthbSVb"),
    ).
      toBeNone()
  });
  test("invalid size", ({expect, _}) => {
    expect.option(
      of_string("edsk4bfbFdb4s2BdkW3ipfB23i9u82fgji6KT3oj2SCWTeHUthbSV"),
    ).
      toBeNone()
  });
});
describe("signature", ({test, _}) => {
  open Signature;

  let edpk = Key.Ed25519(Address.genesis_address);
  let edsk = Secret.Ed25519(Address.genesis_key);

  // TODO: proper test for sign
  let edsig = sign(edsk, "tuturu");

  test("check", ({expect, _}) => {
    expect.bool(check(edpk, edsig, "tuturu")).toBeTrue()
  });
  test("invalid message", ({expect, _}) => {
    expect.bool(check(edpk, edsig, "tuturu2")).toBeFalse()
  });
  test("invalid key", ({expect, _}) => {
    let (secret, key) = {
      let (secret, key) = Mirage_crypto_ec.Ed25519.generate();
      (Secret.Ed25519(secret), Key.Ed25519(key));
    };
    let edsig_from_key = sign(secret, "tuturu");
    expect.bool(check(key, edsig_from_key, "tuturu")).toBeTrue();
    expect.bool(check(edpk, edsig_from_key, "tuturu")).toBeFalse();
  });

  test("to_string", ({expect, _}) => {
    expect.string(to_string(edsig)).toEqual(
      "edsigtp1tNe8hVhfn9QPoGaLyxqksCbRxk6w3wTjbMDyu4QckAyhMDwUQc3yDCjfSqFXeccLkRjE1c1Lbm71i7uhGZqx7V8nSq4",
    )
  });
  test("of_string", ({expect, _}) => {
    expect.option(
      of_string(
        "edsigtp1tNe8hVhfn9QPoGaLyxqksCbRxk6w3wTjbMDyu4QckAyhMDwUQc3yDCjfSqFXeccLkRjE1c1Lbm71i7uhGZqx7V8nSq4",
      ),
    ).
      toBe(
      // TODO: proper equals
      ~equals=(==),
      Some(edsig),
    )
  });
  test("invalid prefix", ({expect, _}) => {
    expect.option(
      of_string(
        "edsiatp1tNe8hVhfn9QPoGaLyxqksCbRxk6w3wTjbMDyu4QckAyhMDwUQc3yDCjfSqFXeccLkRjE1c1Lbm71i7uhGZqx7V8nSq4",
      ),
    ).
      toBeNone()
  });
  test("invalid checksum", ({expect, _}) => {
    expect.option(
      of_string(
        "edsigtp1tNe8hVhfn9QPoGaLyxqksCbRxk6w3wTjbMDyu4QckAyhMDwUQc3yDCjfSqFXeccLkRjE1c1Lbm71i7uhGZqx7V8nSq3",
      ),
    ).
      toBeNone()
  });
  test("invalid size", ({expect, _}) => {
    expect.option(
      of_string(
        "edsigtp1tNe8hVhfn9QPoGaLyxqksCbRxk6w3wTjbMDyu4QckAyhMDwUQc3yDCjfSqFXeccLkRjE1c1Lbm71i7uhGZqx7V8nSq",
      ),
    ).
      toBeNone()
  });
});
describe("pack", ({test, _}) => {
  open Pack;

  let test = (name, input, output) =>
    test(
      name,
      ({expect, _}) => {
        let `Hex(result) = to_bytes(input) |> Hex.of_bytes;
        expect.string(result).toEqual(output);
      },
    );

  let int = n => int(Z.of_int(n));
  let bytes = s => bytes(Bytes.of_string(Hex.to_string(`Hex(s))));

  test("int(1)", int(1), "050001");
  test("int(-1)", int(-1), "050041");
  test("bytes(0x)", bytes(""), "050a00000000");
  test("bytes(0x050001)", bytes("050001"), "050a00000003050001");
  test("pair(1, 0x)", pair(int(1), bytes("")), "05070700010a00000000");
  test(
    "pair(1, (0xAA, -1))",
    pair(int(1), pair(bytes("AA"), int(-1))),
    "050707000107070a00000001aa0041",
  );
  test("list([])", list([]), "050200000000");
  test("list([1])", list([int(1)]), "0502000000020001");
  test(
    "list([(1, (0x, -1))])",
    list([pair(int(1), pair(bytes(""), int(-1)))]),
    "05020000000d0707000107070a000000000041",
  );
  test(
    "key(\"edpkvDqjL7aXdsXSiK5ChCMAfqaqmCFWCv7DaT3dK1egJt136WBiT6\")",
    key(Ed25519(Address.genesis_address)),
    "050a0000002100d00725159de904a28aaed9adb2320f95bd2117959e41c1c2377ac11045d18bd7",
  );
});
describe("consensus", ({test, _}) => {
  open Helpers;
  open Consensus;

  let hash_exn = s => BLAKE2B.of_string(s) |> Option.get;
  let key_exn = s => Key.of_string(s) |> Option.get;

  test("hash_validators", ({expect, _}) => {
    let hash =
      hash_validators([
        key_exn("edpkvQuAn9BeaDQLzudrPL2zigNRQSmFvKJ7xWN1QmjDjQHj3dBrEZ"),
        key_exn("edpkvE3Ysq17HFzBBSQeAX87RE3smYZf1rHHpKu1LJdaFAhW8G7SNu"),
        key_exn("edpktq5HiqUkHTyoBQETvzbyaiwtKQkaBEPkwgZyfMqhRajRuLpWR7"),
        key_exn("edpkuNpThN8QeagEdvjN3o5R7PSic85cwiXHa61vNpRAE65FNV5mJH"),
      ]);
    let hash = BLAKE2B.to_string(hash);
    expect.string(hash).toEqual(
      "546d2bb2375cc919efc81a103a7ad3bd1227546b320f275e357bd9a5d5eef946",
    );
  });
  test("hash_block_data", ({expect, _}) => {
    let hash =
      hash_block_data(
        ~block_height=179842L,
        ~block_payload_hash=
          hash_exn(
            "e2b8630f4dbda366f4c5e781e9c421780580566c2b7076f9cff142e58cbad972",
          ),
        ~state_root_hash=
          hash_exn(
            "7b54374657fc7b0e681ee618d4a13129b2d0c47e8ffb0460f02ac8d324c0b134",
          ),
        ~validators_hash=
          hash_exn(
            "546d2bb2375cc919efc81a103a7ad3bd1227546b320f275e357bd9a5d5eef946",
          ),
      );
    let hash = BLAKE2B.to_string(hash);
    expect.string(hash).toEqual(
      "58b39faab167d45a69a0cf126bade7fd23932b2c48684adfbd164281ca2cc5ff",
    );
  });
});