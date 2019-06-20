#define PIN_TDI 5
#define PIN_TDO 6
#define PIN_TCK 7
#define PIN_TMS 8

#define PIN_LED 13

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  // Set up the JTAG pins
  pinMode(PIN_TDI, OUTPUT);
  pinMode(PIN_TDO, INPUT);
  pinMode(PIN_TCK, OUTPUT);
  pinMode(PIN_TMS, OUTPUT);
  
  pinMode(PIN_LED, OUTPUT);

  Serial.begin(115200);
  delay(500);

  Serial.println("Power On");
  Serial.println("Avaliable commands: getid");
}

// Don´t care
#define SHIFTRAW_OPTION_NONE  0

// If no TMS shift array is specified, shift at least “1” at the
// last shift, to change to the next TAP controller state
#define SHIFTRAW_OPTION_LASTTMS_ONE 1

int getbit (byte* bits, int idx)
{
  byte b = 0;

  b = bits[idx / 8];

  return bitRead(b, idx % 8);
}

void setbit (byte* bits, int idx, int value)
{
  byte x = 0;

  x = bits[idx / 8];
  bitWrite(x, idx % 8, value);

  bits[idx / 8] = x;
}

int U_TAPAccessShiftRaw (int numberofbits, byte* pTMSBits,
                         byte* pTDIBits, byte* pTDOBits,
                         int options)
{
  int i = 0;
  int bit = 0;

  if (numberofbits < 0) return -1;
  
  for (i = 0; i < numberofbits; i++)
  {
    if (NULL != pTDIBits)
    {
        if (getbit(pTDIBits, i))
        {
          digitalWrite(PIN_TDI, HIGH);
          Serial.print("1 ");
        }
        else
        {
          digitalWrite(PIN_TDI, LOW);
          Serial.print("0 ");
        }
    }
//    else
//    {
//      digitalWrite(PIN_TDI, LOW); // debug
//    }

    if (NULL != pTMSBits)
    {
      if (getbit(pTMSBits, i))
      {
        digitalWrite(PIN_TMS, HIGH);
      }
      else
      {
        digitalWrite(PIN_TMS, LOW);
      }
    }
    else
    {
      if (SHIFTRAW_OPTION_LASTTMS_ONE == options && i == (numberofbits - 1)) // last bit
      {
        digitalWrite(PIN_TMS, HIGH);
      }
      else
      {
        digitalWrite(PIN_TMS, LOW);
      }
    }
    
    delay(1);

    digitalWrite(PIN_TCK, HIGH);
    delay(5);

    if (NULL != pTDOBits)
    {
      bit = digitalRead(PIN_TDO);
      setbit(pTDOBits, i, bit);
    }

    digitalWrite(PIN_TCK, LOW);
    delay(5);
  }

  Serial.println("");
  return 0;
}

//
// move TAP controller to Shift-DR state, read the 32 bit
// ID Code and return to Run-Test/Idle state
//
int U_TAPAccessShiftDR (int numberofbits, byte* pTDIBits, byte* pTDOBits)
{
  byte nTMSBits[4];

  // move TAP controller to Shift-DR state
  nTMSBits[0] = 0x1; // TMS sequence 100
  U_TAPAccessShiftRaw(3, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);

  // read ID code and leave Shift-DR state with last bit
  U_TAPAccessShiftRaw(numberofbits, 0, pTDIBits, pTDOBits, SHIFTRAW_OPTION_LASTTMS_ONE);

  // return to Run-Test/Idle
  nTMSBits[0] = 0x1; // TMS sequence 10
  U_TAPAccessShiftRaw(2, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);
  return 0;
}

int U_TAPAccessShiftIR (int numberofbits, byte* pTDIBits, byte* pTDOBits)
{
  byte nTMSBits[4];

  // move TAP controller to Shift-IR state
  nTMSBits[0] = 0x3; // TMS sequence 1100
  U_TAPAccessShiftRaw(4, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);

  // send in IR command and leave Shift-IR state with last bit
  U_TAPAccessShiftRaw(numberofbits, 0, pTDIBits, pTDOBits, SHIFTRAW_OPTION_LASTTMS_ONE);

  // return to Run-Test/Idle
  nTMSBits[0] = 0x1; // TMS sequence 10
  U_TAPAccessShiftRaw(2, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);
  return 0;
}

//
// TAP Controller may be in unknown state,
// reset via TMS and move to Run-test/IDLE
//
int U_TAPAccessIdle (void)
{
  byte nTMSBits[4];

  nTMSBits[0] = 0x1f; // TMS sequence: 1 1 1 1 1 0
  U_TAPAccessShiftRaw(6, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);

  return 0;
}

int GetDAPID()
{
  byte nTDOBits[4];

  int nIDCode = 0;

  U_TAPAccessIdle();

  U_TAPAccessShiftDR(32, 0, nTDOBits);

  nIDCode = ((int)nTDOBits[3]<< 24) | ((int)nTDOBits[2]<<16) |
            ((int)nTDOBits[1]<<8)    | (int)nTDOBits[0];

  return nIDCode;
}

int GetDAPID2()
{
  byte nTDIBits[4];
  byte nTDOBits[4];

  int nIDCode = 0;

  U_TAPAccessIdle();
  
  nTDIBits[0] = 0xE; // IDCODE: 1110
  U_TAPAccessShiftIR(4, nTDIBits, 0);

  U_TAPAccessShiftDR(32, 0, nTDOBits);

  nIDCode = ((int)nTDOBits[3]<< 24) | ((int)nTDOBits[2]<<16) |
            ((int)nTDOBits[1]<<8)    | (int)nTDOBits[0];

  return nIDCode;
}

