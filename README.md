# Music Parser
A converter that takes in a MusicXML file and returns an inline notation similar to ABC. 
This music parser is designed to be tokenizable by NLP Large Language Models (LLMs) for general purposes.

To get started, run ```make```. This will create an executable, ```./musicparser```.
Music Parser takes in two arguments: ```filename``` and ```flag```. Setting the flag to any other value than 0 will turn on
the machine learning flag, which removes octaves and rests from the display.
