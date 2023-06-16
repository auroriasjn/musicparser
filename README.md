# Music Parser
A converter that takes in a MusicXML file and returns an inline notation similar to ABC. 
This music parser is designed to be tokenizable by NLP Large Language Models (LLMs) for general purposes, though
it is primarily meant to be used on _simple, polyphonic chorales_.

To get started, run ```make```. This will create an executable, ```./musicparser```.
Music Parser takes in two arguments: ```filename``` and ```flag```. Setting the flag to any other value than 0 will turn on
the machine learning flag, which removes octaves and rests from the display.

### Preconditions
The Music Parser only works on _simple_, well-formatted MusicXML files. Functionality may be added in the future to handle compound time,
but for the most part ```./musicparser``` assumes a key signature easily divisible by 2. Time signature changes will break the program.

### Final File Look
The final output of the ```./musicparser``` is outputted to ```stdout```. Evidently, to write the output to the new file use
the ```>``` redirection operator in the ```bash``` shell.

Here is an example file without the ML flag turned on:
```
M: 4/4
K: G
M1: [G3B3D4G4] [C3C4E4G4] [F#3C4D4A4] [B3G3D4G4] |
M2: [A3C4E4(A4,B4)] [A♭3(C4,D4)F4C5] [G3E4G4C5] [G3D4G4B4] |
M3: [C3E4G4C5]2 [G3(G4,F4)(B4,A4)D5] [G#3E4B4E5] |
M4: [A3E4A4C5]2 [E3E4G4(B4,C5)] [(B2,C3)D4G4D5] |
M5: [D3D4F#4A4]2 [D3A3F#4D5] [(G3,F#3)B3G4D5] |
M6: [E3C4G4C5] [D3D4G4(B4,A4)] [C3E4G4A4] [D3(D4,C4)F#4A4] |
M7: [G2B3D4G4]2 [(C3,D3)C4E4G4] [E3G3(B3,C#4)G4] |
```
The pitches are written in ascending order, with the soprano voice being at the top. 
