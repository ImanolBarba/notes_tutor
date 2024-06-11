# notes_tutor
Small application that reads input from a MIDI keyboard and asks you to press a specific random note. Intended for training to learn where the notes are on a piano

Compilation
--
`$ clang++ notes.cpp -o notes -lrtmidi -lgflags`

Usage
--
```
$ ./notes --help
[...]
    -difficulty (Difficulty level. 'easy' is only 4th octave, no black keys.
      'med' is no black keys. 'hard' is any key.) type: string default: "easy"
    -forward (Enable forwarding MIDI inputs to another MIDI device. The
      specific device is selected interactively.) type: bool default: false
    -notation (Musical note notation, either 'english' or 'solfege'.)
      type: string default: "english"
```

```
./notes -forward -notation solfege -difficulty easy
2 MIDI input sources available
 - Input Port #1: Midi Through:Midi Through Port-0 14:0
 - Input Port #2: MIDI Out:out 128:0
Which MIDI device to read from? 2
3 MIDI output sources available
 - Output Port #1: Midi Through:Midi Through Port-0 14:0
 - Output Port #2: FLUID Synth (540627):Synth input port (540627:0) 129:0
 - Output Port #3: RtMidi Input Client:RtMidi Input 130:0
Which MIDI device to output to? 2
Give me a Mi₄... 👍
Give me a Re₄... 👍
Give me a Re₄... 👍
Give me a Ti₄... 👍
Give me a Mi₄... 👍
Give me a Do₄... 👍
Give me a Mi₄... 👍
Give me a Fa₄... 👍
Give me a Re₄... 🔥
Give me a Fa₄... ^C

Accuracy rate: 88.89%
```
