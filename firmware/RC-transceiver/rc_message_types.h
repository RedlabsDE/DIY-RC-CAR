uint8_t rc_calculateSum(const uint8_t* pData, uint8_t len);




enum RC_COMMAND_TYPE
{
  COMMAND_TYPE_INVALID = 0,  
  COMMAND_TYPE_PING,  //send from time to time if user data is not changed, to keep communication running
  COMMAND_TYPE_STARTUP,  //send out if RC is switched on
  COMMAND_TYPE_SHUTDOWN_USER, //send out if RC is switched off by user (last command to send)
  COMMAND_TYPE_SHUTDOWN_BATTERY_EMPTY, //send out if RC is forced off by low battery

  COMMAND_TYPE_DATA_HMI, // use to transmit "RC_HMI_DATA"
};

//size of struct must be 32 Byte ??
//packet sent out by remote control
struct RC_COMMAND
{
    // set to "RCC"
    char command_identifier[3];
    
    // count up if struct changes. Current: 0
    uint8_t protocol_version;

    // increment by 1 for each new packet sent via radio (used to dissmiss already received packets)
    uint8_t packet_number;

    enum RC_COMMAND_TYPE command_type;

    // current state of buttons and other 
    struct RC_HMI_DATA hmi_data;  

    
    // Checksum over all previous bytes (simple sum over all bytes)
    uint8_t checksum;
} __attribute__ ((packed, aligned(1)));



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t rc_calculateSum(const uint8_t* pData, uint8_t len)
{
    uint8_t res = 0;
    const uint8_t* pEnd = pData + len;
    while (pData != pEnd)
    {
        res += *pData;
        pData++;
    }
    return res;
}
