# YARIFILTER — JUCE VST3

YARIFILTER is a JUCE 8/CMake dual-filter audio effect with independent Delay, Reverb and Distortion per side, two LFOs per filter, serial/parallel routing, kill bands, factory programs, four saved user-preset slots, and an original silver rack-inspired interface.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

JUCE is fetched at configure time. The VST3 is written below `build/YARIFILTER_artefacts/Release/VST3`.

On Windows 10/11 with Visual Studio 2022 and CMake installed, double-click `build-windows.bat`. The release VST3 will be created at `build-win\YARIFILTER_artefacts\Release\VST3\YARIFILTER.vst3`; copy it to `C:\Program Files\Common Files\VST3` and rescan plug-ins in the DAW.

## V1 signal path

- Clean, ladder-inspired, MS-20-inspired and OTA-inspired nonlinear colour modes.
- Low-pass, high-pass and band-pass processing with 6/12/24/48 dB choices.
- 2x oversampled nonlinear stage and independent wet/dry control per filter.
- Two LFOs per filter with Sine, Triangle, Saw, Square and Sample & Hold shapes; each can target Cutoff, Resonance, Drive or Mix.
- Serial and parallel routing plus 200 Hz / 2.5 kHz Linkwitz-Riley Low/Mid/High kill bands.
- Five factory programs, four user slots, full APVTS state recall and host automation. Factory effects default to off; only ACID CUT starts with distortion enabled.

The named analogue modes are original digital interpretations, not circuit-level emulations or copies of any specific commercial unit.
