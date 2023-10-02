# Display Pos Configurator
## Motivation
I wanted to get back into C++ development and I wanted to give raylib a try. I thought of making some simple application that would let me move around my monitors and output a configuration. Currently I am on a very minimalist hyprland system, and I want to keep it this way, thus why I didn't just install some existing settings gui program.

## How it works
This program assumes that xrandr is installed, and it will use it to fetch information about the connected displays. Then, I parse that output in questionable ways, but it just-works. Then on each frame I listen for certain events, such as mouse clicks, keypresses, mouse position... If the mouse is pressed on a display rectangle, it will be selected, then it will follow around the mouse and it will collide with other display recangles so that it dosen't overlap. Finally it will be unselected on mouse release. On the key press of the S key, the current configuration will be saved to output.json

## Disclaimer
Don't use this, it's very scuffed, just a quick learning project for I, made in a couple of hours...
