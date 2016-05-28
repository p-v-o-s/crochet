// Crochet
#include <SerialCommand.h>
#include <FreqCount.h>

#define moteinoLED 9   // Arduino LED on board

SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object

void setup() {
  pinMode(moteinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(moteinoLED, LOW);    // default to LED off


  FreqCount.begin(1000); //this is the gateing interval/duration of measurement
  
  Serial.begin(9600);

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("LED.ON",      LED_ON_sCmd_handler);          // Turns LED on
  sCmd.addCommand("LED.OFF",     LED_OFF_sCmd_handler);         // Turns LED off
  sCmd.addCommand("FREQ1.READ?", FREQ1_READ_sCmd_handler);         // reads input frequency
  sCmd.setDefaultHandler(UNRECOGNIZED_sCmd_handler);      // Handler for command that isn't matched  (says "What?")
  //Serial.println("Ready");
}

void loop() {
  int num_bytes = sCmd.readSerial();      // fill the buffer
  if (num_bytes > 0){
    sCmd.processCommand();  // process the command
  }
  delay(10);
}

void FREQ1_READ_sCmd_handler(SerialCommand this_sCmd) {
 if (FreqCount.available()) {
    unsigned long count = FreqCount.read();
    this_sCmd.println(count);
  }
}


void LED_ON_sCmd_handler(SerialCommand this_sCmd) {
  this_sCmd.println("LED on");
  digitalWrite(moteinoLED, HIGH);
}

void LED_OFF_sCmd_handler(SerialCommand this_sCmd) {
  this_sCmd.println("LED off");
  digitalWrite(moteinoLED, LOW);
}


// This gets set as the default handler, and gets called when no other command matches.
void UNRECOGNIZED_sCmd_handler(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print("Did not recognize \"");
  this_sCmd.print(command.name);
  this_sCmd.println("\" as a command.");
}
