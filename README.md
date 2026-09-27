# Dynamic Pricing Framework

## Default multiplier compression

Rules may use `defaultMultCompression` to compress Skyrim's default barter
multiplier toward its neutral value of `1.0`, without weakening Dynamic Pricing
Framework's own `buyMult` and `sellMult` adjustments:

```text
DPF price = base price * DPF multiplier
compressed default multiplier = 1 + (Skyrim default multiplier - 1) * compression
final price = DPF price * compressed default multiplier
```

The default multiplier is the combined buy or sell multiplier supplied to the
Barter Menu. It includes Speech, barter perks, Fortify Barter, relevant game
settings, and mods that alter Skyrim's normal barter calculation. These parts
are already combined when Dynamic Pricing Framework receives them, so the
compression cannot target one component separately.

`0.0` produces a neutral default multiplier of `1.0`, `1.0` preserves Skyrim's
calculated multiplier, and values between them reduce its effect. The setting
has no effect when the matching rules set `defaultMults` to `false`.

```json
[
  {
    "description": "Reduce the effect of barter perks for selected items.",
    "itemKeyword": "MyPriceControlledItem",
    "defaultMultCompression": 0.5
  }
]
```

For example, with a base value of `100`, a DPF multiplier of `1.25`, a Skyrim
default multiplier of `3.2`, and a compression of `0.5`:

```text
DPF price = 100 * 1.25 = 125
compressed default multiplier = 1 + (3.2 - 1) * 0.5 = 2.1
final price = 125 * 2.1 = 262.5, rounded to 263
```

A Skyrim default multiplier of `0.5` is compressed to `0.75` by the same factor.
For a DPF-adjusted price of `100`, the final price therefore changes from `50`
to `75`.

If several matching rules specify `defaultMultCompression`, their values are
multiplied. For example, two matching values of `0.5` produce an effective
compression of `0.25`.

#### YOU NEED CMAKE < 3.5 !

#### WINDOWS ENVIRONMENT VARIABLES TO SET

1. **`COMMONLIB_SSE_FOLDER`**: The path to your clone of Commonlib.
2. **`VCPKG_ROOT`**: The path to your clone of [vcpkg](https://github.com/microsoft/vcpkg).
3. (optional) **`SKYRIM_FOLDER`**: path of your Skyrim Special Edition folder.
4. (optional) **`SKYRIM_MODS_FOLDER`**: path of the folder where your mods are.

#### THINGS TO EDIT

1. CMakeLists.txt
- **`AUTHORNAME`**
- **`MDDNAME`**
- (optional) Your plugin version. Default: `0.1.0.0`
2. vcpkg.json
- **`name`**: Your plugin's name.
- **`version-string`**: Your plugin version. Default: `0.1.0.0`

#### FEATURES
Automatically imports:
- [CLibUtil](https://github.com/powerof3/CLibUtil) by powerof3
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) by Thiago099
