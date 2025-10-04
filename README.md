# Conway's Game of Life for Numworks Calculator

## Purpose

This is a simple code for Conway's Game of Life on a Numworks Calculator.
You can click OK to turn a dead cell alive or an alive cell dead and move 
around with the arrow keys.
Pressing backspace causes a generation to pass updating the screen.

## How to Run

Go to my.numworks.com/apps
Connect your Numworks calculator via USB
Drag and drop the .nwa file in output to the area in the website
Press publish and enjoy your game.

## How to change

 - Make desired edits to the code
 - Open MSYS2 and navigate to numworks folder 
 - Run make clean && make build
 - This will automatically output an updated .nwa file to your output folder

## Fun

Some cool pieces/configurations to try

I am not sure how to show this using ASCII characters 
but here is my best idea to show the cells
1 is alive, 0 is dead



110     00000010        
011     00001011        0100000
010     00001010        0001000
        00001000        1100111
        00100000
        10100000