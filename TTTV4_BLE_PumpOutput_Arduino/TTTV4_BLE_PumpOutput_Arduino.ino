#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>
#include <M5StickCPlus2.h>


#define SERVICE_UUID        "00002220-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID "00002221-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_RX  "00002222-0000-1000-8000-00805F9B34FB"

String BLEConnectName = "BLACK3";

BLEServer * pServer;
BLECharacteristic * pCharacteristic;
BLEAdvertising * pAdvertising;
BLE2901 *descriptor_2901 = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
int a;

//ポンプの数を定義
const int NUM_PUMPS = 5;
//それぞれのポンプ番号を定義
const int SALTY_PUMP = 0;
const int SWEET_PUMP = 25;
const int SOUR_PUMP = 26;
const int UMAMI_PUMP = 32;
const int BITTER_PUMP = 33;
//ポンプの番号を格納した配列を定義 0=塩味 1=甘味 2=酸味 3=旨味 4=苦味　（この順でデータが送られてくるので，データの0番の駆動時間はPUMP_PINS[0]に対応する）
const int PUMP_PINS[NUM_PUMPS] = {SALTY_PUMP, SWEET_PUMP, SOUR_PUMP, UMAMI_PUMP, BITTER_PUMP};



//サーバーのコールバック関数
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};


//Stringをカンマ区切りで分割する関数
int splitString(String data, char delimiter, String *dst){
  int index = 0;
  int arraySize = (sizeof(data))/sizeof((data[0]));
  int datalength = data.length();

  for(int i=0; i<datalength; i++){
    char tmp = data.charAt(i);
    if(tmp == delimiter){
      index++;
      if(index>(arraySize-1)) return -1;
    }
    else
    {
      dst[index] += tmp;
    }
  }
  return(index + 1);
}

//writeされたときのコールバック関数
class MyCallbacks: public BLECharacteristicCallbacks {
  //Writeを受け取る処理
  void onWrite(BLECharacteristic *pCharacteristic){
    //値を受け取って表示
    String receivedData = pCharacteristic->getValue();
    // M5.Lcd.println(receivedData);

    //最初の接続時の処理
    if(receivedData.equals("Bluetooth Connected")){
      
    }
    //接続以外の受信時の処理
    else{
      //カンマ区切りを分割して，Int型の配列としてポンプ番号と駆動時間で格納
      receivedData.trim();
      String splittedData[NUM_PUMPS] = {};
      int index = splitString(receivedData, ',', splittedData);
      int splittedData_int[NUM_PUMPS][2] = {};
      for(int i=0; i<index; i++){
        splittedData_int[i][0] = i;
        splittedData_int[i][1] = splittedData[i].toInt();
      }
      
      //駆動時間の値を昇順にソート
      int pumpDriveTime_Array[NUM_PUMPS] = {};
      int pumpOrder_Array[NUM_PUMPS] = {};
      for(int i=0; i<NUM_PUMPS; i++){
        int maxPumpNum; //最大ポンプ駆動時間が必要なポンプ番号を格納する変数
        int maxPumpDriveTime = -1; //最大ポンプ駆動時間を格納する変数
        int maxID; //配列のIDを格納する変数
        //最大値を探索
        for(int j=0; j<NUM_PUMPS; j++){
          if(splittedData_int[j][1]>=maxPumpDriveTime){
            maxPumpNum = splittedData_int[j][0];
            maxPumpDriveTime = splittedData_int[j][1];
            maxID = j;
          }
        }
        //最大値とそのポンプ番号を格納して削除
        pumpDriveTime_Array[NUM_PUMPS-1-i] = maxPumpDriveTime;
        pumpOrder_Array[NUM_PUMPS-1-i] = maxPumpNum;
        splittedData_int[maxID][1] = -1; 
      }

      //各ポンプの停止までのdelayを計算
      int delay_Array[NUM_PUMPS] = {};
      delay_Array[0] = pumpDriveTime_Array[0];
      for(int i=1; i<NUM_PUMPS; i++){
        delay_Array[i] = pumpDriveTime_Array[i] - pumpDriveTime_Array[i-1];
      }

      //全ポンプをONにして，delay_Arrayに従って順番に停止
      for(int i=0; i<NUM_PUMPS; i++){
        digitalWrite(PUMP_PINS[i],LOW);
      }
      for(int i=0; i<NUM_PUMPS; i++){
        delay(delay_Array[i]);
        digitalWrite(PUMP_PINS[pumpOrder_Array[i]],HIGH);
        // M5.Lcd.println("Pump" + String(PUMP_PINS[pumpOrder_Array[i]]) + " : " + String(delay_Array[i]) + " ms");
      }
    }
  }
};




void setup() {
  M5.begin();                 // 本体初期化
  M5.Lcd.begin();             // 画面初期化
  M5.Lcd.setRotation(3);      // 画面向き設定（0～3で設定、4～7は反転)※初期値は1
  M5.Lcd.setTextWrap(true);   // 画面端での改行の有無（true:有り[初期値], false:無し）※print関数のみ有効
  uint16_t background_color=0;
  background_color = M5.Lcd.color565(0,0,0);
  M5.Lcd.fillScreen(background_color);   // 画面の背景色をそれぞれの色に
  uint16_t text_color=0;
  text_color = M5.Lcd.color565(255,255,255);
  M5.Lcd.setTextColor(text_color, background_color);
  M5.Lcd.setTextSize(7);
  M5.Lcd.setCursor(18, 45);
  M5.Lcd.println("TTTV4");

  //画面に識別番号を表示
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.print(BLEConnectName);

  //ピンモードを出力に設定
  pinMode(SALTY_PUMP, OUTPUT);
  pinMode(SWEET_PUMP, OUTPUT);
  pinMode(SOUR_PUMP, OUTPUT);
  pinMode(UMAMI_PUMP, OUTPUT);
  pinMode(BITTER_PUMP, OUTPUT);

  //ポンプを停止させるためにすべてのポンプをHIGHに設定
  digitalWrite(SALTY_PUMP, HIGH);
  digitalWrite(SWEET_PUMP, HIGH);
  digitalWrite(SOUR_PUMP, HIGH);
  digitalWrite(UMAMI_PUMP, HIGH);
  digitalWrite(BITTER_PUMP, HIGH);

  
  // Create the BLE Device
  BLEDevice::init(BLEConnectName);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE
  );
  
  // WriteされたときのコールバックをMyCallbacks（）に設定
  pCharacteristic->setCallbacks(new MyCallbacks());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  
  // Creates BLE Descriptor 0x2902: Client Characteristic Configuration Descriptor (CCCD)
  pCharacteristic->addDescriptor(new BLE2902());

  
  // Adds also the Characteristic User Description - 0x2901 descriptor
  //  descriptor_2901 = new BLE2901();
  //  descriptor_2901->setDescription("My own description for this characteristic.");
  //  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);  // enforce read only - default is Read|Write
  //  pCharacteristic->addDescriptor(descriptor_2901);

  // Start the service
  pService->start();

  // Start advertising
  pAdvertising = BLEDevice::getAdvertising();
//  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  
  Serial.println("Waiting a client connection to notify...");
}

void loop() {  
  // notify changed value 投げたい値をvalueに格納してnotify()するとBLEで送信できる
  if (deviceConnected) {
    value = M5.Power.getBatteryLevel();
    //valueをセットしてNotifyする
    pCharacteristic->setValue((uint8_t *)&value, 4);
    pCharacteristic->notify();
//    M5.Lcd.print(a);
    delay(500);
  }
  
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
