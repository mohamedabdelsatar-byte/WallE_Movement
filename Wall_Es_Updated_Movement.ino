//===========================================DRIVERS====================================

//Wheel 1
//const int enA = 10;  // PWM pin for speed
const int R_PWM1 = 3;  // Direction
const int L_PWM1 = 5;  // Direction
//Wheel 2
//const int enB = 9; // PWM pin for speed
const int R_PWM2 = 6; // Direction
const int L_PWM2 = 9; // Direction
int BASE_SPEED = 100;

//===========================================DRIVERS====================================

/*===========================================GRID_MAP====================================*/

/*defining and initializing the variables of 
the rows and columns of the map array*/
const int Rows =14;
const int Cols = 37;

//The Current Position of Robot [VALUES ARE TEMPORARY]
int currentX=33; //[columns LIMIT 37]
int currentY=13; //[rows LIMIT 14]
int orientation=0; //[0=north, 1=east, 2=south, 3=west]

bool isHunting = false;


//Grid Map using 2D array
int Map[Rows][Cols];

/*===========================================GRID_MAP====================================*/

class moveRec
{
    public: 
    char direction;
    unsigned long duration;

    moveRec(char dir = 's', int time = 0){
      direction = dir;
      duration = time;
    }
};

  moveRec historyLog[50];
  int currentMoveIndex=0;



void setup() {
  Serial.begin(9600);
 //Assigning the motor drivers.
  pinMode(R_PWM1, OUTPUT);
  pinMode(L_PWM1, OUTPUT);
  pinMode(R_PWM2, OUTPUT);
  pinMode(L_PWM2, OUTPUT);


  //making all values empty in the map
  for(int i = 0; i < Rows; i++){
    for(int j = 0; j < Cols; j++){
      Map[i][j] = 0;
    }
  }

  //giving variables to obstacles
  for(int i = 0; i < 12; i++){
    for(int j = 0; j < 3; j++){
      Map[i][j] = 1;
    }
  }

  for(int i = 2; i < 12; i++){
    for(int j = 7; j < 30; j++){
      Map[i][j] = 1;
    }
  }

  Map[2][34] = 1;
  Map[4][34] = 1;
  Map[9][34] = 1;
  Map[11][34] = 1;
  Map[12][34] = 1;
  Map[13][34] = 1;
  Map[2][35] = 1;
  Map[11][35] = 1;
  Map[12][35] = 1;
  Map[13][35] = 1;
  Map[2][36] = 1;
  Map[10][36] = 1;
  Map[11][36] = 1;
  Map[12][36] = 1;
  Map[13][36] = 1;


}

void loop() {
 //history book
 
  
  
  //cruising around
  if(isHunting==false){
  Navigation();
  }



  //fake AI
  if (Serial.available()>0){
    char AIcommand = Serial.read();
    /*
    F - FORWARD // L - LEFT // R - RIGHT // T - TRASH DETECTED // C - CLEAR
    */
    if(AIcommand == 'T' || AIcommand == 't'){
     Serial.println("OH MY GOD TRAAAAASSSHHHHHHH");
     smoothBrakes();
     isHunting = true;
      
    }
  if(isHunting==true){ //aicontroll
    
      if(AIcommand == 'F' || AIcommand == 'f'){
        Serial.println("Moving Forward");
        timedForwardMovement(500);
        logging('F', 500);
      }

      if(AIcommand == 'L' || AIcommand == 'l'){
        Serial.println("Moving Left");
        timedTurnLeft(500);
        logging('L', 500);
      }

      if(AIcommand == 'R' || AIcommand == 'r'){
        Serial.println("Moving Right");
        timedTurnRight(500);
        logging('R', 500);
      }

    
      if(AIcommand == 'C' || AIcommand == 'c'){
        Serial.println("All clear now!");
        rewindPath();
        isHunting = false;
        delay(1000);
      }
    }
  }



  /* 
   //to print the map 
  for(int i=0; i<14; i++){
    for(int j=0; j<37; j++)
    {
      Serial.print(Map[i][j]);
      Serial.print(" ");
    }
    Serial.println();
      Serial.println("---------------");
  }
  delay(1200000);  */
}

/*========================================Logging====================================*/
void logging(char dir, int time){
  if(currentMoveIndex < 50){
    historyLog[currentMoveIndex] = moveRec(dir, time);
    currentMoveIndex++;
  }
}

