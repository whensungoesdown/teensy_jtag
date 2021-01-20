#include <SoftwareSerial.h>

//SIM800 TX is connected to TEENSY D9
#define SIM800_TX_PIN 9

//SIM800 RX is connected to TEENSY D10
#define SIM800_RX_PIN 10

// SIM800 board powerkey
#define SIM800_PWR_PIN 11


#define PIN_TDO 5
#define PIN_TMS 6
#define PIN_TDI 7
#define PIN_TCK 8


#define PIN_LED 13

SoftwareSerial serialSIM800(SIM800_TX_PIN, SIM800_RX_PIN); // RX, TX


void sim800_echo ();

void setup()
{
    // jtag
    // Set up the JTAG pins
    pinMode(PIN_TDI, OUTPUT);
    pinMode(PIN_TDO, INPUT);
    pinMode(PIN_TCK, OUTPUT);
    pinMode(PIN_TMS, OUTPUT);
    // jtag

    

    
    pinMode(PIN_LED, OUTPUT);

    pinMode(SIM800_TX_PIN, OUTPUT);
    pinMode(SIM800_RX_PIN, INPUT);
    pinMode(SIM800_PWR_PIN, OUTPUT);
    

    digitalWrite(PIN_LED, HIGH);

    // press powerkey for 1s
    digitalWrite(SIM800_PWR_PIN, HIGH);
    delay(1000);
    digitalWrite(SIM800_PWR_PIN, LOW);
    
    Serial.begin(9600);
    //while (!Serial);
    delay(1000); // wait or delay 1s

// jtag
    Serial.println("Teensy32 Power On\n");
    Serial.println("Avaliable commands:\n"
                   "    getid\n"
                   "    get_ap0idr\n"
                   "    readmem <address>\n"
                   "    writemem <address> <value>\n\n"
                   "#\n");
// jtag
    //Being serial communication with Arduino and SIM800

    serialSIM800.begin(9600);
    delay(1000);
    Serial.println("Sim800 Power ON");
    
    delay(3000);
    serialSIM800.println("AT+CMGF=1"); // Configuring TEXT mode
    sim800_echo();

    delay(500);
    serialSIM800.println("AT+CNMI=1,2,0,0,0"); // Decides how newly arrived SMS 
    sim800_echo();
}

// jtag
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
                //Serial.print("1 ");
            }
            else
            {
                digitalWrite(PIN_TDI, LOW);
                //Serial.print("0 ");
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

    //Serial.println("");
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

    nTMSBits[0] = 0x3f; // TMS sequence: 1 1 1 1 1 1 0
    U_TAPAccessShiftRaw(7, nTMSBits, 0, 0, SHIFTRAW_OPTION_NONE);

    return 0;
}

// byte str 5 bytes long
int accreg2fields (byte* bytestr, int* pValue, int* pStatus)
{
    int i = 0;

    *pStatus = 0;

    for (i = 0; i < 3; i++)
    {
        setbit((byte*)pStatus, i, getbit(bytestr, i));
    }

    *pValue = 0;

    for (i = 0; i < 32; i++)
    {
        setbit((byte*)pValue, i, getbit(bytestr, i + 3));
    }

    return 0;
}

// byte str 5 bytes long
int fields2accreg (byte* bytestr, int value, int A32, int RnW)
{
    int i = 0;
    memset(bytestr, 0, 5);

    setbit(bytestr, 0, getbit((byte*)&RnW, 0));
    setbit(bytestr, 1, getbit((byte*)&A32, 0));
    setbit(bytestr, 2, getbit((byte*)&A32, 1));

    for (i = 3; i < 35; i++)
    {
        setbit(bytestr, i, getbit((byte*)&value, i - 3));
    }

    return 0;
}

int GetDAPID()
{
    byte nTDOBits[4];

    int nIDCode = 0;

    U_TAPAccessIdle();

    U_TAPAccessShiftDR(32, 0, nTDOBits);

    nIDCode = ((int)nTDOBits[3] << 24) | ((int)nTDOBits[2] << 16) |
              ((int)nTDOBits[1] << 8)    | (int)nTDOBits[0];

    return nIDCode;
}

int GetDAPID2()
{
    byte nTDIBits[4];
    byte nTDOBits[4];

    int nIDCode = 0;

    U_TAPAccessIdle();

    nTDIBits[0] = 0xE; // IDCODE: 0b1110
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    U_TAPAccessShiftDR(32, 0, nTDOBits);

    nIDCode = ((int)nTDOBits[3] << 24) | ((int)nTDOBits[2] << 16) |
              ((int)nTDOBits[1] << 8)    | (int)nTDOBits[0];

    return nIDCode;
}

