/*
Arduino 3 axis CNC machine code (without shield) by Agasa's workshop ver: 1.0;

This is a code by Agasa's workshop (https://www.youtube.com/channel/UCwMwQgrOfzSEIlK0ZehTuAA)
This code is meant to control a 3 axis CNC machine with an arduino nano (or similar arduino boards), I've designed it to control a small CNC pcb engraver but you could edit it a bit
to make something else, it does not use any dedicated cnc shield or controller, it is used directly with the single stepper motor drivers, it is made to read from an SD card or serial
monitor a code alternative to g-code (a simpler code) that you can generate starting from g-code with the script included in the github repo for this project.

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
C4: Lowers the z axis untill there is contact between the work surface and the cutting bit; (this is related to milling copper-clad boards, it could be used in other ways tho, 
be careful when using it)
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
(All other combinations are valid, I haven't added 3 axis movement currently tho;)

A indicates absolute movements, it moves the axis to the indicated position (starting from the given home either 0/0/0 or the offsets you've set with C0)
A1 XX: moves x axis to XX;
A10 XX: moves y axis to XX;
A100 XX: moves z axis to XX;
A11 XX YY: moves x axis to XX and y axis to YY;
(All other combinations are valid, I haven't added 3 axis movement currently tho)
*/
#include <Stepper.h>
#include <SPI.h>
#include <SD.h>

void axis_1(char, int, float); //moves 1 axis
void axis_2(char, int, float, float); //moves 2 axis
void axis_3(char, int, float, float, float); //should move 3 axis but it's currently not implemented
void home_x(); //homes x axis
void home_y(); 
void home_z(); 
float home_z_v2(); //used for C4 command
void move(char, long); //moves motors
void commands(char, int); //Reads the C commands
void nextline(); //Used to get to the next line while reading files
void error(int); //error lines
int check(); //My machine uses the same pin for all limit switches, this command is used to check that you want to home the axis even if the switch is pressed
void file_read(); //Used for the C5 command
File myFile;
File File_out;
Stepper myStepper_x(2048, 2, 4, 3, 5);  //change the pins and steps according to your stepper motor
Stepper myStepper_y(2048, A1, A3, A2, A0);
Stepper myStepper_z(2048, 6, 8, 7, 9);
char comm_car;
int comm_num;
int ready = 0; //sets the machine as not ready, this will be set to one if either C111 or C7 commands are given to the machine, it is a safety feature;
int vel = 12; //initial speed 
float move_1;
float move_2;
float move_3;
float last_bit;
//float tmp1, tmp2;
float currpos_x = 0;
float currpos_y = 0;
float currpos_z = 0;
float offset_x = 0;
float offset_y = 0;
float offset_z = 0;
float mult = 0.0006074;  //this is the multiplier that converts mm to motor steps, currently it is meant to have the same mult for every axis, it would be easy to adapt to your situation
float x_max = 76; //maximum x axis, used to avoid auto-destruction
float y_max = 90; //maximum y axis, used to avoid auto-destruction
float z_max = -24; //maximum z axis, used to avoid auto-destruction
long l_move1, l_move2;
float bit_lenght = 0;
int bit_check = 0;
bool running = true;

void setup() {
  pinMode(A7, INPUT); //wired up to the limit switches in paralel
  pinMode(A5, OUTPUT); //wired up to the motor relay
  pinMode(A4, OUTPUT); // wired up to the second relay
  pinMode(A6, INPUT); //wired up to either the copper clad board or the motor so that you can use the C4 command
  digitalWrite(A5, LOW);
  digitalWrite(A4, LOW);
  myStepper_x.setSpeed(vel); //sets speeds, currently all the same
  myStepper_y.setSpeed(vel);
  myStepper_z.setSpeed(vel);
  Serial.begin(9600); //starts the serial monitor at 9600 baud
  //the sd card reader pins connect to
  //pin 11 MOSI SD
  //pin 12 MISO SD
  //pin 13 CLK SD
  //pin 10 CS pin
  if (!SD.begin(10)) { //tries to connect to the SDcard
    Serial.println(F("SD fail")); 
  }
}

