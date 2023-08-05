#include <usbhid.h>
#include <usbhub.h>
#include <hiduniversal.h>
#include <hidboot.h>
#include <SPI.h>
#define SEG 1000
#define TOTAL_TIME 480
#define MAX_CODES 100

String str_codigo;
int indice = 0, ultimo_chuveiro = 0;
int chuveiros[4] = {2,3,4,5};
int segundos[4] = {0,0,0,0};
double codigo = 0;
double lista_codigos[MAX_CODES];
unsigned long ultima_execucao = 0;
bool ativo = false;

class MyParser : public HIDReportParser {
  public:
    MyParser();
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
  protected:
    uint8_t KeyToAscii(bool upper, uint8_t mod, uint8_t key);
    virtual void OnKeyScanned(bool upper, uint8_t mod, uint8_t key);
    virtual void OnScanFinished();
};

MyParser::MyParser() {}

void MyParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  // If error or empty, return
  if (buf[2] == 1 || buf[2] == 0) return;

  for (uint8_t i = 7; i >= 2; i--) {
    // If empty, skip
    if (buf[i] == 0) continue;

    // If enter signal emitted, scan finished
    if (buf[i] == UHS_HID_BOOT_KEY_ENTER) {
      OnScanFinished();
    }

    // If not, continue normally
    else {
      // If bit position not in 2, it's uppercase words
      OnKeyScanned(i > 2, buf, buf[i]);
    }

    return;
  }
}

uint8_t MyParser::KeyToAscii(bool upper, uint8_t mod, uint8_t key) {
  // Letters
  if (VALUE_WITHIN(key, 0x04, 0x1d)) {
    if (upper) return (key - 4 + 'A');
    else return (key - 4 + 'a');
  }

  // Numbers
  else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
    return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
  }

  return 0;
}

void MyParser::OnKeyScanned(bool upper, uint8_t mod, uint8_t key) {
  uint8_t ascii = KeyToAscii(upper, mod, key);
  str_codigo.concat((char)ascii);
}

bool ativaChuveiro() {
  for(int i = 0; i <= 3; i ++ ) {
    // Serial.print("ultimo_chuveiro ");
    // Serial.println(ultimo_chuveiro);
    if(segundos[ultimo_chuveiro] == 0){
      segundos[ultimo_chuveiro] = TOTAL_TIME;
      ultimo_chuveiro ++;
      return(true);
    }
    ultimo_chuveiro ++;
    if (ultimo_chuveiro > 3){
      ultimo_chuveiro = 0;
    }
  }
  return(false);
}

void MyParser::OnScanFinished() {
  if(str_codigo.substring(0, 3) == "910"){
    // Serial.print("str_codigo ");
    // Serial.println(str_codigo);
    codigo = str_codigo.substring(5, 13).toDouble();
    // Serial.print("codigo ");
    // Serial.println(codigo);
    bool existe = false;
    for (int index = 0; index < MAX_CODES; index++){
      if(codigo == lista_codigos[index]){
        existe = true;
        break;
      }
    }
    if(!existe && ativaChuveiro()){
      lista_codigos[indice] = codigo;
      indice ++;
      if(indice >= MAX_CODES){
        indice = 0;
      }
    }
  }
  str_codigo = String("9");
}

USB          Usb;
USBHub       Hub(&Usb);
HIDUniversal Hid(&Usb);
MyParser     Parser;

void setup() {

  for(int porta = 2; porta <= 5; porta ++ ) {
    pinMode(porta, OUTPUT);
    digitalWrite(porta ,LOW);
  }
  
  // Serial.begin( 115200 );

  if (Usb.Init() == -1) {
    // Serial.println("1");
  }

  delay( 200 );

  Hid.SetReportParser(0, &Parser);
  str_codigo = String("9");
}

void loop() {
  if((millis() - ultima_execucao > SEG)){
    ultima_execucao = millis();
    
    for(int i = 0; i <= 3; i ++ ){
      if(segundos[i] == TOTAL_TIME || segundos[i] == ((TOTAL_TIME/2)-2) || segundos[i] == ((TOTAL_TIME/4)-2)){
        digitalWrite(chuveiros[i], HIGH);
        // Serial.print("Start ");
        // Serial.println(i);
        segundos[i] --;
      }
      else if(segundos[i] == (TOTAL_TIME/2) || segundos[i] == (TOTAL_TIME/4)){
        digitalWrite(chuveiros[i], LOW);
        segundos[i] --;
      }
      else if(segundos[i] == 0){
        digitalWrite(chuveiros[i], LOW);
      }
      else {
        segundos[i] --;
      }
    }
  }
  Usb.Task();
}