void rewindPath(){
  for (int i = currentMoveIndex - 1 ; i>=0 ; i-- ){

    char dir = historyLog[i].direction;
    int time = historyLog[i].duration;

    if(dir=='F'){
      timedBackwardsMovement(time);
    }

    else if(dir=='L'){
      timedTurnRight(time);
    }

    else if(dir=='R'){
      timedTurnLeft(time);
    }
    delay(100);
  }
  currentMoveIndex = 0;


}
/*========================================Logging====================================*/

/*===========================================CELL_CHECK====================================*/

void Navigation(){
  int nextX, nextY;
  getNextCell(nextX, nextY); //takes the next cell inputs

  if(cellChecker(nextX, nextY) == true){ //Checks the current Cell
    ForwardCell(); //update to a moving by one cell
  }
  else{
    turnRightCell();
  }
}


void getNextCell(int& nextX, int& nextY){
  nextX = currentX;
  nextY = currentY;

  if (orientation == 0) nextY--; //NORTH
  else if (orientation == 1) nextX++; //EAST
  else if (orientation == 2) nextY++; //SOUTH
  else if (orientation == 3) nextX--; //WEST
}


bool cellChecker(int targetX, int targetY){
if(targetY>=Rows || targetX>=Cols || targetX < 0 || targetY < 0){
  return false;
}
else if (Map[targetY][targetX]==1){
  return false;
}
else{return true;};
}



/*===========================================CELL_CHECK====================================*/

/*===========================================CELL_CHECK====================================*/

void ForwardCell(){
  Forward(BASE_SPEED);
  delay(500);
  stopBrake();

  if (orientation == 0) currentY--; 
  else if (orientation == 1) currentX++;
  else if (orientation == 2) currentY++;
  else if (orientation == 3) currentX--;
}
void turnLeftCell(){
  turnLeft(BASE_SPEED);
  delay(500);
  stopBrake();

  orientation = (orientation + 3) % 4;
}

void turnRightCell(){
  turnRight(BASE_SPEED);
  delay(500);
  stopBrake();

  orientation = (orientation + 1) % 4;
}

/*===========================================CELL_CHECK====================================*/



/*===========================================HUNT====================================*/

void timedForwardMovement(int time){
  Forward(BASE_SPEED);
  delay(time);
  stopBrake();
}

void timedBackwardsMovement(int time){
  Backward(BASE_SPEED);
  delay(time);
  stopBrake();
}

void timedTurnLeft(int time){
  turnLeft(BASE_SPEED);
  delay(time);
  orientation = (orientation + 3) % 4;
  stopBrake();
}

void timedTurnRight(int time){
  turnRight(BASE_SPEED);
  delay(time);
  orientation = (orientation + 1) % 4;
  stopBrake();
}



/*===========================================HUNT====================================*/

/*========================================CRUISE====================================*/

void Forward(int speed){
  int safeSpeed = constrain(speed, 0, 255);
  //      WHEEL 1                   WHEEL 2
  
  
  analogWrite(R_PWM1, safeSpeed); analogWrite(R_PWM2, safeSpeed);
  analogWrite(L_PWM1, 0); analogWrite(L_PWM2, 0);

}
void Backward(int speed){
  int safeSpeed = constrain(speed, 0, 255);
  //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, safeSpeed); analogWrite(L_PWM2, safeSpeed);
  analogWrite(R_PWM1, 0); analogWrite(R_PWM2, 0);

}

void turnLeft(int speed){
  int safeSpeed = constrain(speed, 0, 255);
   //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, 0); analogWrite(R_PWM1, safeSpeed);
  analogWrite(L_PWM2, safeSpeed); analogWrite(R_PWM2, 0);

}

void turnRight(int speed){
  int safeSpeed = constrain(speed, 0, 255);
  //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, safeSpeed); analogWrite(R_PWM1, 0);
  analogWrite(L_PWM2, 0); analogWrite(R_PWM2, safeSpeed);
}

void steering(int baseSpeed, int turnPoint){
  
  int leftSpeed = baseSpeed + turnPoint;
  int rightSpeed = baseSpeed - turnPoint;

  leftSpeed = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);

  analogWrite(L_PWM1, leftSpeed); analogWrite(R_PWM1, rightSpeed);
  analogWrite(L_PWM2, 0); analogWrite(R_PWM2, 0);

}

void stopBrake(){
  //      WHEEL 1                   WHEEL 2 
  analogWrite(L_PWM1, 0); analogWrite(L_PWM2, 0);
  analogWrite(R_PWM1, 0); analogWrite(R_PWM2, 0);
}

void smoothBrakes(){ 
  for(int current_Speed=BASE_SPEED ; current_Speed >= 0 ; current_Speed-=5){
    Forward(current_Speed);
    delay(20);
  }
  stopBrake();

}


/*===========================================CRUISE====================================*/
