#include <MQ135.h>

#define BLINKER_WIFI

#include <Blinker.h>
#include <DHT.h>

char auth[] = "8eae08d786f5";    //blinker密钥
char ssid[] = "Skadi";   //WiFi名称
char pswd[] = "64683552";   //WiFi密码

BlinkerNumber HUMI("humi");
BlinkerNumber TEMP("temp");
BlinkerNumber GAS("gas");
BlinkerNumber FIRE("fire");
BlinkerButton Button1("btn-fire");
BlinkerNumber Number1("num-fire");


#define DHTPIN 4  //温湿度传感器
#define MQPIN  A0  //空气质量传感器
#define FIREPIN 5  //火焰传感器
#define BUZZERPIN 12 //蜂鸣器

#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);
MQ135 gasSensor = MQ135(MQPIN);

float humi_read = 0, temp_read = 0;
float gas_read = 0;
int   fire_read = 0;
bool  counter = 1;
float recallgas1 = 0, recallgas2 = 0, recallgas3 = 0, recallgas4 = 0, avegas = 0; //用于增加烟雾传感器精度，储存前值

unsigned long pMillis = 0;//保存上次推送消息的时间，初始值为0

void heartbeat()  //心跳包发送
{
    HUMI.print(humi_read);
    TEMP.print(temp_read);
    GAS.print(gas_read);
    FIRE.print(fire_read);
}

void dataStorage()   //数据回溯
{
    Blinker.dataStorage("temp", temp_read);
    Blinker.dataStorage("humi", humi_read);
    Blinker.dataStorage("gas", gas_read);
}

void setup()
{
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);
    BLINKER_DEBUG.debugAll();
    pinMode(BUZZERPIN, OUTPUT);
    pinMode(FIREPIN,INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Blinker.begin(auth, ssid, pswd);
    Blinker.attachHeartbeat(heartbeat);
    Blinker.attachDataStorage(dataStorage);
    dht.begin();
}

float GetVoltage()  //获取A0管脚的电压值，并返回float型数据
{
  float a = analogRead(A0);
  float b = a/204.8;  
  return b;
}

float GetPPM()  //将读取到的电压值转化为气体浓度，并返回float型数据
{
  float a = GetVoltage();
  float b = pow(11.5428 * 35.904 * a/(25.5-5.1* a),0.6549);  
  return b;
}

void delaypush(unsigned long time1, unsigned long time2)
{
  if(time1 - time2 >= 10000)
    Blinker.notify("Gas Alarm!!!");
}

void loop()
{
    Blinker.run();

    unsigned long cMillis = millis();
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float a = GetPPM();
    int   f = digitalRead(FIREPIN);
    

    avegas = (a + recallgas1 + recallgas2 + recallgas3 + recallgas4)/5; 
    recallgas4 = recallgas3;
    recallgas3 = recallgas2;
    recallgas2 = recallgas1; //取气体读数五次的均值
    recallgas1 = a;
    

    if (isnan(h) || isnan(t) || isnan(a) || isnan(f)) //检测读数是否为空
    {
        BLINKER_LOG("Failed to read from sensor!");  //为空则报错
    }
    else
    {
        BLINKER_LOG("Humidity: ", h, " %");
        BLINKER_LOG("Temperature: ", t, " *C");
        BLINKER_LOG("GAS: ", a, " ppm");
        BLINKER_LOG("AVEGAS: ", avegas, " ppm");
        
        if(f == LOW){
        BLINKER_LOG("FIRE: ",  "危险");
        Blinker.notify("Fire Alarm!!!");
        }
          
        else if(f == HIGH)
        BLINKER_LOG("FIRE: ",  "安全");
       
       
        humi_read = h;
        temp_read = t;
        gas_read = a;
        fire_read = f;
        
        if(avegas > 17.46 & avegas <= 28.76){ //低阈值
        delaypush(cMillis, pMillis);
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }

        else if(avegas > 28.76){ //高阈值
        digitalWrite(LED_BUILTIN, LOW);
        Blinker.notify("Gas Alarm!!!");
        }
        
        else if(avegas <= 17.46)
        digitalWrite(LED_BUILTIN, HIGH);

        if(f == LOW) //火焰检测
        digitalWrite(BUZZERPIN,HIGH);
        else if(f == HIGH)
        digitalWrite(BUZZERPIN,LOW);

        Serial.println(pMillis);
        Serial.println(cMillis);
        if(cMillis - pMillis > 10000)
          pMillis = cMillis; //更新pMillis
    }

    Blinker.delay(800); 
}
  