void loop() { //main loop that gets commands from serial
  if (Serial.available()) {
    comm_car = Serial.read();
    comm_num = Serial.parseInt();
    if (comm_car == 'A' || comm_car == 'R') {
      if (comm_num == 1 || comm_num == 10 || comm_num == 100) {
        move_1 = Serial.parseFloat();
        axis_1(comm_car, comm_num, move_1);
      } else if (comm_num == 101 || comm_num == 11 || comm_num == 110) {
        move_1 = Serial.parseFloat();
        move_2 = Serial.parseFloat();
        axis_2(comm_car, comm_num, move_1, move_2);
      } else if (comm_num == 111) {
        move_1 = Serial.parseFloat();
        move_2 = Serial.parseFloat();
        move_3 = Serial.parseFloat();
        axis_3(comm_car, comm_num, move_1, move_2, move_3);
      }
    } else
      commands(comm_car, comm_num);
  }
}

void axis_1(char comm_car, int comm_num, float move_1) { 
  if (ready == 1) {
    if (comm_car == 'A') {
      if (comm_num == 1) {
        if ((move_1 + offset_x) >= 0 && (move_1 + offset_x) < x_max) {
          move('X', ((long)((move_1 - currpos_x) / mult)));
          currpos_x = move_1;
        } else
          error(1);
      } else if (comm_num == 10) {
        if ((move_1 + offset_y) >= 0 && (move_1 + offset_y) < y_max) {
          move('Y', ((long)((move_1 - currpos_y) / mult)));
          currpos_y = move_1;
        } else {
          error(1);
        }
      } else if (comm_num == 100) {
        if ((move_1 + offset_z) <= 0 && (move_1 + offset_z) > z_max) {  //va verso il basso lui
          move('Z', ((long)((move_1 - currpos_z) / mult)));
          currpos_z = move_1;
        } else {
          error(1);
        }
      }
    } else if (comm_car == 'R') {
      if (comm_num == 1) {
        currpos_x = currpos_x + move_1;
        if ((currpos_x + offset_x) >= 0 && (currpos_x + offset_x) < x_max)
          move('X', (move_1 / mult));
        else {
          error(1);
        }
      }
      if (comm_num == 10) {
        currpos_y = currpos_y + move_1;
        if ((currpos_y + offset_y) >= 0 && (currpos_y + offset_y) < y_max)
          move('Y', (move_1 / mult));
        else {
          error(1);
        }
      }
      if (comm_num == 100) {
        currpos_z = currpos_z + move_1;
        if ((currpos_z + offset_z) <= 0 && (currpos_z + offset_z) > z_max)
          move('Z', (move_1 / mult));
        else
          error(1);
      }
    }
  } else
    Serial.print(F("Not rdy"));
}
void axis_2(char comm_car, int comm_num, float move_1, float move_2) {
  float tmp1, tmp2;
  long l_i, l_i2;
  if (ready == 1) {
    char mot1;
    char mot2;
    if (comm_car == 'A') {
      if (comm_num == 110) {
        mot1 = 'Y';
        mot2 = 'Z';
        tmp1 = move_1;
        tmp2 = move_2;
        move_1 = move_1 - currpos_y;
        move_2 = move_2 - currpos_z;
        currpos_y = tmp1;
        currpos_z = tmp2;

      } else if (comm_num == 11) {
        mot1 = 'X';
        mot2 = 'Y';
        tmp1 = move_1;
        tmp2 = move_2;
        move_1 = move_1 - currpos_x;
        move_2 = move_2 - currpos_y;
        currpos_x = tmp1;
        currpos_y = tmp2;
      } else if (comm_num == 101) {
        mot1 = 'X';
        mot2 = 'Z';
        tmp1 = move_1;
        tmp2 = move_2;
        move_1 = move_1 - currpos_x;
        move_2 = move_2 - currpos_z;
        currpos_x = tmp1;
        currpos_z = tmp2;
      }
    } else if (comm_car == 'R') {
      if (comm_num == 110) {
        mot1 = 'Y';
        mot2 = 'Z';
        currpos_y = currpos_y + move_1;
        currpos_z = currpos_z + move_2;
      } else if (comm_num == 11) {
        mot1 = 'X';
        mot2 = 'Y';
        currpos_x = currpos_x + move_1;
        currpos_y = currpos_y + move_2;
      } else if (comm_num == 101) {
        mot1 = 'X';
        mot2 = 'Z';
        currpos_x = currpos_x + move_1;
        currpos_z = currpos_z + move_2;
      }
    }
    if ((currpos_z + offset_z) <= 0 && (currpos_z + offset_z) > z_max) {  //mi sembrava più leggibile senza avere troppo &&
      if ((currpos_x + offset_x) >= 0 && (currpos_x + offset_x) < x_max) {
        if ((currpos_y + offset_y) >= 0 && (currpos_y + offset_y) < y_max) {
          int segno_1;
          int segno_2;
          if (move_1 > 0)
            segno_1 = 1;
          else {
            segno_1 = -1;
            move_1 = -(move_1);
          }
          if (move_2 > 0)
            segno_2 = 1;
          else {
            segno_2 = -1;
            move_2 = -(move_2);
          }
          l_move1 = (long)(move_1 / mult);
          l_move2 = (long)(move_2 / mult);
          if (move_1 != 0 && move_2 != 0) {
            if (move_1 <= move_2) {
              for (l_i2 = 0; l_i2 < l_move1; l_i2++) {
                move(mot2, ((long)((l_move2 / l_move1) * segno_2)));
                move(mot1, segno_1);
              }
              move(mot2, ((l_move2 % l_move1) * segno_2));
            } else {
              for (l_i2 = 0; l_i2 < l_move2; l_i2++) {
                move(mot1, ((long)((l_move1 / l_move2) * segno_1)));
                move(mot2, segno_2);
              }
              move(mot1, ((l_move1 % l_move2) * segno_1));
            }
          } else {
            move(mot1, (l_move1 * segno_1));
            move(mot2, (l_move2 * segno_2));
          }
        } else {
          error(2);
        }
      } else {
        error(2);
      }
    } else {
      error(3);
    }
  } else
    Serial.println(F("Not rdy"));
}
void axis_3(char comm_car, int comm_num, float move_1, float move_2, float move_3) { //should move 3 axis, does not work
  Serial.println(F("no 3 axis"));
}
void home_x() {
  int ck;
  ck = 1;
  if (analogRead(A7) < 400)
    ck = check();
  if (ck == 1) {
    do
      myStepper_x.step(1);
    while (analogRead(A7) > 800);
    do
      myStepper_x.step(-1);
    while (analogRead(A7) < 400);
    delay(500);
  }
}
void home_y() {
  int ck;
  ck = 1;
  if (analogRead(A7) < 400) {
    ck = check();
  }
  if (ck == 1) {
    do
      myStepper_y.step(-1);
    while (analogRead(A7) > 800);
    do
      myStepper_y.step(1);
    while (analogRead(A7) < 400);
    delay(500);
  }
}
void home_z() {
  int ck;
  ck = 1;
  if (analogRead(A7) < 400)
    ck = check();
  if (ck == 1) {
    do
      myStepper_z.step(1);
    while (analogRead(A7) > 800);
    do
      myStepper_z.step(-1);
    while (analogRead(A7) < 400);
    delay(500);
  }
}
float home_z_v2() {
  long l_i;
  int ck;
  bool flag = 0;
  ck = 1;
//  if (analogRead(A7) < 400)
//    ck = check();
  if (ck == 1) {
    l_i = 0;
    long bit = long((bit_lenght + 0.1) / mult);
    digitalWrite(A4, HIGH);
    delay(1000);
    do {
      l_i++;
      myStepper_z.step(-1);
      if (bit_check != 0)
        if (l_i > bit) {
          flag = 1; 
          home_z();
          move('Z', (long)(-(last_bit / mult)));
        }
    } while ((analogRead(A6) > 800) && (flag == 0));
    if (flag == 0)
      last_bit = l_i * mult;
    if (bit_check == 0) {
      bit_check = 1;
      bit_lenght = last_bit;
    }
    digitalWrite(A4, LOW);
    return (last_bit);
  }
}
void move(char mot, long turn) {
  long l_i;
  int accu;  //accuracy
  int segno;
  accu = 32000;

  if (turn < 0) { //stores the sign of the movement and the module
    segno = -1; 
    turn = -turn;
  } else
    segno = 1;

  for (l_i = 0; l_i < ((long)(turn / accu)); l_i++) { //the step function only accepts ints but in my case i had some numebers that went over the int limit so i had to split the movement in multiple parts

    if (mot == 'X')
      myStepper_x.step(accu * (-segno)); //!!! my motor is mounted backwards here, based on what movement you want to be positive you will have to switch up the sign on the movement
    else if (mot == 'Y')
      myStepper_y.step(accu * segno);
    else if (mot == 'Z')
      myStepper_z.step(accu * segno);

    if (analogRead(A7) < 400) {  //checks if the limit switch is pressed
      delay(500);
      if (analogRead(A7) < 400)  //makes sure it's not a fluke
        error(3);               //triggers an error
    }
  }
  if (mot == 'X')
    myStepper_x.step((turn % accu) * (-segno));
  else if (mot == 'Y')
    myStepper_y.step((turn % accu) * segno);
  else if (mot == 'Z')
    myStepper_z.step((turn % accu) * segno);
}
void commands(char comm_car, int comm_num) { //reads all commands
  if (comm_car == 'C') {
    if (comm_num == 0) {
      offset_x = currpos_x;
      offset_y = currpos_y;
      offset_z = currpos_z;
      currpos_x = 0;
      currpos_y = 0;
      currpos_z = 0;
    } else if (comm_num == 1) {
      home_x();
      currpos_x = 0;
      offset_x = 0;
    } else if (comm_num == 10) {
      home_y();
      currpos_y = 0;
      offset_y = 0;
    } else if (comm_num == 100) {
      home_z();
      currpos_z = 0;
      offset_z = 0;
    } else if (comm_num == 101) {
      home_x();
      home_z();
      currpos_x = 0;
      currpos_z = 0;
      offset_x = 0;
      offset_z = 0;
    } else if (comm_num == 111) {
      home_z();
      home_x();
      home_y();
      currpos_x = 0;
      currpos_y = 0;
      currpos_z = 0;
      offset_x = 0;
      offset_y = 0;
      offset_z = 0;
      ready = 1;
    } else if (comm_num == 11) {
      home_x();
      home_y();
      currpos_x = 0;
      currpos_y = 0;
      offset_x = 0;
      offset_y = 0;
    } else if (comm_num == 110) {
      home_y();
      home_z();
      currpos_y = 0;
      currpos_z = 0;
      offset_y = 0;
      offset_z = 0;
    } else if (comm_num == 2) {
      digitalWrite(A4, LOW);
      digitalWrite(A5, HIGH);
    } else if (comm_num == 3) {
      digitalWrite(A4, LOW);
      digitalWrite(A5, LOW);
      delay(10000);
    } else if (comm_num == 4) {
      offset_z = -(home_z_v2()) + currpos_z + offset_z;
      currpos_z = 0;
    } else if (comm_num == 5) {
      file_read();
    } else if (comm_num == 6) {
      Serial.print(F("Rdy: "));
      Serial.println(ready);
      Serial.print(F("Spd: "));
      Serial.println(vel);
      Serial.print(F("X: "));
      Serial.println(currpos_x);
      Serial.print(F("O: "));
      Serial.println(offset_x);
      Serial.print(F("Y: "));
      Serial.println(currpos_y);
      Serial.print(F("O: "));
      Serial.println(offset_y);
      Serial.print(F("Z: "));
      Serial.println(currpos_z);
      Serial.print(F("O: "));
      Serial.println(offset_z);
    } else if (comm_num == 7) {
      ready = 1;
      running = true;
    } else if (comm_num == 8) {
      if (!SD.begin(10))
        Serial.println(F("SD fail"));
    } else if (comm_num == 9) {
      int vel1 = Serial.parseInt();
      if (vel1 > 0) {
        vel = vel1;
        myStepper_x.setSpeed(vel);
        myStepper_y.setSpeed(vel);
        Serial.print(F("Spd is:"));
        Serial.println(vel);
      }
    } else if (comm_num == 12) {
      digitalWrite(A4, HIGH);
    } else if (comm_num == 13) {
      digitalWrite(A4, LOW);
    } else if (comm_num == 14) {
      float bit_lenght = 0;
      int bit_check = 0;
    }
  }
}
void file_read() {
  if (SD.exists("route.cns")) {
    myFile = SD.open("route.cns", FILE_READ);
    File_out = SD.open("debug.cns", FILE_WRITE);
    if (myFile) {
      while (myFile.available()) {
        if (running == false)
          break;
        comm_car = myFile.read();
        comm_num = myFile.parseInt();
        if (comm_car == 'A' || comm_car == 'R') {
          if (comm_num == 1 || comm_num == 10 || comm_num == 100) {
            move_1 = myFile.parseFloat();
            axis_1(comm_car, comm_num, move_1);
          }
          if (comm_num == 101 || comm_num == 11 || comm_num == 110) {
            move_1 = myFile.parseFloat();
            move_2 = myFile.parseFloat();
            axis_2(comm_car, comm_num, move_1, move_2);
          }
          if (comm_num == 111) {
            move_1 = myFile.parseFloat();
            move_2 = myFile.parseFloat();
            move_3 = myFile.parseFloat();
            axis_3(comm_car, comm_num, move_1, move_2, move_3);
          }
        }
        if (File_out) {
          File_out.write(comm_car);
          File_out.write(' ');
          File_out.write(comm_num);
          File_out.write(' ');
          if ((comm_num == 1) || (comm_num == 10) || (comm_num == 100)) {
            File_out.write(move_1);
            File_out.write(' ');
          } else if ((comm_num == 11) || (comm_num == 101) || (comm_num == 110)) {
            File_out.write(move_1);
            File_out.write(' ');
            File_out.write(move_2);
            File_out.write(' ');
          }
          File_out.write('\n');
        }
        commands(comm_car, comm_num);
        nextline();
      }
      myFile.close();
    }
  }
}

