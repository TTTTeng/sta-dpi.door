#include "rfid.h"
#include <Wire.h>
#include "PN532_HSU.h"
#include "PN532.h"
#define led 13
#define sound 5
#define door 4
#define sw 3
#define ewl 2
RFID rfid;
String studentid;
int open_time = 0;
#define switch_open_time 10
#define remote_open_time 20

static PN532_HSU pn532_hsu(Serial3);
static PN532 pn532(pn532_hsu);

uint8_t RFID::skeletonKey[8] = {0x64,0x1C,0x6D,0xDB};
uint8_t RFID::skeletonKeyLength = 4;

void right()
{
  tone(sound,1000);
  delay(400);
  tone(sound,800,750);
  digitalWrite(sound,LOW);
}
void special()
{
  tone(sound,750);
  delay(400);
  tone(sound,1000);
  delay(400);
  tone(sound,1250,600);
  digitalWrite(sound,LOW);
}
void special2()
{
  tone(sound,1250);
  delay(400);
  tone(sound,1000);
  delay(400);
  tone(sound,750,600);
  digitalWrite(sound,LOW);
}
void wrong()
{
  tone(sound,700,300);
  delay(300);
  digitalWrite(sound,LOW);
  delay(300);
  tone(sound,700,300);
  delay(300);
  digitalWrite(sound,LOW);
  delay(300);
  tone(sound,700,300);
  digitalWrite(sound,LOW);
}
void door_open()
{
  for (int i=0;i<20;i++)
  {
    if (digitalRead(sw)==HIGH) return;
    delay(1);
  }
  digitalWrite(door,LOW);
  open_time = switch_open_time;
}
void door_open_remote()
{
  for (int i=0;i<20;i++)
  {
    if (digitalRead(ewl)==HIGH) return;
    delay(1);
  }
  digitalWrite(door,LOW);
  open_time = remote_open_time;
}
void RFID::Poll()
{
  boolean success;

  if(Found()){ //A card had been found previously
    return;
  }
  
  unsigned long currentMillis = millis();
  if(currentMillis-previousPollMillis>10){
    previousPollMillis = currentMillis;
  }else{
    return;
  }

  if(card == Card_14443B){
    if(pn532.stuCardIsPresent()){ //Card doesn't leave yet
      return;
    }else{
      pn532.resetConfigFor14443B();
      card = Card_None;
      digitalWrite(led,LOW);
    }
  }

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = pn532.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 200);
  if(card == Card_14443A){
    if(success){
      //Card doesn't leave yet since last detected
      pn532.inRelease(0); //Release all cards
      return;
    }else{
      card = Card_None;
      digitalWrite(led,LOW);
    }
  }

  if (success) {
    Serial.println("Found Mifare card!");
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) 
    {
      Serial.print(" 0x");Serial.print(uid[i], HEX); 
    }
    Serial.println("");
    pn532.inRelease(0); //Release all cards
    card = Card_14443A;
    found = true;
    digitalWrite(led,HIGH);
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
    delay(10);
  }

  static uint8_t AFI[] = {0};
  success = pn532.inListPassiveTarget(PN532_106KBPS_ISO14443B, sizeof(AFI) , AFI, 20);
  if (success) {
    uint8_t cardId[3]; 
    uint8_t expire[3]; 
    char studentId[11];

    pn532.inRelease();
    success = pn532.readTsighuaStuCard(cardId, expire, studentId);
    if(success){
      Serial.println("Found student card!");
      Serial.print("Student Number: "); Serial.println(studentId);
      studentid=studentId;
      for(int i=0; i<5; i++){
        uid[i] = studentId[i+5];
      }
      for(int i=0; i<3; i++){
        uid[i+5] = cardId[i];
      }
      uidLength = 8;
      card = Card_14443B;
      found = true;
      digitalWrite(led,HIGH);
    }
    else{
      pn532.resetConfigFor14443B();
      Serial.println("reset");
      delay(1);
    }
  }
  else{
  Serial.println("Timed out waiting for a studentcard");
  delay(10);
  }
}

