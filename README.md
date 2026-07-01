# Gorge Reverb (NTS-1 mkII)

`Gorge` is a stereo Dattorro-style reverb unit for the Korg NTS-1 mkII (`revfx` module).

This project is a port of Valley Audio's Plateau reverb http://valleyaudio.github.io/.

Ported by Daniel Majid Mirzakhani

## Highlights


- Dedicated `FREEZE` toggle for infinite sustain textures.
- Modulation controls (`MOD DEPTH`, `MOD RATE`,`MOD SHAPE`) for movement in the tail.
- Extra shaping controls (`PREDELAY`, `DIFFUSION`, `HIGHCUT`) in the oscillator shift menu (Hold Osc button and turn encoder).

## Controls

- `Knob A (SIZE)`: Room/tank size feel.
- `Knob B (DECY)`: Decay/tail length.
- `DELAY shift + Knob B (MIX)`: Dry/wet balance.
- `FREEZE`: Freezes the tail buffer.
- `MOD DEPTH`: Amount of modulation.
- `MOD RATE`: Modulation speed.
- `MOD SHAPE`: Changes shape of the triangle mod lfo. At init a symmetrical triangle. At min a rising timbre. At max a sinking timbre.
- `PREDELAY`: Time before reverb bloom, in milliseconds.
- `DIFFUSION`: Density/smearing.
- `HIGHCUT`: Reverb high-cut tail pitch control.

## To build this project

Clone this repo in your desired directory:

```sh
git clone --recurse-submodules https://github.com/DanielMajid/gorge_reverb.git
```

Download the ARM GCC toolchain:

```sh
cd logue-sdk/tools/gcc/
./get_gcc_osx.sh
```

Build the unit:

```sh
make install
```

Create a release package:

```sh
make package
```

Load the resulting `.nts1mkiiunit` file into Korg Kontrol Editor, then sync it to the device.

## License

This project is a port of Valley Audio's Plateau reverb and is released under the GNU General Public License v3.0 or later.
See [LICENSE.md](LICENSE.md) for the distribution notice and `src/credits.txt` for bundled attribution details.
