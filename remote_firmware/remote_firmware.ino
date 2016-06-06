// Crochet

#include <SerialCommand.h>
#include <PacketCommand.h>

#include <FreqCount.h>

#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

//uncomment for debugging messages
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PORT Serial
#endif

#define moteinoLED 9   // Arduino LED on board
#define FREQUENCY  915
#define BASESTATION_ADDRESS 1
#define REMOTE_ADDRESS 2

//Serial Interface for Testing over FDTI
SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object


//Radio interface for field communication
#define PACKET_SIZE 32
RH_RF95 driver_RF95;

//// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager_RHRD(driver_RF95, REMOTE_ADDRESS);

#define PACKETCOMMAND_MAX_COMMANDS 20
#define PACKETCOMMAND_INPUT_BUFFER_SIZE PACKET_SIZE
#define PACKETCOMMAND_OUTPUT_BUFFER_SIZE 32
PacketCommand pCmd_RHRD(PACKETCOMMAND_MAX_COMMANDS,
                        PACKETCOMMAND_INPUT_BUFFER_SIZE,
                        PACKETCOMMAND_OUTPUT_BUFFER_SIZE);

//******************************************************************************
// Setup
void setup() {
  pinMode(moteinoLED, OUTPUT);      // Configure the onboard LED for output
  //blink to signal setup
  digitalWrite(moteinoLED, HIGH);
  delay(500);
  digitalWrite(moteinoLED, LOW);     // default to LED off


  FreqCount.begin(1000); //this is the gateing interval/duration of measurement
  
  Serial.begin(115200);

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("LED.ON",      LED_ON_sCmd_handler);          // Turns LED on
  sCmd.addCommand("LED.OFF",     LED_OFF_sCmd_handler);         // Turns LED off
  sCmd.addCommand("FREQ1.READ?", FREQ1_READ_sCmd_handler);         // reads input frequency
  sCmd.setDefaultHandler(UNRECOGNIZED_sCmd_handler);      // Handler for command that isn't matched  (says "What?")
  #ifdef DEBUG
  DEBUG_PORT.println(F("#SerialCommand Ready"));
  #endif
  
  // Setup callbacks and command for PacketCommand interface
  // Setup callbacks for PacketCommand actions and queries
  pCmd_RHRD.addCommand((byte*) "\xFF\x11","FREQ1.READ?", FREQ1_READ_pCmd_query_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x41","LED.ON",      LED_ON_pCmd_handler);            // Turns LED on   ("\x41" == "A")
  pCmd_RHRD.addCommand((byte*) "\xFF\x42","LED.OFF",     LED_OFF_pCmd_handler);           // Turns LED off  ("
  //Setup callbacks for PacketCommand replies
  pCmd_RHRD.addCommand((byte*) "\x11","FREQ1!",          NULL);
  //pCmd.registerDefaultHandler(unrecognized);                          // Handler for command that isn't matched  (says "What?")
  pCmd_RHRD.registerRecvCallback(pCmd_RHRD_recv_callback);
  pCmd_RHRD.registerSendCallback(pCmd_RHRD_send_callback);
  
  //Bring up the Radio
  if (!manager_RHRD.init()){
    #ifdef DEBUG
    DEBUG_PORT.print(F("#RHRD init failed\n"));
    #endif
  }
  else {
    driver_RF95.setFrequency(FREQUENCY);
    #ifdef DEBUG
    DEBUG_PORT.print(F("#rf95 init OK - "));
    DEBUG_PORT.print(FREQUENCY);
    DEBUG_PORT.print(F("mhz\n"));
    #endif
  }
  //configure the default node address
  pCmd_RHRD.setOutputToAddress(BASESTATION_ADDRESS);
}

//******************************************************************************
// Main Loop

void loop() {
  int num_bytes = sCmd.readSerial();      // fill the buffer
  if (num_bytes > 0){
    sCmd.processCommand();  // process the command
  }

  //process incoming radio packets
  if (manager_RHRD.available()) { //there is some radio activity
      //check to see if there is an available packet
      PacketShared::STATUS pcs = pCmd_RHRD.recv();
      if (pcs == PacketShared::SUCCESS){
        //blink to signal packet received and processed
        //digitalWrite(moteinoLED, HIGH);
        pcs = pCmd_RHRD.processInput(); //will dispatch to handler
        //digitalWrite(moteinoLED, LOW);
      }
  }
  
  delay(1000);
}

//******************************************************************************
// SerialCommand handlers

void FREQ1_READ_sCmd_handler(SerialCommand this_sCmd) {
 if (FreqCount.available()) {
    unsigned long count = FreqCount.read();
    this_sCmd.println(count);
  }
}

void LED_ON_sCmd_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("#LED on"));
  digitalWrite(moteinoLED, HIGH);
}