void RFID::Init()
{
  pn532.begin();

  uint32_t versiondata = pn532.getFirmwareVersion();
  if (! versiondata) {
    Serial.println("Didn't find PN53x board");
    return;
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.print((versiondata>>24) & 0xFF, HEX); 
  Serial.print(" with firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  pn532.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  pn532.SAMConfig();
}

void setup()
{
  pinMode(led,OUTPUT);
  pinMode(sound,OUTPUT);
  pinMode(sw,INPUT_PULLUP);
  pinMode(ewl,INPUT_PULLUP);
  pinMode(door,OUTPUT);
  digitalWrite(door,HIGH);
  Serial.begin(9600);
  Serial3.begin(115200);
  Serial2.begin(115200);
  rfid.Init(); 
  attachInterrupt(0, door_open_remote, FALLING);
  attachInterrupt(1, door_open, FALLING);
}
void loop()
{
  if (Serial.read() == 'r')
  {
    digitalWrite(door,LOW);
    open_time = remote_open_time; 
  }
  rfid.Poll();
  if(rfid.Found()){
    if(
    //诸位大哥和元老们
    studentid=="2016010564"    //王广晗
    or studentid=="2020210998" //王广晗
    or studentid=="2018011562" //范雨晗
    or studentid=="2019310437" //刘心志
    or studentid=="2019310438" //丁荣
    or studentid=="2017310381" //欧阳晨光
    or studentid=="2016010766" //林逸晗
    or studentid=="2020310429" //林逸晗
    or studentid=="2016010819" //王逸群
 
	  //老师
	  or studentid=="1988990346" //李老师
    or studentid=="1988990346" //李老师
    or studentid=="2017640415" //胡老师
    or studentid=="2013990111" //张浩老师
    or studentid=="2014990017" //罗川老师
    or studentid=="2001990208" //李玉和老师
    or studentid=="2020620975" //徐东老师
    or studentid=="2006980114" //王芃老师
    or studentid=="1980990115" //侯汝舜老师
  
	  //主席团
    or studentid=="2018010635" //李豪汉
	  or studentid=="2018010625" //张翀
	  or studentid=="2018010647" //郑重
    or studentid=="2018010634" //姜凯元
    or studentid=="2018010653" //涂汉璋
    or studentid=="2018010579" //周卓润
    or studentid=="2017010820" //滕峻池
    or studentid=="2017010644" //阎华毅
    or studentid=="2017010687" //刘悦阳

    //其他用户
    or studentid=="2018011622" //肖铂
    or studentid=="2017010484" //王陈梓
    or studentid=="2017010661" //王绍航
    or studentid=="2017010875" //孔瑞楷
    or studentid=="2020310415" //陈胜杰
    or studentid=="2020012940" //陈伟亮
    or studentid=="2019010586" //邓璟瑗
    or studentid=="2019010600" //丁思宇
    or studentid=="2019010579" //杜子煜
    or studentid=="2018010652" //段宇光
    or studentid=="2020010475" //谷绍伟
    or studentid=="2019010604" //郝海清
    or studentid=="2017010643" //贺梦凡
    or studentid=="2020310428" //胡发德
    or studentid=="2016010658" //胡克勤
    or studentid=="2017010678" //黄骏
    or studentid=="2020010384" //解维嘉
    or studentid=="2019010614" //李冰白
    or studentid=="2018310434" //李福祥
    or studentid=="2018010623" //李金峰
    or studentid=="2019010588" //李子康
    or studentid=="2020012947" //练邱爽
    or studentid=="2020012960" //梁与同
    or studentid=="2019010560" //廖子健
    or studentid=="2019010567" //林巧妹
    or studentid=="2019010561" //刘庚
    or studentid=="2020012950" //刘柠赫
    or studentid=="2017010686" //刘宇航
    or studentid=="2019010581" //卢祎迪
    or studentid=="2017010641" //马钺洋
    or studentid=="2019010565" //梅子麒
    or studentid=="2019010560" //穆琳聪
    or studentid=="2019010575" //潘首安
    or studentid=="2017010519" //蒲子航
    or studentid=="2019010766" //钱彦
    or studentid=="2019010764" //邱士乘
    or studentid=="2020012958" //沈珂宇
    or studentid=="2019010557" //时其然
    or studentid=="2020012949" //宋东芪
    or studentid=="2017011674" //宋九如
    or studentid=="2018010648" //陶海旭
    or studentid=="2019010607" //王略
    or studentid=="2020010387" //王晓霆
    or studentid=="2020310418" //王雪霖
    or studentid=="2017310402" //王宇晨
    or studentid=="2018010693" //谢荣博
    or studentid=="2020012955" //辛约
    or studentid=="2019010552" //邢海潼
    or studentid=="2017010679" //闫佳文
    or studentid=="2020012941" //颜丙禄
    or studentid=="2019010572" //杨瑞
    or studentid=="2019011520" //杨学舸
    or studentid=="2019010556" //袁泳
    or studentid=="2019010573" //苑蒙
    or studentid=="2020010379" //翟光弘
    or studentid=="2019010606" //张博涵
    or studentid=="2018010646" //张浩哲
    or studentid=="2020012952" //张茗堃
    or studentid=="2019010566" //张玥
    or studentid=="2019010578" //张越
    or studentid=="2017010675" //赵天罡
    or studentid=="2019010559" //郑雨珂
    or studentid=="2019010574" //周恩泽
    or studentid=="2019010576" //周鹏宇
    )
    {
      Serial.print("door open successful");
      Serial2.print("1 ");
      Serial2.println(studentid);
      if (studentid=="2018011562") special(); //wgh的女朋友
      else if (studentid=="2018011622") special2(); //tjc的女朋友
      else right();
      digitalWrite(door,LOW);
      delay(3000);
      digitalWrite(door,HIGH);
    }
    else
    {
      uint8_t uidLen;
      uint8_t* uid = rfid.GetUid(uidLen);
      if (studentid!="")
      {
		Serial2.print("0 ");
		Serial2.println(studentid);
      }
      wrong();
    }
    rfid.Next();
  }
  if (open_time>0) open_time--;
  else digitalWrite(door,HIGH); 
  studentid="";
}