int get_ap0idr (void)
{
    byte nTDIBits[5] = {0};
    byte nTDOBits[5] = {0};

    int AP_IDR = 0;
    int value = 0;
    int status = 0;

    U_TAPAccessIdle();

    nTDIBits[0] = 0xA; // DPACC: 0b1010
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    
    // Write 0x50000000 to DP.CTRL/STAT register
    // CTRL/STAT A[3:2] 01
    // RnW  Write 0
    fields2accreg(nTDIBits, 0x50000000, 0x1, 0x0);


    U_TAPAccessShiftDR(35, nTDIBits, 0);

    Serial.println(" > Write DP.CTRL/STAT DATAIN 0x50000000, A[3:2] 01, W");
    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &value, &status);

    Serial.print(" > DP.CTRL/STAT: ");
    Serial.print(value, HEX);
    Serial.print("  status: ");
    Serial.println(status, HEX);


    // write 0x0 to the DP.SELECT register to activatie the AP
    // at position 0 on the DAP bus

    // Data: ap 0, apbank 0xf
    // DP.SELECT A[3:2] 10
    // RnW  WRITE 0
    //nTDIBits[0] = 0x84;
    //nTDIBits[1] = 0x07;
    //nTDIBits[2] = 0x0;
    //nTDIBits[3] = 0x0;
    //nTDIBits[4] = 0x0;

    fields2accreg(nTDIBits, 0x000000F0, 0x2, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);


    //
    nTDIBits[0] = 0xB; // APACC: 0b1011
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    // A[3:2] 11  the forth register IDR
    // RnW READ 1
    //nTDIBits[0] = 0x7;
    //nTDIBits[1] = 0x0;
    //nTDIBits[2] = 0x0;
    //nTDIBits[3] = 0x0;
    //nTDIBits[4] = 0x0;
    fields2accreg(nTDIBits, 0x00000000, 0x3, 0x1);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    //fields2accreg(nTDIBits, 0x50000000, 0x1, 0x0);

    Serial.println(" > Write DP.SELECT APACC");

    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &AP_IDR, &status);

    Serial.print(" > AP IDR: ");
    Serial.print(AP_IDR, HEX);
    Serial.print("  status: ");
    Serial.println(status, HEX);

    return 0;
}