void LED_OFF_sCmd_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("#LED off"));
  digitalWrite(moteinoLED, LOW);
}


// This gets set as the default handler, and gets called when no other command matches.
void UNRECOGNIZED_sCmd_handler(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print(F("#Did not recognize \""));
  this_sCmd.print(command.name);
  this_sCmd.println(F("\" as a command."));
}
//******************************************************************************
// PacketCommand callback and handlers
bool pCmd_RHRD_recv_callback(PacketCommand& this_pCmd){
  //get the buffer address from the pCmd instance
  byte* inputBuffer = this_pCmd.getInputBuffer();
  uint8_t len = this_pCmd.getInputBufferSize(); //this value must be initialized to buffer length will be updated by reference to the actual message length
  uint8_t from_addr;
  //handle incoming packets
  this_pCmd.resetInputBuffer(); //important, reset the index state
  uint32_t recv_timestamp = millis();
  bool success = manager_RHRD.recvfromAck(inputBuffer, &len, &from_addr); //len should be updated to message length
  if(success){
    //now we hand the buffer back with the new input length
    this_pCmd.assignInputBuffer(inputBuffer, len);
    PacketCommand::InputProperties input_props = this_pCmd.getInputProperties();
    input_props.from_addr      = from_addr;
    input_props.recv_timestamp = recv_timestamp;
    input_props.RSSI           = driver_RF95.lastRssi();
    this_pCmd.setInputProperties(input_props);
    //digitalWrite(moteinoLED, HIGH); //blink light
    #ifdef DEBUG
    DEBUG_PORT.println(F("#got request from: 0x"));
    DEBUG_PORT.println(from_addr,HEX);
    DEBUG_PORT.print(F("#\tlen: "));
    DEBUG_PORT.println(len);
    DEBUG_PORT.print(F("#\tinputBuffer: 0x"));
    for(int i=0; i < len; i++){
      if(inputBuffer[i] < 15){DEBUG_PORT.print(0);}
      DEBUG_PORT.print(inputBuffer[i],HEX);
    }
    DEBUG_PORT.println();
    DEBUG_PORT.print(F("#\tRSSI: "));
    DEBUG_PORT.println(input_props.RSSI, DEC);
    #endif
    return true;
  }
  else{
    #ifdef DEBUG
    DEBUG_PORT.println(F("#recv failed"));
    #endif
    return false;
  }
}

bool pCmd_RHRD_send_callback(PacketCommand& this_pCmd){
  byte* outputBuffer = this_pCmd.getOutputBuffer();
  size_t       len   = this_pCmd.getOutputLen();
  uint8_t  to_addr   = (uint8_t) this_pCmd.getOutputToAddress();
  #ifdef DEBUG
  DEBUG_PORT.print(F("#pCmd_RHRD_send_callback to: 0x"));
  DEBUG_PORT.println(to_addr);
  DEBUG_PORT.print(F("#\toutputBuffer: 0x"));
  for(int i=0; i < len; i++){
    if(outputBuffer[i] < 15){DEBUG_PORT.print(0);}
    DEBUG_PORT.print(outputBuffer[i],HEX);
  }
  DEBUG_PORT.println();
  #endif
  bool success = manager_RHRD.sendtoWait(outputBuffer, len, to_addr);
  if(!success){
    #ifdef DEBUG
    DEBUG_PORT.println(F("#send failed"));
    #endif
    return false;
  }
  else{
    return true;
  }
}

void FREQ1_READ_pCmd_query_handler(PacketCommand& this_pCmd) {
 if (FreqCount.available()) {
    uint32_t count = FreqCount.read();
    this_pCmd.resetOutputBuffer();
    this_pCmd.setupOutputCommandByName("FREQ1!");
    this_pCmd.pack_uint32(count);
    this_pCmd.send();
  }
}

void LED_ON_pCmd_handler(PacketCommand& this_pCmd) {
  //this_sCmd.println(F("LED on"));
  digitalWrite(moteinoLED, HIGH);
}

void LED_OFF_pCmd_handler(PacketCommand& this_pCmd) {
  //this_sCmd.println(F("LED off"));
  digitalWrite(moteinoLED, LOW);
}

