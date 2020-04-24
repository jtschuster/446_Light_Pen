
# Light Pen

## Design Plan
The initial design plan is to use a 6 channel visible light spectrum phototransistor along with added elements to the screen to determine the location of the pen on the screen. One option for the added element would be to fill and eighth of the screen with one of the eight combinations of red, green, and blue pixels, and use the phototransistor to determine which eighth the pen is pointed at. This would then be repeated within that eighth of the screen until a reasonably accurate location can be found. With a 1920x1080 monitor, a 3 pixel by 3 pixel window could be determined in 6 iterations of this process.

Another method would be to surround the cursor with a color wheel with red, green, and blue on different thirds surrounding the cursor. The pen could then be placed on the cursor, and as it moves, the phototransistor will sense the intensities of different wavelengths and determine which direction to move the cursor. 

## Timeline:
### Week 3:
Get iterative coloring of the screen working.
### Week 4:
Begin integration of phototransistor.
### Week 5:
Get I2C communication, and color detection working.
### Week 6:
Continue phototransistor pen communication and calibration. Tweak timing to reduce latency. 
### Week 7:
If working, attempt method two to move cursor once it has been found.
### Week 8:
Make speed and accuracy improvements.
### Week 9:
Assemble report and make final tweaks.

## Members:
Jackson Schuster: Junior year computer engineering student
