/*
      作者:发明家
      时间:2020.4.11
*/
#include "LedControl.h"
#include <cmath>
#include <ArduinoQueue.h>
#include <arduino-timer.h>
#include <WiFi.h>
#include <WebServer.h>
#define MAX_SIZE 65

LedControl lc=LedControl(36,40,26,1);//实例化点阵屏对象，通过构造函数传入IO
auto timer = timer_create_default();//实例化定时器
WebServer server(80);//webserver

struct Node
{
  int x;
  int y;
};

bool eat_s = false;//Gen_point()与food_judge()通信
bool infi = false;//无限地图
int len = 0,point[2] = {5,5};//蛇体长度与初始食物位置
char Direction = 'd';//蛇移动方向
int snake[MAX_SIZE][2] = {0};//存储蛇体坐标数组
int form = 0;
const char PAGE_INDEX[] PROGMEM= R"=====(
<!DOCTYPE html>
<html>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>贪吃蛇小游戏</title>
<body>
<script>
function up()
{select("u");}
function down()
{select("d");}
function right()
{select("r");}
function left()
{select("l");}
function pause()
{select("p");}
function easy()
{select("e");}
function normal()
{select("n");}
function hard()
{ select("h");}
function infinite()
{select("f");}
function limited()
{ select("m");}
function select(message)
{
  xmlhttp=new XMLHttpRequest();
  var url =  "/?position=" + message;
  xmlhttp.open("GET",url,true);
  xmlhttp.send();
}
</script>
<div>
<center><button type="button" onclick="up()">up</button></center>
<center><button type="button" onclick="left()">left</button><button type="button" onclick="right()">right</button></center>
<center><button type="button" onclick="down()">down</button></center>
<br>
<center><button type="button" onclick="pause()">pause</button></center>
<center><button type="button" onclick="easy()">easy</button></center>
<center><button type="button" onclick="normal()">normal</button></center>
<center><button type="button" onclick="hard()">hard</button></center>
<center><button type="button" onclick="infinite()">infinite</button></center>
<center><button type="button" onclick="limited()">limited</button></center>
</div>
</body>
</html>
)=====";

ArduinoQueue<Node> body(MAX_SIZE);//蛇体数据队列

void Draw(int *x,int *y,int Len);//点阵屏绘图
bool AutoMove(void *);//自动移动
void Move(char receive);//移动函数
void bound_judge();//判断边界以及是否撞到身体
void food_judge(int x,int y);//判断吃到食物
void Gen_point();//生成食物
void Convert();//给Draw传参，队列重新生成，蛇体数据清0
void Gen_map();//生成蛇体坐标数据
void handleRoot();//处理网页信息

void setup() 
{
  Serial.begin(115200);
  lc.shutdown(0,false);
  lc.setIntensity(0,5);
  lc.clearDisplay(0);
  Node first = {0,0};
  body.enqueue(first);
  timer.every(500, AutoMove);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("RC1", "");
  server.on("/", handleRoot);
  server.begin();
  Serial.println("begin");
}

void loop() 
{
  server.handleClient();
  timer.tick();
}

void Gen_map()
{
  int n = body.item_count();
  len = n;
  for (int i = n - 1 ; i >= 0 ; i --)
  {
    snake[i][0] = body.getHead().x;
    snake[i][1] = body.getHead().y;
    body.dequeue();
  }
}

bool AutoMove(void *)
{
  Move(Direction);
  return true;
}

void Move(char receive)
{
      Node temp;
      temp.x = body.getTail().x;
      temp.y = body.getTail().y;
      switch (receive)
      {
         case 'u':
            temp.y --;
            body.enqueue(temp);
            break;
         case 'd':
            temp.y ++;
            body.enqueue(temp);
            break;
         case 'r':
            temp.x ++ ;
            body.enqueue(temp);
            break;
         case 'l':
            temp.x --;
            body.enqueue(temp);
            break;
      };
      food_judge(temp.x,temp.y);
      Gen_map();
      if (eat_s){Gen_point();eat_s = false;}
      bound_judge();
}

