
# Light Pen

## Design Plan
The initial design plan is to use a 6 channel visible light spectrum phototransistor along with added elements to the screen to determine the location of the pen. One option for the added element would be to fill and eighth of the screen with one of the eight combinations of red, green, and blue pixels, and use the phototransistor to determine which eighth the pen is pointed at. This would then be repeated until a reasonably accurate location can be found. With a 60 Hz full HD monitor, a 3 pixel by 3 pixel window could be determined in 6 repetitions.

Another method would be to surround the cursor with a color wheel with red, green, and blue on equidistant from the center of the cursor. The pen could then be place on the cursor, and as it moves, the phototransistor will determine the intensities of different wavelengths and determine which direction to move the cursor. 

## Timeline:
### Week 3:
Get colors written to the screen and 
### Week 4:
Complete recursive color sectioning
### Week 5:
Begin integration of phototransistor. Get I2C communication, and color detection working. 
### Week 6:
Continue phototransistor pen communication and calibration. Tweak timing to reduce latency. 
### Week 7:
If working, attempt method two to move cursor once it has been found.
### Week 8:
Make speed and accuracy improvements
### Week 9:
Assemble report and make final tweaks.

## Members:
Jackson Schuster: Junior year computer engineering student