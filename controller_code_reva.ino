/*

  Code for the controller device

*/

#define local_device_ID 10            // set to a number between 0 and 15 (inclusive)
#define paired_device_ID 9            // set to other device's ID

#define rssi_for_high_dBm -70         // set to a dBm power recieve level (depends on local noise floor)
#define rssi_voltage_converted ((rssi_for_high_dBm + 137.5) / 62.5) * (1024 / 5)

#define baud_rate 5                  // baud rate (in ms)

#define trm433_pdn_pin 7
#define trm433_t_r_pin 4
#define trm433_data_pin 2
#define trm433_rssi_pin A0
#define ant_sw_ctrl_line 5
#define ladj_u_d_pin 8
#define ladj_cs_pin 9
#define pushbutton_pin 3

int control_status = 0;

void setup() {

  // begin serial monitor 
  Serial.begin(9600);

  pinMode(trm433_pdn_pin, OUTPUT);       // 433mhz power down pin
  digitalWrite(trm433_pdn_pin, LOW);     // device in sleep when pin is low

  pinMode(trm433_t_r_pin, OUTPUT);       // 433mhz transmit/recieve select pin
  digitalWrite(trm433_t_r_pin, LOW);     // set to recieve initially

  pinMode(trm433_data_pin, INPUT);       // 433mhz data line (initally high Z)

  pinMode(trm433_rssi_pin, INPUT);       // 433mhz RSSI line

  pinMode(pushbutton_pin, INPUT);

}

void loop() {
  
  digitalWrite(trm433_pdn_pin, LOW);     // device in sleep when pin is low
  Serial.println("Status: waiting for button press");
  waitForPBpress();
  Serial.println("Status: button pressed!");
  Serial.print("Sending RTS --> ");
  if (control_status) {control_status = 0;} else {control_status = 1;}    // invert control_status
  sendRTS();
  Serial.println("Status: rts successfully sent!");
  Serial.print("Listening for CTS --> ");
  int paired_rssi = checkCTS();
  if (paired_rssi != 0) {    // successful CTS
    Serial.print("Status: successful cts! rssi: ");
    float actual_rssi = 62.5 * ( (float) paired_rssi / (1024 / 5) ) - 137.5;
    Serial.print(actual_rssi);
    Serial.println(" dBm");
    Serial.print("Sending data --> ");
    sendData(control_status);
    Serial.println("Status: data successfully sent!");
    Serial.print("Listening for ACK --> ");
    int my_rssi = getACK();
    if (my_rssi != 0) {
      Serial.println("Status: ack recieved!");
    }
    
    while (digitalRead(pushbutton_pin) == 0) { delay(5); }
    
  } else {
    Serial.println("Status: cts not recieved!");
  }
  

}

void waitForPBpress() {

  int pb_status = digitalRead(pushbutton_pin);

  while(pb_status == 1) {   // wait for 1 --> 0 transition

    pb_status = digitalRead(pushbutton_pin);

  }        
  
  return;

}

void waitForHIGH() {

  while(1) {
    float rssi_value = analogRead(trm433_rssi_pin);
    if (rssi_value >= rssi_voltage_converted) { return; }
  }

}

void waitForHIGH10baud() {

  for (int i = 0; i < 100; i ++) {
    float rssi_value = analogRead(trm433_rssi_pin);
    if (rssi_value >= rssi_voltage_converted) { return; }
    delay(baud_rate / 10);
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

int readData() {

  float rssi_value = analogRead(trm433_rssi_pin);
  if (rssi_value < rssi_voltage_converted) { return 0; } else { return 1; }

}

void sendRTS() {

  // RTS (request to send) is 1101xxxx1011yy
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
  send(1);
  Serial.print("1");
  send(0);
  Serial.print("0");
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
  send(0);
  Serial.print("0");
  send(1);
  Serial.print("1");
  send(1);
  Serial.print("1 ");

}

int checkCTS() {

  // return 0 if no CTS
  // return RSSI (in voltage) if CTS

  // CTS (clear to send) is 1011xxxx1101yy
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

  waitForHIGH10baud();
  Serial.print("1");
  delay(baud_rate / 2);
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
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1");
  rssi_sum = rssi_sum + getRSSI();
  delay(baud_rate);
  if (checkForLOW() == 0) { return 0; }       // 0
  Serial.print("0");
  delay(baud_rate);
  if (checkForHIGH() == 0) { return 0; }      // 1
  Serial.print("1 ");
  rssi_sum = rssi_sum + getRSSI();

  delay(baud_rate);
  delay(baud_rate);
  delay(baud_rate / 2);          // timing back on start of bit

  return rssi_sum / 6;

}

void sendData(int data) {

  // data is sent as 101xxxx101
  // x is the data to be sent (sent 4 bit periods)

  digitalWrite(trm433_pdn_pin, HIGH);     // make sure rf is on
  digitalWrite(trm433_t_r_pin, HIGH);     // set to transmit mode
  pinMode(trm433_data_pin, OUTPUT);       // data pin is output

  send(1);
  Serial.print("1");
  send(0);
  Serial.print("0");
  send(1);
  Serial.print("1");
  send(data);
  Serial.print("d");
  send(data);
  Serial.print("d");
  send(data);
  Serial.print("d");
  send(data);
  Serial.print("d");
  send(1);
  Serial.print("1");
  send(0);
  Serial.print("0");
  send(1);
  Serial.print("1 ");
  delay(baud_rate * 2.5);       // back on middle of bit

}

int getACK() {

  // ACK (acknowledge) is 101xxxx
  // xxxx is quantized rssi of paired device

  digitalWrite(trm433_pdn_pin, HIGH);     // wake up rf module
  digitalWrite(trm433_t_r_pin, LOW);      // set to recieve
  pinMode(trm433_data_pin, INPUT);

  waitForHIGH10baud();
  Serial.print("1");
  delay(baud_rate);
  if (checkForLOW() == 0) { return 0; }        // 0
  Serial.print("0");
  delay(baud_rate);
  if (checkForHIGH() == 0) { return 0; }       // 1
  Serial.print("1");
  delay(baud_rate);

  int bit0 = readData();
  Serial.print(bit0);
  delay(baud_rate);
  int bit1 = readData();
  Serial.print(bit1);
  delay(baud_rate);
  int bit2 = readData();
  Serial.print(bit2);
  delay(baud_rate);
  int bit3 = readData();
  Serial.print(bit3);
  Serial.print(" ");
  delay(baud_rate);

  int rssi = 8 * bit0 + 4 * bit1 + 2 * bit2 + bit3;

  return rssi;

}

float getRSSI() {

  return analogRead(trm433_rssi_pin);

}

int quantizeRSSI(float rssi_voltage) {

  int quantRSSI;

  float rssi_actual = (65 * rssi_voltage - 139.25) / 204.6;

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