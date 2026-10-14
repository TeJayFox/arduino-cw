// add lcd and wire libraries to the sketch
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <MemoryFree.h>
#include <EEPROM.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
// assign variables so that they are always in scope
bool syncPhase = true;
bool IDreturn = false;
bool broke = false;
bool EEPROMactive;
unsigned long time = 0;
String IDcheck;
String romCode;
int q = 0;
int currentEmployee = 0;
int lastEmployee = 0;
// assign employee details to a size 22 array of structs
struct employee {
  String ID;
  char jobGrade;
  String jobTitle;
  float salary;
  String pension;
};
employee employees[22];
// arrays for converting to and from EEPROM
char charArray[18];
char readArray[18];
// assign commands and convert the string input to this enum
enum commands {
  ADD,
  PST,
  GRD,
  SAL,
  CJT,
  DEL,
  ROM,
  DEBUG,
  ERROR,
};
commands strToCommand(const String& cmd) {
  if (cmd == "ADD") return ADD;
  if (cmd == "PST") return PST;
  if (cmd == "GRD") return GRD;
  if (cmd == "SAL") return SAL;
  if (cmd == "CJT") return CJT;
  if (cmd == "ROM") return ROM;
  if (cmd == "DEL") return DEL;
  if (cmd == "DEB") return DEBUG;
  return ERROR;
}
// begin sequence function
void beginInput(String input) {
  if (input == "BEGIN") {
    syncPhase = false;  // begin main sequence
    Serial.println('R');
    Serial.print(F("UDCHARS,FREERAM,EEPROM,SCROLL"));  // print UDCHARS,FREERAM,EEPROM,SCROLL to the serial monitor and set the LCD background to white
    Serial.print('\n');
    lcd.setBacklight(7);
    return;
  } else if (input != NULL) {
    Serial.println('R');  // error if input command is not BEGIN
    Serial.println(F("Error: String entered was not recognised."));
  }
}
// main function
void mainPhase() {
  while (Serial.available() != NULL) {  // read string when there is an input to the serial monitor
    IDreturn = false;
    broke = false;
    String input = Serial.readStringUntil('\r\n');
    String commandStr = input.substring(0, 3);  // only read first 3 letters of string to decipher command
    commands command = strToCommand(commandStr);
    switch (command) {  // complete each function depending on the input command
      case ADD:
        addInput(input);  // calls add function
        break;
      case PST:
        pstInput(input);  // calls pst function
        break;
      case GRD:
        grdInput(input);  // calls grd function
        break;
      case SAL:
        salInput(input);  // calls sal function
        break;
      case CJT:
        cjtInput(input);  // calls cjt function
        break;
      case DEL:
        delInput(input);  // calls del function
        break;
      case ROM:
        romInput(input);  // calls rom function
        break;
      case DEBUG:
        if (input.substring(0, 6) == "DEBUG:") {  // prints DEBUG: messages into the serial monitor
          Serial.println(input);
          // Serial.println(EEPROMactive);
          // Serial.println(romCode);
          // Serial.println(lastEmployee);
          // Serial.println(currentEmployee);
        } else {
          Serial.println(F("ERROR: DEBUG: command was entered incorrectly."));
        }
        break;
      case ERROR:
        Serial.println(F("ERROR: Command entered was not recognised."));
        break;
      default:  // default case to break incase case is not recognised (which is unlikely)
        break;
    }
    Serial.println("DONE!");  // print DONE! everytime the program processes a serial command
    lcd.clear();
  }
}
// ID checker function
void IDchecker(String input) {
  IDcheck = input.substring(4, 11);  // if ID doesn't only contain numbers 0-9, send an error
  for (int i = 0; i <= 6; i++) {
    if (!isDigit(IDcheck[i])) {
      Serial.println(F("ERROR: ID contains characters other than numbers."));
      IDreturn = true;  // assign IDreturn to return from the next function if ID is invalid
      return;
    }
  }
}
// Sort employees by ID in numerical order via bubble sort
void sortEmployees() {
  for (int i = 0; i < lastEmployee - 1; i++) {
    for (int j = 0; j < lastEmployee - i - 1; j++) {
      if (employees[j].ID > employees[j + 1].ID) {
        employee temp = employees[j];
        employees[j] = employees[j + 1];
        employees[j + 1] = temp;
      }
    }
  }
}
// add function
void addInput(String input) {
  if (lastEmployee == 22) {
    Serial.println(F("ERROR: Employee list has reached its maximum. Please delete employee data to continue."));
    return;
  }
  if (input.length() < 17 || input.length() > 31) {  // if input length is the incorrect length, send an error
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-' || input.charAt(11) != '-' || input.charAt(13) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    IDchecker(input);  // calls ID checker function and returns from main loop if it doesn't fit ID format
    if (IDreturn == true) return;
    if (isDigit(input.charAt(12)) == false || input.charAt(12) == '0') {  // if job grade isn't a number 1-9, send an error
      Serial.println(F("ERROR: Job grade isn't a number between 1-9."));
      return;
    }
  }
  for (int i = 0; i <= 22; i++) {  // if ID matches another employee, send an error
    if (IDcheck == employees[i].ID) {
      Serial.println(F("ERROR: ID already matches another employee."));
      return;
    }
  }
  for (int i = 15; i < input.length(); i++) {  // if job title contains characters that aren't alphanumeric, _s or .s, send an error
    if (!isAlpha(input.charAt(i)) && !isDigit(input.charAt(i)) && input.charAt(i) != '_' && input.charAt(i) != '.') {
      Serial.println(F("ERROR: Job title entered contained characters without letters, numbers, .s or _s"));
      return;
    }
  }
  employees[lastEmployee].ID = IDcheck;  // add all inputted values to the current employee's struct in the array
  employees[lastEmployee].jobGrade = input.charAt(12);
  employees[lastEmployee].jobTitle = input.substring(14, input.length());
  employees[lastEmployee].salary = 0.00;
  employees[lastEmployee].pension = "PEN";
  ++lastEmployee;  // increment the employee so that the next one isn't overroded
}
// pst function
void pstInput(String input) {
  if (input.length() != 15 && input.length() != 22) {  // if the input is the wrong length, send an error
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-' || input.charAt(11) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    IDchecker(input);  // calls ID checker function and returns from main loop if it doesn't fit ID format
    if (IDreturn == true) return;
    if (input.substring(12, input.length()) != "PEN" && input.substring(12, input.length()) != "NPEN") {  // if the last string isn't the desired argument, sends an error
      Serial.println(F("ERROR: Command did not end in PEN or NPEN."));
      return;
    }
  }
  for (int i = 0; i <= 22; i++) {
    if (IDcheck == employees[i].ID) {     // checks for the employee with the matching ID
      if (employees[i].salary == 0.00) {  // if employee has no money assign them the broke variable (for errors)
        broke = true;
      }
      if (employees[i].pension == input.substring(12, input.length())) {
        Serial.println(F("ERROR: Employee pension status hasn't changed."));  // if the employee pension status is unchanged send an error
      } else {
        employees[i].pension = input.substring(12, input.length());  // otherwise, update the pension status
        if (broke == true) {
          Serial.println(F("ERROR: Employee's salary is still £0.00."));
        }
      }
      return;
    }
  }
  Serial.println(F("ERROR: ID does not match an employee ID."));  // if entered ID doesn't match with an employee, then send an error
}
// grd function
void grdInput(String input) {
  if (input.length() != 13) {  // if input length is not the correct length, send an error
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-' || input.charAt(11) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    IDchecker(input);  // calls ID checker function and returns from main loop if it doesn't fit ID format
    if (IDreturn == true) return;
    if (isDigit(input.charAt(12)) == false) {  // dont need to include digit can = 0 as it can't be changed to lower than 1.
      Serial.println(F("ERROR: Job grade isn't a number between 1-9."));
      return;
    }
  }
  for (int i = 0; i <= 22; i++) {
    if (IDcheck == employees[i].ID) {  // checks for the employee with the matching ID
      if (employees[i].jobGrade >= int(input.charAt(12))) {
        Serial.println(F("ERROR: Employee job grade has not changed or is lower than the previous value."));
      } else {
        employees[i].jobGrade = input.charAt(12);  // otherwise, update the job grade status
      }
      return;
    }
  }
  Serial.println(F("ERROR: ID does not match an employee ID."));  // if entered ID doesn't match with an employee, then send an error
}
// sal function
void salInput(String input) {
  int count = 0;  // send an error if the command entered is too short
  if (input.length() < 13) {
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-' || input.charAt(11) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    IDchecker(input);  // calls ID checker function and returns from main loop if it doesn't fit ID format
    if (IDreturn == true) return;
    for (int i = 12; i < input.length(); i++) {
      if (input.charAt(i) == '.') {
        count++;
      } else if (input.charAt(i) != '.' && !isDigit(input.charAt(i)) || count > 1) {  // if salary contains more than one . or characters other than numbers, send an error
        Serial.println(F("ERROR: Salary entered contained characters other than numbers."));
        return;
      }
    }
    if (input.substring(12, input.length()).toFloat() > 99999.99) {  // if the salary entered is not between 0.00 - 99999.99, send an error
      Serial.println(F("ERROR: Command entered is the incorrect length."));
      return;
    }
  }
  for (int i = 0; i <= 22; i++) {
    if (IDcheck == employees[i].ID) {                               // checks for the employee with the matching ID
      if (input.substring(12, input.length()).toFloat() == 0.00) {  // if employee has no money assign them the broke variable (for errors)
        Serial.println(F("ERROR: Employee's salary is still £0.00."));
      } else {
        employees[i].salary = (input.substring(12, input.length())).toFloat();  // otherwise, update the salary
      }
      return;
    }
  }
  Serial.println(F("ERROR: ID does not match an employee ID."));  // if entered ID doesn't match with an employee, then send an error
}
// cjt function
void cjtInput(String input) {
  if (input.length() < 15 || input.length() > 29) {  // if command isn't the correct length, send an error
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-' || input.charAt(11) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    IDchecker(input);  // calls ID checker function and returns from main loop if it doesn't fit ID format
    if (IDreturn == true) return;
  }
  for (int i = 12; i < input.length(); i++) {  // if job title contains characters that aren't alphanumeric, _s or .s, send an error
    if (!isAlpha(input.charAt(i)) && !isDigit(input.charAt(i)) && input.charAt(i) != '_' && input.charAt(i) != '.') {
      Serial.println(F("ERROR: Job title entered contained characters without letters, numbers, .s or _s"));
      return;
    }
  }
  for (int i = 0; i <= 22; i++) {
    if (IDcheck == employees[i].ID) {                                      // checks for the employee with the matching ID
      if (employees[i].jobTitle == input.substring(12, input.length())) {  // sends an error if the job title is unchanged
        Serial.println(F("ERROR: Employee job title matches its previous value."));
      } else {
        employees[i].jobTitle = input.substring(12, input.length());  // otherwise, update the job title
      }
      return;
    }
  }
  Serial.println(F("ERROR: ID does not match an employee ID."));  // if entered ID doesn't match with an employee, then send an error
}
// del function
void delInput(String input) {
  if (input.length() != 11) {
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    IDchecker(input);  // calls ID checker function and returns from main loop if it doesn't fit ID format
    if (IDreturn == true) return;
  }
  for (int i = 0; i <= 22; i++) {
    if (IDcheck == employees[i].ID) {  // checks for the employee with the matching ID
      employees[i].ID = ' ';           // sets all deleted employee's data to null
      employees[i].jobGrade = ' ';
      employees[i].jobTitle = ' ';
      employees[i].salary = NULL;
      employees[i].pension = ' ';
      for (int j = i; j < lastEmployee; j++) {
        employees[j] = employees[j + 1];  // decrement all employee information by 1 from the deleted employee up to the current employee
      }
      if (i == currentEmployee && i != 0) {  // if the deleted employee is the current employee and not the first employee on the list, switch to the previous employee
        currentEmployee = --i;
      }
      --lastEmployee;  // decrement the number of total employees
      return;
    }
  }
  Serial.println(F("ERROR: ID does not match an employee ID."));  // if entered ID doesn't match with an employee, then send an error
}
void romInput(String input) {
  if (input.length() < 5 || input.length() > 14) {  // if command entered is the incorrect length, send an error
    Serial.println(F("ERROR: Command entered is the incorrect length."));
    return;
  } else if (input.charAt(3) != '-') {  // inbetween characters are not dashes, send an error
    Serial.println(F("ERROR: Command has dashes in incorrect placement."));
    return;
  } else {
    memset(charArray, 0, sizeof(charArray));        // erases array data
    for (int i = 0; i < input.length() - 4; i++) {  //ensure that all digits in the command are alphabetic
      if (!isalpha(input.substring(4, input.length())[i])) {
        Serial.println(F("ERROR: ROM code entered contained digits other than alphanumeric characters."));
        return;
      }
    }
    input.substring(4, input.length()).toCharArray(charArray, sizeof(charArray));  // convert the command to a char array
    for (int i = 0; i < sizeof(charArray); i++) {
      readArray[i] = EEPROM.read(i);  // read all values from the EEPROM to an array
    }
    romCode = String(readArray);                          // convert the array to a string
    if (input.substring(4, input.length()) == romCode) {  // if the EEPROM string is the same as the command, deactivate EEPROM
      romCode = ' ';
      EEPROMactive = false;
      EEPROM.write(19, EEPROMactive);
      return;
    } else {  // otherwise, write the new command into EEPROM and activate EEPROM
      for (int i = 0; i < sizeof(charArray); i++) {
        EEPROM.write(i, charArray[i]);
      }
      EEPROMactive = true;
      EEPROM.write(19, EEPROMactive);
    }
  }
}
// necessary setup information for arduino
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setBacklight(3);  // turns LCD background to yellow
  Serial.setTimeout(500);
  for (int i = 0; i < sizeof(readArray); i++) {  // read the ROM value stored in the EEPROM and convert to a string
    readArray[i] = EEPROM.read(i);
  }
  romCode = String(readArray);
  EEPROMactive = EEPROM.read(19);  // read to see if EEPROM is active
  while (syncPhase == true) {
    String input = Serial.readStringUntil('\r\n');  // during sync phase, constantly poll to read the input
    if (millis() - time >= 2000) {                  // print R to serial monitor every 2 seconds
      Serial.print('R');
      time = millis();
    }
    if (EEPROMactive == false) {  // if EEPROM is deactivated, start sync phase as normal
      beginInput(input);
    } else {  // otherwise, enter code to start main phase
      if (input == romCode) {
        Serial.print(F("UDCHARS,FREERAM,EEPROM,SCROLL"));  // print UDCHARS,FREERAM,EEPROM,SCROLL to the serial monitor and set the LCD background to white
        Serial.print('\n');
        lcd.setBacklight(7);
        syncPhase = false;  // begin main sequence
        return;
      } else if (input != NULL) {
        Serial.println('R');  // error if input command is not the ROM code
        Serial.println(F("Error: String entered was not recognised."));
      }
    }
  }
  byte upArrow[8] = { // create up arrow character to display on LCD
                      0b00100,
                      0b01110,
                      0b10101,
                      0b00100,
                      0b00100,
                      0b00100,
                      0b00100,
                      0b00100
  };
  lcd.createChar(1, upArrow);
  byte downArrow[8] = { // create down arrow character to display on LCD
                        0b00100,
                        0b00100,
                        0b00100,
                        0b00100,
                        0b00100,
                        0b10101,
                        0b01110,
                        0b00100
  };
  lcd.createChar(0, downArrow);
}
// system loop
void loop() {
  mainPhase();  // calls the main phase function
  sortEmployees();
  unsigned long selectTime = millis();  // times the beginning of when select is pressed
  uint8_t buttons = lcd.readButtons();
  bool clearLCD = false;
  while (buttons & BUTTON_SELECT) {
    buttons = lcd.readButtons();          // reads buttons constantly
    if (millis() - selectTime >= 1000) {  // after 1 second clears the LCD, sets the background to purple, and prints the student ID and free SRAM to the LCD
      if (clearLCD == false) {
        lcd.clear();
        clearLCD = true;
      }
      lcd.setBacklight(5);
      mainPhase();
      lcd.setCursor(0, 0);
      lcd.print(F("ID: F418497"));
      lcd.setCursor(0, 1);
      lcd.print(F("Free SRAM:"));
      lcd.setCursor(11, 1);
      lcd.print(freeMemory());
    }
  }
  if (lastEmployee >= 1) {                                             // if an employee is added to the array begin reading to LCD
    String jobGradeStr = String(employees[currentEmployee].jobGrade);  // convert job grade to string
    lcd.setCursor(1, 0);
    lcd.print(jobGradeStr + ' ');  // empty char spaces ensure that LCD can't produce characters between these spaces
    lcd.setCursor(7, 0);
    lcd.print(' ');
    lcd.setCursor(3, 0);
    lcd.print(employees[currentEmployee].pension + ' ');  // display each employee's information on the LCD
    lcd.setCursor(8, 0);
    lcd.print(employees[currentEmployee].salary);
    lcd.setCursor(1, 1);
    lcd.print(employees[currentEmployee].ID + ' ');
    lcd.setCursor(9, 1);
    if (employees[currentEmployee].jobTitle.length() <= 7) {  // if the job title is smaller than 7 characters, don't scroll
      lcd.print(employees[currentEmployee].jobTitle + ' ' + ' ');
    } else if (q <= employees[currentEmployee].jobTitle.length() - 7) {  // otherwise scroll at 1 character every 0.5s
      if (millis() - time >= 500) {
        lcd.setCursor(9, 1);
        lcd.print(employees[currentEmployee].jobTitle.substring(q, employees[currentEmployee].jobTitle.length()));
        time = millis();
        q++;
      }
    } else {
      q = 0;  // reset the scroll when the job title has reached its end
    }
    if (employees[currentEmployee].pension == "PEN") {
      lcd.setBacklight(2);  // turn LCD green when current employee has a pension
    } else if (employees[currentEmployee].pension == "NPEN") {
      lcd.setBacklight(1);  // turn LCD red when current employee doesn't have a pension
    }
    if (lastEmployee == 1) {  // when there is only one employees don't display any arrows
      lcd.setCursor(0, 0);
      lcd.print(' ');
      lcd.setCursor(0, 1);
      lcd.print(' ');
    } else if (currentEmployee == 0) {  // when there is more than one employee but it's at the beginning of the array only display one arrow
      lcd.setCursor(0, 0);
      lcd.write((uint8_t)1);
      lcd.setCursor(0, 1);
      lcd.print(' ');
      if (buttons & BUTTON_UP) {  // press the up button to scroll to the next employee
        ++currentEmployee;
        lcd.clear();
      }
    } else if (currentEmployee == lastEmployee - 1) {  // when there is more than one employee but it's at the end of the array only display one arrow
      lcd.setCursor(0, 0);
      lcd.print(' ');
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)0);
      if (buttons & BUTTON_DOWN) {  // press the down button to scroll to the previous employee
        --currentEmployee;
        lcd.clear();
      }
    } else {
      lcd.setCursor(0, 0);  // when there are more than one employees but not at the end or beginning of the array, display both arrows
      lcd.write((uint8_t)1);
      lcd.setCursor(0, 1);
      lcd.write((uint8_t)0);
      ;
      if (buttons & BUTTON_UP) {  // press the up button to scroll to the next employee and the down button to scroll to the previous
        ++currentEmployee;
        lcd.clear();
      } else if (buttons & BUTTON_DOWN) {
        --currentEmployee;
        lcd.clear();
      }
    }
  } else {
    lcd.clear();  // clears the LCD  and sets LCD background to white if all employees are deleted
    lcd.setBacklight(7);
  }
}