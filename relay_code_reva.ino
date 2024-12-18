/*

  Code for the relay device

*/

#define local_device_ID 9             // set to a number between 0 and 15 (inclusive)
#define paired_device_ID 10           // set to other device's ID

#define rssi_for_high_dBm -70         // set to a dBm power recieve level (depends on local noise floor)
#define rssi_voltage_converted ((rssi_for_high_dBm + 137.5) / 62.5) * (1024 / 5)

#define baud_rate 5                  // baud rate (in ms)

#define relay_ctrl_pin 6
#define trm433_pdn_pin 7
#define trm433_t_r_pin 4
#define trm433_data_pin 2
#define trm433_rssi_pin A0
#define ant_sw_ctrl_line 5
#define ladj_u_d_pin 8
#define ladj_cs_pin 9

void setup() {

  // begin serial monitor 
  Serial.begin(9600);

  // pin initializations
  pinMode(relay_ctrl_pin, OUTPUT);       // motor control relay pin

  pinMode(trm433_pdn_pin, OUTPUT);       // 433mhz power down pin
  digitalWrite(trm433_pdn_pin, LOW);     // device in sleep when pin is low

  pinMode(trm433_t_r_pin, OUTPUT);       // 433mhz transmit/recieve select pin
  digitalWrite(trm433_t_r_pin, LOW);     // set to recieve initially

  pinMode(trm433_data_pin, INPUT);       // 433mhz data line (initally high Z)

  pinMode(trm433_rssi_pin, INPUT);       // 433mhz RSSI line

}

void loop() {
  
  float rssi_of_other_device_voltage = 0;

  Serial.println("Status: checking for rts");
  while(rssi_of_other_device_voltage == 0) {
    rssi_of_other_device_voltage = checkForRTS();
  }
  Serial.print("Status: rts was sent, rssi: ");
  Serial.println(62.5 * ( rssi_of_other_device_voltage / (1024 / 5) ) - 137.5);

  sendCTS();
  Serial.println("Status: cts sent");

  delay(baud_rate / 2);

  // for (int i = 0; i < 20; i++) {

  //   Serial.println(readData());
  //   delay(baud_rate);

  // }

  int data = captureData();
  Serial.print("Status: data recieved: ");
  Serial.println(data);

  if (data != -1) {
    sendACK(rssi_of_other_device_voltage);
    Serial.println("Status: ack sent");
  }

  Serial.println("Status: relay updated");
  updateRelay(data);

}

void waitForHIGH() {

  while(1) {
    float rssi_value = analogRead(trm433_rssi_pin);
    if (rssi_value >= rssi_voltage_converted) { return; }
  }

}

int checkForHIGH() {

  float rssi_value = analogRead(trm433_rssi_pin);
  if (rssi_value >= rssi_voltage_converted) { return 1; } else { return 0; }

}

void waitForLOW() {

  while(1) {
    float rssi_value = analogRead(trm433_rssi_pin);
    if (rssi_value < rssi_voltage_converted) { return; }
  }

}

int checkForLOW() {

  float rssi_value = analogRead(trm433_rssi_pin);
  if (rssi_value < rssi_voltage_converted) { return 1; } else { return 0; }

}

int readData() {

  float rssi_value = analogRead(trm433_rssi_pin);
  if (rssi_value < rssi_voltage_converted) { return 0; } else { return 1; }

}

