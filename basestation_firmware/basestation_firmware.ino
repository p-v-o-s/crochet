// Crochet

#include <SerialCommand.h>
#include <PacketCommand.h>

#include <FreqCount.h>

#include <SPI.h>
#include <RH_RF95.h>

//uncomment for debugging messages
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PORT Serial
#endif

#define moteinoLED 9   // Arduino LED on board
#define FREQUENCY  915

//Serial Interface for Testing over FDTI
SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object


//Radio interface for field communication
#define PACKET_SIZE 32
RH_RF95 rf95;

#define PACKETCOMMAND_MAX_COMMANDS 20
#define PACKETCOMMAND_INPUT_BUFFER_SIZE PACKET_SIZE
#define PACKETCOMMAND_OUTPUT_BUFFER_SIZE 32
PacketCommand pCmd_rf95(PACKETCOMMAND_MAX_COMMANDS,
                        PACKETCOMMAND_INPUT_BUFFER_SIZE,
                        PACKETCOMMAND_OUTPUT_BUFFER_SIZE);

//******************************************************************************
// Setup
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
  #ifdef DEBUG
  DEBUG_PORT.println(F("SerialCommand Ready"));
  #endif
  
  // Setup callbacks for PacketCommand requests
  pCmd_rf95.addCommand((byte*) "\xFF\x11","FREQ1.READ?", NULL);
  pCmd_rf95.addCommand((byte*) "\xFF\x41","LED.ON",      NULL);
  pCmd_rf95.addCommand((byte*) "\xFF\x42","LED.OFF",     NULL);
  //Setup callbacks for PacketCommand replies
  pCmd_rf95.addCommand((byte*) "\x11","FREQ1!",          FREQ1_pCmd_reply_handler);
  //pCmd.registerDefaultHandler(unrecognized);                          // Handler for command that isn't matched  (says "What?")
  pCmd_rf95.registerRecvCallback(rf95_pCmd_recv_callback);
  //pCmd.registerSendCallback(rf95_pCmd_send_callback);
  
  //Bring the Radio
  if (!rf95.init()){
    #ifdef DEBUG
    DEBUG_PORT.print(F("rf95 init failed\n"));
    #endif
  }
  else {
    #ifdef DEBUG
    DEBUG_PORT.print(F("rf95 init OK - "));
    DEBUG_PORT.print(FREQUENCY);
    DEBUG_PORT.print(F("mhz\n"));
    #endif
  }
}

//******************************************************************************
// Main Loop

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

//******************************************************************************
// SerialCommand handlers

void FREQ1_READ_sCmd_handler(SerialCommand this_sCmd) {
  pCmd_rf95.resetOutputBuffer();
  pCmd_rf95.setupOutputCommandByName("FREQ1.READ?");
  pCmd_rf95.send();
}

void LED_ON_sCmd_handler(SerialCommand this_sCmd) {
  pCmd_rf95.resetOutputBuffer();
  pCmd_rf95.setupOutputCommandByName("LED.ON");
  pCmd_rf95.send();
}

void LED_OFF_sCmd_handler(SerialCommand this_sCmd) {
  pCmd_rf95.resetOutputBuffer();
  pCmd_rf95.setupOutputCommandByName("LED.OFF");
  pCmd_rf95.send();
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
bool rf95_pCmd_recv_callback(PacketCommand& this_pCmd){
  byte* inputBuffer = this_pCmd.getInputBuffer();
  uint8_t len; //this value is updated by reference
  //handle incoming packets
  if (rf95.available()) {
    bool success = rf95.recv(inputBuffer, &len);
    if(success){
      digitalWrite(moteinoLED, HIGH); //blink light
      #ifdef DEBUG
      DEBUG_PORT.print("got request: ");
      DEBUG_PORT.println((char*) inputBuffer);
      DEBUG_PORT.print("RSSI: ");
      DEBUG_PORT.println(rf95.lastRssi(), DEC);
      #endif
      return true;
    }
    else{
      #ifdef DEBUG
      DEBUG_PORT.println("recv failed");
      #endif
      return false;
    }
  }
  return false;
}

bool rf95_pCmd_send_callback(PacketCommand& this_pCmd){
  byte* outputBuffer = this_pCmd.getOutputBuffer();
  uint8_t len        = this_pCmd.getOutputBufferIndex();
  rf95.send(outputBuffer, len);
}

void FREQ1_pCmd_reply_handler(PacketCommand& this_pCmd){
  uint32_t count;
  this_pCmd.unpack_uint32(count);
  Serial.print("FREQ1: ");Serial.println(count);
}

