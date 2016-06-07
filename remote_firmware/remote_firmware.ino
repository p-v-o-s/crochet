// Crochet
#include <PacketCommand.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include "pCmd_RHRD_RF95_module.h"

#include <SerialCommand.h>

#include <FreqCount.h>

//uncomment for debugging messages
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PORT Serial
#endif

#define moteinoLED 9   // Arduino LED on board

//Serial Interface for Testing over FDTI
SerialCommand sCmd(Serial);

#define PACKETCOMMAND_MAX_COMMANDS 20
#define PACKETCOMMAND_INPUT_BUFFER_SIZE 32
#define PACKETCOMMAND_OUTPUT_BUFFER_SIZE 32
PacketCommand pCmd_RHRD(PACKETCOMMAND_MAX_COMMANDS,
                        PACKETCOMMAND_INPUT_BUFFER_SIZE,
                        PACKETCOMMAND_OUTPUT_BUFFER_SIZE);

#define BASESTATION_ADDRESS 1
#define DEFAULT_REMOTE_ADDRESS 2
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
  DEBUG_PORT.println(F("# SerialCommand Ready"));
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
  
  //configure the pCmd_RHRD_RF95_module
  pCmd_RHRD_module_setup(DEFAULT_REMOTE_ADDRESS,
                         PCMD_RHRD_DEFAULT_FREQUENCY,
                         PCMD_RHRD_DEFAULT_NUM_RETRIES
                         );
                         
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
  pCmd_RHRD_module_proccess_input(pCmd_RHRD);
  
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
  this_sCmd.println(F("# LED on"));
  digitalWrite(moteinoLED, HIGH);
}

void LED_OFF_sCmd_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("# LED off"));
  digitalWrite(moteinoLED, LOW);
}


// This gets set as the default handler, and gets called when no other command matches.
void UNRECOGNIZED_sCmd_handler(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  this_sCmd.print(F("# Did not recognize \""));
  this_sCmd.print(command.name);
  this_sCmd.println(F("\" as a command."));
}
//******************************************************************************
// PacketCommand handlers
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