int readmem(int address)
{
    byte nTDIBits[5] = {0};
    byte nTDOBits[5] = {0};

    int value = 0;
    int status = 0;

    char formatbuf[64] = {0};

    U_TAPAccessIdle();

    nTDIBits[0] = 0xA; // DPACC: 0b1010
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    // select ap0, bank 0   a[3:2]:DP.SELECT
    fields2accreg(nTDIBits, 0x00000000, 0x2, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    // Write 0x50000000 to DP.CTRL/STAT register
    // CTRL/STAT A[3:2] 01
    // RnW  Write 0
    fields2accreg(nTDIBits, 0x50000000, 0x1, 0x0);

    //  Serial.println(" > TDIBits: ");
    //  for (int i = 4; i >= 0; i--)
    //  {
    //    Serial.print(nTDIBits[i], HEX);
    //    Serial.print(" ");
    //  }
    //  Serial.println("");

    U_TAPAccessShiftDR(35, nTDIBits, 0);

    Serial.println(" > Write DP.CTRL/STAT DATAIN 0x50000000, A[3:2] 01, W");
    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &value, &status);

    Serial.print(" > DP.CTRL/STAT: ");
    Serial.print(value, HEX);
    Serial.print("  status: ");
    Serial.println(status, HEX);

    // DP.APSEL 0, BANK 0, WRITE 0
    fields2accreg(nTDIBits, 0x0, 0x2, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    nTDIBits[0] = 0xB; // APACC: 0b1011
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    // AP.CSW 0 WRITE 0
    fields2accreg(nTDIBits, 0x22000012, 0x0, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);


    // AP.TAR 1 WRITE 0
    // address
    fields2accreg(nTDIBits, address, 0x1, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    // AP.DRW 0x3 READ 1
    fields2accreg(nTDIBits, 0x0, 0x3, 0x1);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &value, &status);

    sprintf(formatbuf, " > address 0x%x: 0x%08x, status 0x%x\n", address, value, status);
    Serial.println(formatbuf);

    return 0;
}

int writemem(int address, int value)
{
    byte nTDIBits[5] = {0};
    byte nTDOBits[5] = {0};

    int retvalue = 0;
    int status = 0;

    char formatbuf[64] = {0};

    U_TAPAccessIdle();

    nTDIBits[0] = 0xA; // DPACC: 0b1010
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    // Write 0x50000000 to DP.CTRL/STAT register
    // CTRL/STAT A[3:2] 01
    // RnW  Write 0
    fields2accreg(nTDIBits, 0x50000000, 0x1, 0x0);

    //  Serial.println(" > TDIBits: ");
    //  for (int i = 4; i >= 0; i--)
    //  {
    //    Serial.print(nTDIBits[i], HEX);
    //    Serial.print(" ");
    //  }
    //  Serial.println("");

    U_TAPAccessShiftDR(35, nTDIBits, 0);

    Serial.println(" > Write DP.CTRL/STAT DATAIN 0x50000000, A[3:2] 01, W");
    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &retvalue, &status);

    Serial.print(" > DP.CTRL/STAT: ");
    Serial.print(retvalue, HEX);
    Serial.print("  status: ");
    Serial.println(status, HEX);

    // DP.APSEL 0, BANK 0, WRITE 0
    fields2accreg(nTDIBits, 0x0, 0x2, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    nTDIBits[0] = 0xB; // APACC: 0b1011
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    // AP.CSW 0 WRITE 0
    fields2accreg(nTDIBits, 0x22000012, 0x0, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);


    // AP.TAR 1 WRITE 0
    // address
    fields2accreg(nTDIBits, address, 0x1, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    // AP.DRW 0x3 WRITE 0  data: value
    fields2accreg(nTDIBits, value, 0x3, 0x0);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    sprintf(formatbuf, " > address 0x%x: 0x%08x, status 0x%x\n", address, value, status);

    Serial.println(formatbuf);

    return 0;
}

int dp_rdbuff (void)
{
    byte nTDIBits[5];
    byte nTDOBits[5];

    U_TAPAccessIdle();

    nTDIBits[0] = 0xA; // DPACC: 0b1010
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

    int value = 0;
    int status = 0;

    U_TAPAccessIdle();

    nTDIBits[0] = 0xA; // DPACC: 0b1010
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    //
    // DPIDR A[3:2] 0
    // RnW  READ 1

    fields2accreg(nTDIBits, 0x50000000, 0x0, 0x1);
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &value, &status);

    Serial.print(" > DP.DPIDR: ");
    Serial.print(value, HEX);
    Serial.print("  status: ");
    Serial.println(status, HEX);

    return 0;
}

int dp_ctrl (void)
{
    byte nTDIBits[5];
    byte nTDOBits[5];

    int value = 0;
    int status = 0;

    U_TAPAccessIdle();

    nTDIBits[0] = 0xA; // DPACC: 0b1010
    U_TAPAccessShiftIR(4, nTDIBits, 0);

    //
    // DPIDR A[3:2] 01
    // RnW  READ 1
    nTDIBits[0] = 0x3;
    nTDIBits[1] = 0;
    nTDIBits[2] = 0;
    nTDIBits[3] = 0x0;
    nTDIBits[4] = 0x0;
    U_TAPAccessShiftDR(35, nTDIBits, 0);

    U_TAPAccessShiftDR(35, 0, nTDOBits);

    accreg2fields(nTDOBits, &value, &status);

    Serial.print(" > DP.CTRL/STAT: ");
    Serial.print(value, HEX);
    Serial.print("  status: ");
    Serial.println(status, HEX);

    Serial.println(" > Test read DP.CTRL");
    for (int i = 4; i >= 0; i--)
    {
        Serial.print(nTDOBits[i], HEX);
    }
    Serial.println("");

    return 0;
}

// jtag


void GetArg(char* commAndline, int* Argc, char* Argv[], int  mAxArgc)
{
    int length;
    int i = 0;
    int index = 0;
    length = strlen(commAndline);
    Argv[index] = commAndline;
    index++;
    for (i = 0; i < length; i++) {
        if (commAndline[i] == ' ' && commAndline[i + 1] != ' ' && commAndline[i + 1] != '\0') {
            commAndline[i] = '\0';//breAk up
            i++;
            while (commAndline[i] == ' ' && i < length) {
                i++;
            }
            if (index < mAxArgc) {      //////////////处理最大参数个数 只取前mAxArgc个参数
                Argv[index] = commAndline + i;
                index ++;
            }                 ////////////////////////
            if (commAndline[i] == '"') { /////////////////////这段是对"的处理
                commAndline[i] = '\0';
                Argv[index - 1] = (char*)Argv[index - 1] + 1;
                while (commAndline[i] != '"' && i < length) {
                    i++;
                }
                if (commAndline[i] == '"') {
                    commAndline[i] = '\0';
                    i--;
                }
            }//if                                //////////////
        }

    }
    *Argc = index;
}

void sim800_echo ()
{
    // By default, the last intialized port is listening.
    // when you want to listen on a port, explicitly select it:
    serialSIM800.listen();
    delay(500);

    //Read SIM800 output (if available) and print it in Arduino IDE Serial Monitor
    while (serialSIM800.available())
    {
        Serial.write(serialSIM800.read());
    }
}
int sendmessage(char* phonenumber, char* text)
{
    serialSIM800.println("AT+CMGF=1"); // Configuring TEXT mode
    sim800_echo();
    
    // phone number
    serialSIM800.print("AT+CMGS=\"");
    serialSIM800.print(phonenumber);
    serialSIM800.println("\"");
    sim800_echo();
    
    serialSIM800.print(text); //text content
    sim800_echo();
    
    serialSIM800.write(26);
    sim800_echo();

    return 0;
}

#define MAX_COMMAND_LENGTH  128

void loop()
{
    String incoming = "";
    char command[MAX_COMMAND_LENGTH + 1] = {0};
    int argc = 0;
    char* argv[9] = {0};

    String simRead;
    char simBuffer[MAX_COMMAND_LENGTH] = {0};


    digitalWrite(PIN_LED, HIGH);
    delay(500);

    // By default, the last intialized port is listening.
    // when you want to listen on a port, explicitly select it:
    serialSIM800.listen();

    //Read SIM800 output (if available) and print it in Arduino IDE Serial Monitor
    while (serialSIM800.available())
    {
        //Serial.write(serialSIM800.read());
        simRead = serialSIM800.readString();
        simRead.toCharArray(simBuffer, MAX_COMMAND_LENGTH);
        Serial.write(simBuffer);
        if (strstr(simBuffer, "fat"))
        {
            // find the command
            Serial.println("Keyword: fat");
            writemem(0x20002d0b, 0x00050800);
            writemem(0x4005e3fc, 0x9f);
        }
    }

    digitalWrite(PIN_LED, LOW);
    delay(500);

    //Read Arduino IDE Serial Monitor inputs (if available) and send them to SIM800
    if (Serial.available())
    {
        // read the incoming:
        incoming = Serial.readString();
        if (incoming.length() > MAX_COMMAND_LENGTH)
        {
            Serial.println(" > invalid command.");
            return;
        }

        incoming.toCharArray(command, MAX_COMMAND_LENGTH);
        // say what you got:
        Serial.print(incoming);

        if (0 == strncmp(command, "AT", 2))
        {
            // AT command, passthrough
            serialSIM800.write(command);
            return;
        }

        if ('\n' == command[strlen(command) - 1]) command[strlen(command) - 1] = 0;
        GetArg(command, &argc, argv, 9);
        if (0 == stricmp(argv[0], "sendmessage"))
        {
            if (3 != argc)
            {
                Serial.println(" > invalid parameters.  e.g. sendmessage <phonenumber> <text>");
            }
            else
            {
                sendmessage(argv[1], argv[2]);
            }
        }
// jtag        
        else if (0 == stricmp(argv[0], "getid"))
        {
            int nIDCode = 0;
            nIDCode = GetDAPID();
            Serial.print(" > DAP ID: ");
            Serial.println(nIDCode, HEX);
        }
        else if (0 == stricmp(argv[0], "getid2"))
        {
            int nIDCode = 0;
            nIDCode = GetDAPID2();
            Serial.print(" > DAP ID: ");
            Serial.println(nIDCode, HEX);
        }
        else if (0 == stricmp(argv[0], "get_ap0idr"))
        {
            get_ap0idr();
        }
        else if (0 == stricmp(argv[0], "dp_rdbuff"))
        {
            dp_rdbuff();
        }
        else if (0 == stricmp(argv[0], "dp_dpidr"))
        {
            dp_dpidr();
        }
        else if (0 == stricmp(argv[0], "dp_ctrl"))
        {
            dp_ctrl();
        }
        else if (0 == stricmp(argv[0], "readmem"))
        {
            if (2 != argc)
            {
                Serial.println(" > invalid parameters.  e.g. readmem <addr>");
            }
            else
            {
                readmem(strtoul(argv[1], NULL, 0));
            }
        }
        else if (0 == stricmp(argv[0], "writemem"))
        {
            if (3 != argc)
            {
                Serial.println(" > invalid parameters.  e.g. writemem <addr> <value>");
            }
            else
            {
                writemem(strtoul(argv[1], NULL, 0), strtoul(argv[2], NULL, 0));
            }
        }
// jtag
        else
        {
            //invalid command
            Serial.println(" > invalid command.");
            incoming = "";
        }
// jtag
        Serial.print("#");
        Serial.flush();
// jtag
    }

}