float checkForRTS() {

  // return 0 if no RTS
  // return RSSI (in voltage) if RTS

  // RTS (request to send) is 1101xxxx1011yy
  // xxxx is device ID of paired device
  // yy is 2 baud rate delay

  float rssi_sum = 0;

  digitalWrite(trm433_pdn_pin, HIGH);     // wake up rf module
  digitalWrite(trm433_t_r_pin, LOW);      // set to recieve
  pinMode(trm433_data_pin, INPUT);

  int binval0 = paired_device_ID / 8;     // convert int device ID to binary ID
  int binval1 = (paired_device_ID % 8) / 4;
  int binval2 = (paired_device_ID % 4) / 2;
  int binval3 = paired_device_ID % 2;

  waitForHIGH();                          // 1
  Serial.print("1");
  rssi_sum = rssi_sum + getRSSI();
  delay(baud_rate * 1.5);     // delay for 1.5 baud to ensure data reads in middle of bit period
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1");
  rssi_sum = rssi_sum + getRSSI();
  delay(baud_rate);
  if (checkForLOW() == 0) { return 0; }       // 0
  Serial.print("0");
  delay(baud_rate);
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1");
  rssi_sum = rssi_sum + getRSSI();
  delay(baud_rate);

  if (binval0) {                          // 0th ID bit
    if (checkForHIGH() == 0) { return 0; }
  } else {
    if (checkForLOW() == 0) { return 0; }
  }
  Serial.print("x");
  delay(baud_rate);
  if (binval1) {                          // 1st ID bit
    if (checkForHIGH() == 0) { return 0; }
  } else {
    if (checkForLOW() == 0) { return 0; }
  }
  Serial.print("x");
  delay(baud_rate);
  if (binval2) {                          // 2nd ID bit
    if (checkForHIGH() == 0) { return 0; }
  } else {
    if (checkForLOW() == 0) { return 0; }
  }
  Serial.print("x");
  delay(baud_rate);
  if (binval3) {                          // 3rd ID bit
    if (checkForHIGH() == 0) { return 0; }
  } else {
    if (checkForLOW() == 0) { return 0; }
  }
  Serial.print("x");
  delay(baud_rate);
  
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1");
  rssi_sum = rssi_sum + getRSSI();
  delay(baud_rate);
  if (checkForLOW() == 0) { return 0; }       // 0
  Serial.print("0");
  delay(baud_rate);
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1");
  rssi_sum = rssi_sum + getRSSI();
  delay(baud_rate);
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1 ");
  rssi_sum = rssi_sum + getRSSI();

  delay(baud_rate * 2);
  delay(baud_rate / 2);          // timing back on start of bit

  return rssi_sum / 6;

}

void send(int bit) {

  // only call when rf module is on, in transmit mode, and data pin is output

  if (bit) {
    digitalWrite(trm433_data_pin, HIGH);
  } else {
    digitalWrite(trm433_data_pin, LOW);
  }
  delay(baud_rate);
  return;

}

void waitForHIGH10baud() {

  for (int i = 0; i < 100; i ++) {
    float rssi_value = analogRead(trm433_rssi_pin);
    if (rssi_value >= rssi_voltage_converted) { return; }
    delay(baud_rate / 10);
  }

}

void sendCTS() {

  // CTS (clear to send) is 1011xxxx1101yy
  // xxxx is device ID of this device
  // yy is 2 baud rate delay

  digitalWrite(trm433_pdn_pin, HIGH);     // make sure rf is on
  digitalWrite(trm433_t_r_pin, HIGH);     // set to transmit mode
  pinMode(trm433_data_pin, OUTPUT);       // data pin is output

  int binval0 = local_device_ID / 8;     // convert int device ID to binary ID
  int binval1 = (local_device_ID % 8) / 4;
  int binval2 = (local_device_ID % 4) / 2;
  int binval3 = local_device_ID % 2;

  send(1);
  Serial.print("1");
  send(0);
  Serial.print("0");
  send(1);
  Serial.print("1");
  send(1);
  Serial.print("1");
  send(binval0);
  Serial.print("x");
  send(binval1);
  Serial.print("x");
  send(binval2);
  Serial.print("x");
  send(binval3);
  Serial.print("x");
  send(1);
  Serial.print("1");
  send(1);
  Serial.print("1");
  send(0);
  Serial.print("0");
  send(1);
  Serial.print("1 ");

}

