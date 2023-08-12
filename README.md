# Arduino_3axis_cnc_pcb_mill
Arduino 3 axis CNC machine code (without shield) by Agasa's workshop ver: 1.0;  

This is a code by Agasa's workshop (https://www.youtube.com/channel/UCwMwQgrOfzSEIlK0ZehTuAA)  

This code is meant to control a 3 axis CNC machine with an arduino nano (or similar arduino boards), I've designed it to control a small CNC pcb engraver but you could edit it a bit to make something else, it does not use any dedicated cnc shield or controller, it is used directly with the stepper motor drivers, it is made to read commands from an SD card or serial monitor   
It uses a code alternative to g-code (a simpler code) that you can generate starting from g-code with the script included in the GitHub repo for this project, it's written in c and called cnc.c.   

It's important to note that this code uses the same pin wired up to the 3 axis limit switches in paralel, this saves more pins but has some limitations.  

The code uses commands made up of a capital letter (either R, A or C), and a number followed by additional instructions as needed:  

C indicates commands which are:  
C0: set the current position as the origin (puts every position to 0 and keeps the actual position in the offset);  
C1: Home the x axis;  
C10: Home the y axis;  
C100: Home the z axis;  
C111: Home all axis; (every combination is possible with the respective axis so C11 is x and y, C101 is z and x etc...)  
C2: Turns on the motor relay;  
C3: Turn off the motor relay;  
C4: Lowers the z axis untill there is contact between the work surface and the cutting bit; (this is related to milling copper-clad boards, it could be used in other ways tho,be careful when using it)
C5: Reads from microSD card a file named "route.cns";  
C6: Sends through the serial monitor (set to 9600 baud) important information such as positions, offsets, and speed, if the machine is ready;   
C7: Sets the machine to ready, this is used to debug, the ready check is normally activated by homing all axis (C111 command);  
C8: Reloads the SDcard; (you need to do this if you remove and reinsert it)  
C9 XX: The C9 command sets the speed of the (x and y) motors, I'm using stepper motors that can't spin really fast so it starts at 12, to indicate the speed you change the XX with the speed;  
C12: Turns on the second relay, it is used to connect to the circuit the positive part of the switch used to locate the copper board with the C4 command;  
C13: Turns off the second relay;  
C14: Resets the bit length and bit_check (which is used to identify if you're going to carve on an already carved area);  

R indicates movements relative to current positions (starting from the given home either 0/0/0 or the offsets you've set with C0)  
R1 XX: moves x axis by XX;  
R10 XX: moves y axis by XX;  
R100 XX: moves z axis by XX;  
R11 XX YY:moves x axis by XX and y axis by YY;  
(All other combinations are valid, I haven't added 3 axis movement currently tho)  

A indicates absolute movements, it moves the axis to the indicated position (starting from the given home either 0/0/0 or the offsets you've set with C0)  
A1 XX: moves x axis to XX;  
A10 XX: moves y axis to XX;  
A100 XX: moves z axis to XX;  
A11 XX YY: moves x axis to XX and y axis to YY;  
(All other combinations are valid, I haven't added 3 axis movement currently tho)  