void Gen_point()
{
  int point_x = random(0, 8);
  int point_y = random(0, 8);
  bool Status = false;
  while(!Status)
  {
    for (int i =0; i < len; i ++)
    {
      if (snake[i][0] == point_x && snake[i][1] == point_y)
      {
        point_x = random(0, 8);
        point_y = random(0, 8);
        i = 0;
      }
      if (i == len - 1)
      {
        Status = true;
        break;
      }
    }
  }
  point[0] = point_x;
  point[1] = point_y;
}

void bound_judge()
{
  bool Status = true;
  for (int i =0; i < len; i ++)
  {
    Serial.println(infi);
    if (!infi)
    {
      if (snake[i][0] < 0 || snake[i][0] > 7 || snake[i][1] > 7||snake[i][1] < 0) {Status = false;}
    }
    else
    {
      if (snake[i][0] < 0) {snake[i][0] += 8;}
      if (snake[i][1] < 0) {snake[i][1] += 8;}
      if (snake[i][0] > 7) {snake[i][0] -= 8;}
      if (snake[i][1] > 7) {snake[i][1] -= 8;}
    }
    if (i>=1&&(snake[0][1] == snake[i][1])&&(snake[0][0] == snake[i][0])) {Status = false;}
    if (snake[0][0] == snake[1][0]&&snake[0][1] == snake[1][1]) {Status = false;}
  }
  Serial.println(Status);
  if (Status)
  Convert();
  else 
  {
    lc.clearDisplay(0);
    lc. setColumn(0,4,B11111111);
    delay(1000);
    while(!body.isEmpty())
    {
      body.dequeue();
    }
    Node first = {0,0};
    body.enqueue(first);
    Direction = 'd';
    point[0] = 5;
    point[1] = 5;
    Move(Direction);
  }
}

void food_judge(int x,int y)
{
      if (x ==point[0]&&y == point[1])
      {
        eat_s = true;
      }
      else
      {
        body.dequeue();
      }
}

void Convert()
{
  int temp_x[len + 1];
  int temp_y[len + 1];
  for (int i = 0; i < len ; i ++)
  {
    temp_x[i] = snake[i][0];
    temp_y[i] = snake[i][1];
  }
  temp_x[len] = point[0];
  temp_y[len] = point[1];
  Draw(temp_x,temp_y,len + 1);
  Node temp;
  for (int i = len - 1; i >= 0; i --)
  {
    temp.x = snake[i][0];
    temp.y = snake[i][1];
    body.enqueue(temp);
  }
  for (int i = len - 1; i >= 0; i --)
  {
    snake[i][0] = 0;
    snake[i][1] = 0;
  }
}

void Draw(int *x,int *y,int Len)
{
    byte Map[8]={ 0 };
    for (int i = 0; i < Len; i ++)
    {
      Map[7-y[i]] = Map[7-y[i]] + pow(2,7-x[i]);
    }
    for (int i = 0; i <=7; i ++)
    {
     if (form == 0) {lc. setColumn(0,i,Map[i]);}
     else if(form == 255) {lc. setColumn(0,i,form - Map[i]);}
     else {lc. setColumn(0,i,Map[i]^B00001111);}
    }
    delay(200);
}

void handleRoot() 
{
  String web = PAGE_INDEX;
  server.send(200, "text/html", web);
  String message = server.arg("position");
  char temp[message.length()+1];
  strcpy(temp,message.c_str());
  if (temp[0] =='u'||temp[0]=='d'||temp[0]=='l'||temp[0]=='r'||temp[0] =='p') {Direction = temp[0];}
  if (temp[0] =='e') {form = 0;}
  if (temp[0] =='n') {form = 15;}
  if (temp[0] =='h') {form = 255;}
  if (temp[0] == 'f') {infi= true;}
  if (temp[0] == 'm') {infi= false;}
  while (Direction == 'p')
  {
    server.handleClient();
    delay(500);
  }
}
