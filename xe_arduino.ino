/*
******************** Thong tin ********************
written by Nguyễn Bachs - CT020205
Kỹ thuật: 0333148314
Email: nguyenvanbach579@gmail.com
***************************/


/**** Giới thiệu code ***************
 *  ---------------------------------Tự Lái-----------------------------------
 *  Code sử dụng biến giới hạn là 30 cm để xác định khoảng cách cần dừng.
 *  Khi ta muốn thay đổi khoảng cách, ta thay đổi tham số "gioihan" đang để mặc định là 25. ( đơn vị cm)
 *  Xe ưu tiên rẽ trái trong các trường  hợp phía trước có vật cản. 
 *  Ví dụ: khi khoang cách phía trước <  giới hạn, xe lùi lại 1 đoạn
 *  Sau đó, cảm biến đo từ bên trái sang bên phải các góc 0, 45, 90, 135, 180, nếu khoảng cách trước mặt của vị trí nào đủ thì rẽ hướng đó
 *  Nếu không có vị trí đủ thì lại tiếp tục lùi và lại lặp 1 quá trình mới.
 *  
 *  ---------------------------------Người Lái-----------------------------------
 *  Khi nhận được tín hiệu từ bluetooth, biến state sẽ đc gán giá trị nhận được đó
 *  Nếu 0 < state < 10, thì state sẽ là tín hiệu điều khiển hướng đi của xe
 *  Nếu state > 10, thì state sẽ là tín hiệu tốc độ và gán gía trị của state cho biến Speed
 *  Nếu state nhận tín hiệu là 0 thì xe sẽ lại chạy tự động 
 */
#include "DHT.h"       //thư viện đo nhiệt độ và độ ẩm
#include <Servo.h>     //thư viện servo

Servo myservo;         // Tạo đối tượng myservo để điều khiển Servo
/******** khai báo chân input/output**************/
#define gioihan 30                  // khoảng cách giới hạn đến vật cản
#define DHTPIN 11      // Chân data đo nhiệt độ và độ ẩm
#define DHTTYPE DHT11  // kiểu loại của máy đo nhiệt độ và độ ẩm
#define trig A1        //Định nghĩa đầu ra tín hiệu vào là tín hiệu tương tự đầu A1 của SRF-05.
#define echo A2        //Định nghĩa đầu vào tín hiệu vào là tín hiệu tương tự đầu A2 của SRF-05.
int enA = 6;           // chân truyền xung PWM để điều khiển tốc độ cho IN - A và IN - B
int enB = 3;           // chân truyền xung PWM để điều khiển tốc độ cho IN - C và IN - D
int tien1=2;           // chân IN - A của Module L298.
int lui1=4;            // chân IN - B của Module L298.
int tien2=12;          // chân IN - C của Module L298.
int lui2=13;           // chân IN - D của Module L298.
int dongcoservo=10;    // chân Orange của Servo.

unsigned long thoigian;            // biến đo thời gian
int khoangcach;                    // biến lưu khoảng cách thẳng đến vật cản khi tự lái
int khoangcachtrai,khoangcachphai; // biến lưu khoảng cách trái, phải đến vật cản khi tự lái

DHT dht(DHTPIN, DHTTYPE);          //Tạo đối tượng đo nhiệt và ẩm 
int state = 0;                     // biến trạng thái chuyển đổi chế độ và di chuyển khi người lái
int Speed = 70;                    // tốc độ người điều khiển

int temp;                          // biến nhiệt độ
int hum;                           // biến độ ẩm

int distance;                      // biến khoảng cách vật cản khi người lái
boolean chk = true;                       // biến kiểm tra có đc đi thẳng khi người lái

/*Khai báo các hàm căn bản trong chương trình*/
int dokhoangcach();
void dithang();
void disangtrai();
void disangphai();
void dilui();
void resetdongco();
int quaycb(int a);
void setEn(int en);

void setup() {
  // chạy 1 lần duy nhất khi khởi tạo
    myservo.attach(dongcoservo);    // nối chân servo ở pin 9 tới đối tượng myservo
    pinMode(trig,OUTPUT);           // chân trig sẽ phát tín hiệu
    pinMode(echo,INPUT);            // chân echo sẽ nhận tín hiệu
    pinMode(enA, OUTPUT);           // chân enA sẽ phát tín hiệu
    pinMode(enB, OUTPUT);           // chân enB sẽ phát tín hiệu
    pinMode(tien1,OUTPUT);          // chân tien1 sẽ phát tín hiệu
    pinMode(tien2,OUTPUT);          // chân tien2 sẽ phát tín hiệu
    pinMode(lui1,OUTPUT);           // chân lui1 sẽ phát tín hiệu 
    pinMode(lui2,OUTPUT);           // chân lui2 sẽ phát tín hiệu
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    myservo.write(90);              // quay servo thẳng trước mặt 
}

void loop() 
{   
  if(Serial.available() > 0 || state != 0){     // nếu state bị thay đổi thì xe sẽ chuyển sang chế độ người lái
    state = Serial.read();                      // Đọc tín hiệu từ app
    nguoiLai();
  }
    
  if(state == 0)                                // nếu state = 0 thì xe vẫn tự chạy
    tuLai();
}