int test (void)
{
  byte nTDIBits[5] = {0};
  byte nTDOBits[5] = {0};

  U_TAPAccessIdle();
  
  nTDIBits[0] = 0xA; // DPACC: 1010
  U_TAPAccessShiftIR(4, nTDIBits, 0);

//  // Write 0x50000000 to DP.CTRL/STAT register
//  // CTRL/STAT A[3:2] 01
//  // RnW  Write 0
//  nTDIBits[0] = 0x2;
//  nTDIBits[1] = 0;
//  nTDIBits[2] = 0;
//  nTDIBits[3] = 0x8;
//  nTDIBits[4] = 0x2;
//  U_TAPAccessShiftDR(35, nTDIBits, 0);
//
//  // Write 0x50000000 to the DP.CTRL/STAT register
//  // poll the DP.CTRL/STAT register for 0xf0000000
//  // actually, I got 0x30000000, says CSYSPWRUPREQ and CSYSPWRUPACK unset, 
//  // CDBGPWRUPREQ and CDBGPWRUPACK set
//
//  Serial.println(" > Test write DP CTRL/STAT DATAIN 0x50000000, A[3:2] 01, W");
//  U_TAPAccessShiftDR(35, 0, nTDOBits);
//
//  Serial.print(" > Test read DP CTRL/STAT: ");
//  for (int i = 4; i >= 0; i--)
//  {
//    Serial.print(nTDOBits[i], HEX);
//  }
//  Serial.println("");


  // write 0x0 to the DP.SELECT register to activatie the AP
  // at position 0 on the DAP bus

  // DP.SELECT A[3:2] 10
  // RnW  WRITE 0
  nTDIBits[0] = 0x84;
  nTDIBits[1] = 0x07; 
  nTDIBits[2] = 0x0;
  nTDIBits[3] = 0x0;
  nTDIBits[4] = 0x0;
  U_TAPAccessShiftDR(35, nTDIBits, 0);
  

  //
  nTDIBits[0] = 0xB; // APACC: 1011
  U_TAPAccessShiftIR(4, nTDIBits, 0);

  // A[3:2] 11  the forth register IDR
  // RnW READ 1
  nTDIBits[0] = 0x7;
  nTDIBits[1] = 0x0;
  nTDIBits[2] = 0x0;
  nTDIBits[3] = 0x0;
  nTDIBits[4] = 0x0;
  U_TAPAccessShiftDR(35, nTDIBits, 0);

  Serial.println(" > Test write DP.SELECT APACC");
  
  U_TAPAccessShiftDR(35, 0, nTDOBits);

  Serial.print(" > Test read AP IDR: ");
  for (int i = 4; i >= 0; i--)
  {
    Serial.print(nTDOBits[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
  
  return 0;
}

int dp_rdbuff (void)
{
  byte nTDIBits[5];
  byte nTDOBits[5];

  U_TAPAccessIdle();
  
  nTDIBits[0] = 0xA; // DPACC: 1010
  U_TAPAccessShiftIR(4, nTDIBits, 0);

  //
  // RDBUFF A[3:2] 11
  // RnW  READ 1
  nTDIBits[0] = 0x7;
  nTDIBits[1] = 0;
  nTDIBits[2] = 0;
  nTDIBits[3] = 0x0;
  nTDIBits[4] = 0x0;
  U_TAPAccessShiftDR(35, nTDIBits, 0);

  U_TAPAccessShiftDR(35, 0, nTDOBits);

  Serial.println(" > Test read DP.RDBUFF, should be 0");
  for (int i = 4; i >= 0; i--)
  {
    Serial.print(nTDOBits[i], HEX);
  }
  Serial.println("");
  
  return 0;
}

int dp_dpidr (void)
{
  byte nTDIBits[5];
  byte nTDOBits[5];

  U_TAPAccessIdle();
  
  nTDIBits[0] = 0xA; // DPACC: 1010
  U_TAPAccessShiftIR(4, nTDIBits, 0);

  //
  // DPIDR A[3:2] 0
  // RnW  READ 1
  nTDIBits[0] = 0x1;
  nTDIBits[1] = 0;
  nTDIBits[2] = 0;
  nTDIBits[3] = 0x0;
  nTDIBits[4] = 0x0;
  U_TAPAccessShiftDR(35, nTDIBits, 0);

  U_TAPAccessShiftDR(35, 0, nTDOBits);

  Serial.println(" > Test read DP.DPIDR");
  for (int i = 4; i >= 0; i--)
  {
    Serial.print(nTDOBits[i], HEX);
  }
  Serial.println("");
  
  return 0;
}


void loop() {
  // put your main code here, to run repeatedly:
  String incoming = "";   // for incoming serial string data

  delay(1000);
  digitalWrite(PIN_LED, HIGH);
  delay(1000);

  

  if (Serial.available() > 0) 
  {
    // read the incoming:
    incoming = Serial.readString();
    // say what you got:
    Serial.print(incoming);  
      
    if (incoming == "getid\n") 
    {
      int nIDCode = 0;  
      nIDCode = GetDAPID();
      Serial.print(" > DAP ID: ");
      Serial.println(nIDCode, HEX);
    }
    else if (incoming == "getid2\n") 
    {
      int nIDCode = 0;  
      nIDCode = GetDAPID2();
      Serial.print(" > DAP ID: ");
      Serial.println(nIDCode, HEX);
    }  
    else if (incoming == "test\n") 
    { 
      test();
    }
    else if (incoming == "dp_rdbuff\n")
    {
        dp_rdbuff();
    }
    else if (incoming == "dp_dpidr\n")
    {
        dp_dpidr();
    }
    else 
    {
      //invalid command
      Serial.println(" > invalid command.");
      incoming = "";
    } 
    
    Serial.flush(); 
  } 

  digitalWrite(PIN_LED, LOW);
}
