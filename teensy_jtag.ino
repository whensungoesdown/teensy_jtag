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
        }
        else
        {
          digitalWrite(PIN_TDI, LOW);
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
  U_TAPAccessShiftRaw(32, 0, 0, pTDOBits, SHIFTRAW_OPTION_LASTTMS_ONE);

  // return to Run-Test/Idle
  nTMSBits[0] = 0x1; // TMS sequence 10
  U_TAPAccessShiftRaw(2, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);
  return 0;
}

int GetDAPID()
{
  byte nTMSBits[4];
  byte nTDOBits[4];

  int nIDCode = 0;

  nTMSBits[0] = 0x1f; // TMS sequence: 1 1 1 1 1 0
  U_TAPAccessShiftRaw(6, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);

//  nTMSBits[0] = 0x1; // TMS sequence 100
//  U_TAPAccessShiftRaw(3, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);
//  
//  U_TAPAccessShiftRaw(32, 0, 0, nTDOBits, SHIFTRAW_OPTION_LASTTMS_ONE);
//
//  nTMSBits[0] = 0x1; // TMS sequence 10
//  U_TAPAccessShiftRaw(2, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);

  U_TAPAccessShiftDR(32, 0, nTDOBits);

  nIDCode = ((int)nTDOBits[3]<< 24) | ((int)nTDOBits[2]<<16) |
            ((int)nTDOBits[1]<<8)    | (int)nTDOBits[0];

  return nIDCode;
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
