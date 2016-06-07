#include "Arduino.h"
#include "pCmd_RHRD_RF95_module.h"

//uncomment for debugging messages
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PORT Serial
#endif

//Radio interface for field communication
RH_RF95 driver_RF95;

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager_RHRD(driver_RF95);

//******************************************************************************
// Setup
void pCmd_RHRD_module_setup(uint8_t this_address,
                            unsigned int radio_frequency,
                            unsigned int num_retries
                            ){
  //Bring up the Radio
  manager_RHRD.setThisAddress(this_address);
  manager_RHRD.setRetries(num_retries);
  if (!manager_RHRD.init()){
    #ifdef DEBUG
    DEBUG_PORT.print(F("# RHRD init failed\n"));
    #endif
  }
  else {
    driver_RF95.setFrequency(radio_frequency);
    #ifdef DEBUG
    DEBUG_PORT.print(F("# rf95 init OK - "));
    DEBUG_PORT.print(radio_frequency);
    DEBUG_PORT.print(F("mhz\n"));
    #endif
  }
}

//******************************************************************************
PacketShared::STATUS pCmd_status = PacketShared::SUCCESS;
bool pCmd_RHRD_module_proccess_input(PacketCommand& this_pCmd) {
  //process incoming radio packets
  if (manager_RHRD.available()) { //there is some radio activity
      //check to see if there is an available packet
      pCmd_status = this_pCmd.recv();
      if (pCmd_status == PacketShared::SUCCESS){
        //blink to signal packet received and processed
        pCmd_status = this_pCmd.processInput(); //will dispatch to handler
        if (pCmd_status == PacketShared::SUCCESS){
          return true;
        } else{
          #ifdef DEBUG
          DEBUG_PORT.print(F("# Error in pCmd_RHRD_module_proccess_input:\n"));
          DEBUG_PORT.print(F("# \tthis_pCmd.processInput returned errocode "));
          DEBUG_PORT.println(pCmd_status);
          #endif
        }
      } else{
          #ifdef DEBUG
          DEBUG_PORT.print(F("# Error in pCmd_RHRD_module_proccess_input:\n"));
          DEBUG_PORT.print(F("# \tthis_pCmd.recv returned errocode "));
          DEBUG_PORT.println(pCmd_status);
          #endif
      }
  }
  return false;
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
  uint32_t recv_millis = millis();
  bool success = manager_RHRD.recvfromAck(inputBuffer, &len, &from_addr); //len should be updated to message length
  if(success){
    //now we hand the buffer back with the new input length
    this_pCmd.assignInputBuffer(inputBuffer, len);
    PacketCommand::InputProperties input_props = this_pCmd.getInputProperties();
    input_props.from_addr      = from_addr;
    input_props.recv_timestamp = recv_millis;
    input_props.RSSI           = driver_RF95.lastRssi();
    this_pCmd.setInputProperties(input_props);
    #ifdef DEBUG
    DEBUG_PORT.print(F("# got request from: 0x"));
    DEBUG_PORT.println(from_addr,HEX);
    DEBUG_PORT.print(F("# \tlen: "));
    DEBUG_PORT.println(len);
    DEBUG_PORT.print(F("# \tinputBuffer: 0x"));
    for(int i=0; i < len; i++){
      if(inputBuffer[i] < 15){DEBUG_PORT.print(0);}
      DEBUG_PORT.print(inputBuffer[i],HEX);
    }
    DEBUG_PORT.println();
    DEBUG_PORT.print(F("# \tRSSI: "));
    DEBUG_PORT.println(input_props.RSSI, DEC);
    #endif
    return true;
  }
  else{
    #ifdef DEBUG
    DEBUG_PORT.println(F("# recv failed"));
    #endif
    return false;
  }
}

bool pCmd_RHRD_send_callback(PacketCommand& this_pCmd){
  byte* outputBuffer = this_pCmd.getOutputBuffer();
  size_t       len   = this_pCmd.getOutputLen();
  uint8_t  to_addr   = (uint8_t) this_pCmd.getOutputToAddress();
  #ifdef DEBUG
  DEBUG_PORT.print(F("# pCmd_RHRD_send_callback to: 0x"));
  DEBUG_PORT.println(to_addr);
  DEBUG_PORT.print(F("# \toutputBuffer: 0x"));
  for(int i=0; i < len; i++){
    if(outputBuffer[i] < 15){DEBUG_PORT.print(0);}
    DEBUG_PORT.print(outputBuffer[i],HEX);
  }
  DEBUG_PORT.println();
  #endif
  bool success = manager_RHRD.sendtoWait(outputBuffer, len, to_addr);
  if(!success){
    #ifdef DEBUG
    DEBUG_PORT.println(F("# send failed"));
    #endif
    return false;
  }
  else{
    return true;
  }
}

