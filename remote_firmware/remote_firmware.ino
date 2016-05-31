// Crochet

#include <SerialCommand.h>
#include <PacketCommand.h>

#include <FreqCount.h>

#include <SPI.h>
#include <RH_RF95.h>


#define moteinoLED 9   // Arduino LED on board
#define FREQUENCY  915

//Serial Interface for Testing over FDTI
SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object


//Radio interface for field communication
RH_RF95 rf95;
#define PACKETCOMMAND_MAX_COMMANDS 20
PacketCommand pCmd_rf95(PACKETCOMMAND_MAX_COMMANDS); // The PacketCommand object, initialize with any Stream object

void setup() {
  pinMode(moteinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(moteinoLED, LOW);    // default to LED off


  FreqCount.begin(1000); //this is the gateing interval/duration of measurement
  
  Serial.begin(115200);

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("LED.ON",      LED_ON_sCmd_handler);          // Turns LED on
  sCmd.addCommand("LED.OFF",     LED_OFF_sCmd_handler);         // Turns LED off
  sCmd.addCommand("FREQ1.READ?", FREQ1_READ_sCmd_handler);         // reads input frequency
  sCmd.setDefaultHandler(UNRECOGNIZED_sCmd_handler);      // Handler for command that isn't matched  (says "What?")
  Serial.println(F("SerialCommand Ready"));
  
  // Setup callbacks and command for PacketCommand interface
  // Setup callbacks for PacketCommand commands
  pCmd_rf95.addCommand((byte*) "\xFF\x41","LED.ON",     LED_ON_pCmd_handler);            // Turns LED on   ("\x41" == "A")
  pCmd_rf95.addCommand((byte*) "\xFF\x42","LED.OFF",    LED_OFF_pCmd_handler);           // Turns LED off  ("
  //pCmd.registerDefaultHandler(unrecognized);                          // Handler for command that isn't matched  (says "What?")
  pCmd_rf95.registerRecvCallback(rf95_pCmd_recv_callback);
  //pCmd.registerSendCallback(rf95_pCmd_send_callback);
  
  //Bring the Radio
  if (!rf95.init()){
    Serial.print(F("rf95 init failed\n"));
  }
  else {
    Serial.print(F("rf95 init OK - "));
    Serial.print(FREQUENCY);
    Serial.print(F("mhz\n")); 
  }
}

void loop() {
  int num_bytes = sCmd.readSerial();      // fill the buffer
  if (num_bytes > 0){
    sCmd.processCommand();  // process the command
  }
  
  //check to see if there is an available packet
  bool gotPacket = pCmd_rf95.recv();
  if (gotPacket){
    PacketShared::STATUS pcs = pCmd_rf95.processInput(); //will dispatch to handler
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
  this_sCmd.println(F("LED on"));
  digitalWrite(moteinoLED, HIGH);
}

void LED_OFF_sCmd_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("LED off"));
  digitalWrite(moteinoLED, LOW);
}


// This gets set as the default handler, and gets called when no other command matches.
void UNRECOGNIZED_sCmd_handler(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print(F("Did not recognize \""));
  this_sCmd.print(command.name);
  this_sCmd.println(F("\" as a command."));
}
//******************************************************************************
// PacketCommand callback and handlers
#define PACKET_SIZE 32
bool rf95_pCmd_recv_callback(PacketCommand& this_pCmd){
  static byte inputBuffer[PACKET_SIZE];  //a place to put incoming data, for example from a Serial stream or from a packet radio
  uint8_t len = sizeof(inputBuffer);
  //handle incoming packets
  while (rf95.available() > 0) {
    bool success = rf95.recv(inputBuffer, &len);
    if(success){
      digitalWrite(moteinoLED, HIGH); //blink light
      Serial.print("got request: ");
      Serial.println((char*) inputBuffer);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      this_pCmd.assignInputBuffer(inputBuffer, len);
      return true;
    }
    else{
      Serial.println("recv failed");
      return false;
    }
  }
  return false;
}

void LED_ON_pCmd_handler(PacketCommand& this_pCmd) {
  //this_sCmd.println(F("LED on"));
  digitalWrite(moteinoLED, HIGH);
}

void LED_OFF_pCmd_handler(PacketCommand& this_pCmd) {
  //this_sCmd.println(F("LED off"));
  digitalWrite(moteinoLED, LOW);
}

