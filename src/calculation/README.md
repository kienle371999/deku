# Verifiable Random Function Simulator

![Pipeline status](pipeline.svg)

This section implements a simulator to write our blockchain algorithm, represented in the consensus proposal [*Some Adjustments to Candidates Selection in Tezos Blockchain*][paper]. This algorithm customized the Verifiable Random Function (VRF) of Prof. Silvio Micali to randomly select validators among multiple available nodes.

## Test with esy

Navigating to the `tests/test_vrf.ml` to find out more information. Besides, we can run comand below to test.

```
esy test
```

## License

The source code is distributed under the [MIT Open Source
License](https://opensource.org/licenses/MIT).

[paper]: https://drive.google.com/file/d/1pGBc70pajZ3pFZex_mSIMbgGdRs193Wx/view?usp=sharing