int captureData() {

  // data is sent as 101xxxx101
  // x is the data to be sent (sent 4 bit periods)

  digitalWrite(trm433_pdn_pin, HIGH);     // make sure rf module is awake
  digitalWrite(trm433_t_r_pin, LOW);      // set to recieve
  pinMode(trm433_data_pin, INPUT);        // data pin is input

  waitForHIGH10baud();
  Serial.print("1");
  delay(baud_rate / 2);     // delay for 0.5 baud to ensure data reads in middle of bit period
  delay(baud_rate);
  if (checkForLOW() == 0) { return -1; }       // 0
  Serial.print("0");
  delay(baud_rate);
  if (checkForHIGH() == 0) { return -1; }      // 1
  Serial.print("1");
  delay(baud_rate);

  int data = -1;

  if (readData()) {             // check data value and set variable
    data = 1;
  } else {
    data = 0;
  }
  Serial.print("d");
  delay(baud_rate);             // if value changes over next 3 cycles, return failed
  if (readData() != data) { return -1; }
  Serial.print("d");
  delay(baud_rate);
  if (readData() != data) { return -1; }
  Serial.print("d");
  delay(baud_rate);
  if (readData() != data) { return -1; }
  Serial.print("d");
  delay(baud_rate);

  if (checkForHIGH() == 0) { return -1; }      // 1
  Serial.print("1");
  delay(baud_rate);
  if (checkForLOW() == 0) { return -1; }       // 0
  Serial.print("0");
  delay(baud_rate);
  if (checkForHIGH() == 0) { return -1; }      // 1
  Serial.print("1 ");
  
  delay(baud_rate * 2);
  delay(baud_rate / 2);         // back on the start of the bit period

  return data;

}

void sendACK(float rssi_of_paired_v) {

  // ACK (acknowledge) is 101xxxx
  // xxxx is quantized rssi of paired device

  digitalWrite(trm433_pdn_pin, HIGH);     // make sure rf is on
  digitalWrite(trm433_t_r_pin, HIGH);     // set to transmit mode
  pinMode(trm433_data_pin, OUTPUT);       // data pin is output

  int quant_rssi = quantizeRSSI(rssi_of_paired_v);

  int binval0 = quant_rssi / 8;     // convert int to binary
  int binval1 = (quant_rssi % 8) / 4;
  int binval2 = (quant_rssi % 4) / 2;
  int binval3 = quant_rssi % 2;

  send(1);
  Serial.print("1");
  send(0);
  Serial.print("0");
  send(1);
  Serial.print("1");
  send(binval0);
  Serial.print(binval0);
  send(binval1);
  Serial.print(binval1);
  send(binval2);
  Serial.print(binval2);
  send(binval3);
  Serial.print(binval3);

}

void updateRelay(int relayStatus) {

  if (relayStatus == 0) {
    digitalWrite(relay_ctrl_pin, LOW);
    delay(100);     // delay to protect device controlled by relay
  } else if (relayStatus == 1) {
    digitalWrite(relay_ctrl_pin, HIGH);
    delay(100);     // delay to protect device controlled by relay
  } else {
    return;
  }

}

float getRSSI() {

  return analogRead(trm433_rssi_pin);

}

int quantizeRSSI(float rssi_voltage) {

  int quantRSSI;

  float rssi_actual = 62.5 * ( rssi_voltage / (1024 / 5) ) - 137.5;

  if (rssi_actual < -90) { quantRSSI = 0; }
  else if (rssi_actual >= -90 && rssi_actual < -85) { quantRSSI = 1; }
  else if (rssi_actual >= -85 && rssi_actual < -80) { quantRSSI = 2; }
  else if (rssi_actual >= -80 && rssi_actual < -75) { quantRSSI = 3; }
  else if (rssi_actual >= -75 && rssi_actual < -70) { quantRSSI = 4; }
  else if (rssi_actual >= -70 && rssi_actual < -65) { quantRSSI = 5; }
  else if (rssi_actual >= -65 && rssi_actual < -60) { quantRSSI = 6; }
  else if (rssi_actual >= -60 && rssi_actual < -55) { quantRSSI = 7; }
  else if (rssi_actual >= -55 && rssi_actual < -50) { quantRSSI = 8; }
  else if (rssi_actual >= -50 && rssi_actual < -45) { quantRSSI = 9; }
  else if (rssi_actual >= -45 && rssi_actual < -40) { quantRSSI = 10; }
  else if (rssi_actual >= -40 && rssi_actual < -35) { quantRSSI = 11; }
  else if (rssi_actual >= -35 && rssi_actual < -30) { quantRSSI = 12; }
  else if (rssi_actual >= -30 && rssi_actual < -25) { quantRSSI = 13; }
  else if (rssi_actual >= -25 && rssi_actual < -20) { quantRSSI = 14; }
  else if (rssi_actual >= -20) { quantRSSI = 15; }

  return quantRSSI;

}