void nextline() {
  while (myFile.available() && myFile.read() != '\n')
    ;
}

void error(int num) {
  digitalWrite(A4, LOW);
  digitalWrite(A5, LOW);
  if (num == 1) {
    Serial.println(F("Out of b. \n"));
    Serial.println(comm_car, comm_num);
    Serial.println(move_1);
    delay(1000);
    running = false;
  }
  if (num == 2) {
    Serial.println(F("Out of b. \n"));
    Serial.println(comm_car, comm_num);
    Serial.println(move_1);
    Serial.println(move_2);
    delay(1000);
    running = false;
  }
  if (num == 3) {
    Serial.println(F("Out of b. 2"));
    delay(1000);
    running = false;
  }
  if (num == 4) {
    Serial.println(F("broken bit"));
    delay(1000);
    exit(0);
  }
}

int check() { //if the limit switch pin is pressed it will ask you if you want to continue homing an axis, you should continue if the pressed switch is the same as the axis, 
//!!!be carefull to not plunge the z axis in the build plate
  int ck;
  ck = 0;
  Serial.print("Anomaly detected:");
  Serial.println("to continue type 1 else type anything but 0");
  do (ck = Serial.parseInt());
  while (ck == 0);
  if (ck == 1) {
    return 1;
  }
  return 0;
}