void nguoiLai(){
   // Nếu state > 10 thì sẽ gán cho tốc độ
   if(state > 10)     
     Speed = state;

   // Đo khoảng cách trước mặt
   distance = quaycb(90);

   // Nếu khoảng cách nhỏ hơn giói hạn thì kiểm tra check
   if(distance<gioihan){
      chk = false; 
      resetdongco();      // Dừng động cơ
   }
   if(distance>gioihan){
      chk = true;
   }
   // Nếu state nhận được '1' thì xe đi thẳng cho đến khi nhấc tay khỏi phím bấm
   if ((state == 1) && chk){
      dithang(Speed);
   }

   // Nếu state nhận được '2' thì xe đi lùi cho đến khi nhấc tay khỏi phím bấm
   else if (state == 2){
    dilui(Speed);
   }
    
   // Nếu state nhận được '3' thì xe sang trái cho đến khi nhấc tay khỏi phím bấm
   else if (state == 3){
    disangtrai(Speed);
   }
    
   // Nếu state nhận được '4' thì xe đi lùi cho đến khi nhấc tay khỏi phím bấm
   else if (state == 4){
    disangphai(Speed);
   }
    
   // Nếu state nhận được '5' thì xe dừng lại 
   else if (state == 5) {
    resetdongco();
   }  
   /* Gửi tín hiệu qua bluetooth */
   Serial.print("A");
   Serial.print(";");
   Serial.print(distance); //Gửi khoảng cách đến vật cản qua App
   Serial.print(";");
   Serial.print(distance); //Gửi khoảng cách đến vật cản qua App
   Serial.println(";");
   delay(50); 
   hum = dht.readHumidity();
   temp = dht.readTemperature();                                                         
   Serial.print("B");
   Serial.print(";");
   Serial.print(temp); //Gửi nhiệt độ qua App
   Serial.print(";");
   Serial.print(hum);  //Gửi độ ẩm qua App
   Serial.println(";");     
  
}

void tuLai(){
    int cheophai = 0;
    int cheotrai = 0;
    khoangcach = quaycb(90);
    if(khoangcach<=gioihan) 
    {
      resetdongco();
      delay(200);
      dilui(80);
      delay(450);
      resetdongco();
      delay(200);
      khoangcachphai = quaycb(0);
      delay(50);
      cheophai = quaycb(45);
      delay(50);
      cheotrai = quaycb(135);
      delay(50);
      khoangcachtrai = quaycb(180);
      delay(50);
      myservo.write(90);   
      if(khoangcachphai <= gioihan && khoangcachtrai <= gioihan){
        if(cheophai <= gioihan && cheotrai <= gioihan){
          resetdongco();
          delay(200);
          dilui(80);
          delay(450);
          resetdongco();
          delay(200);
        }
        else{
          if (cheophai >= cheotrai)
          {
            disangphai(90);//45
            delay(450);
            resetdongco();
          }
        else
          {
            disangtrai(90);//135
            delay(450);
            resetdongco();
          }
        }
      }
      else{
        if (khoangcachphai >= khoangcachtrai)
          {
            disangphai(120);
            delay(500);
            resetdongco();
          }
        else
          {
            disangtrai(120); 
            delay(500);
            resetdongco();
          }
      }
    }
    else
    {
      dithang(70);
    }
}

void setEn(int en){
  analogWrite(enA, en); // phát xung điều khiển tốc độc động cơ
  analogWrite(enB, en); 
}

void dithang(int en)
{ 
  digitalWrite(tien1,HIGH); // phát tín hiệu điều khiển chiều dòng điện từ HIGH -> LOW
  digitalWrite(lui1,LOW);
  digitalWrite(tien2,HIGH);
  digitalWrite(lui2,LOW);
  setEn(en);
}

void disangtrai(int en)
{
  digitalWrite(tien1,LOW);
  digitalWrite(lui1,HIGH);
  digitalWrite(tien2,HIGH);
  digitalWrite(lui2,LOW);  
  setEn(en);
}
void disangphai(int en)
{
  digitalWrite(tien1,HIGH);
  digitalWrite(lui1,LOW);
  digitalWrite(tien2,LOW);
  digitalWrite(lui2,HIGH);
  setEn(en);
}

void dilui(int en)
{
  digitalWrite(tien1,LOW);
  digitalWrite(lui1,HIGH);
  digitalWrite(tien2,LOW);
  digitalWrite(lui2,HIGH);
  setEn(en);
}
void quaylai(int en)
{
  digitalWrite(tien1,HIGH);
  digitalWrite(lui1,LOW);
  digitalWrite(tien2,LOW);
  digitalWrite(lui2,LOW);
  setEn(en);
}

void resetdongco()
{
  digitalWrite(tien1,LOW);
  digitalWrite(lui1,LOW);
  digitalWrite(tien2,LOW);
  digitalWrite(lui2,LOW);
}
/******** chương trình đo khoảng cách SRF05 ***************/
int dokhoangcach()
{
   delay(100);
/* Phát xung từ chân trig */
    digitalWrite(trig,LOW);   // tắt chân trig
    delayMicroseconds(2);
    digitalWrite(trig,HIGH);   // phát xung từ chân trig
    delayMicroseconds(10);   // xung có độ dài 10 microSeconds
    digitalWrite(trig,LOW);   // tắt chân trig
    
    /* Tính toán thời gian */
    // Đo độ rộng xung HIGH ở chân echo. 
    thoigian = pulseIn(echo,HIGH);  
    // Tính khoảng cách đến vật.
    return int(thoigian*0.034/2);
}

/*********** chương trình quay cảm biến xang trái phải*********/
int quaycb(int a)
{
    myservo.write(a);              // Quay servo theo góc a
    khoangcach = dokhoangcach();
    delay(50);
    return khoangcach;
}
