// ESP32通过AP获取WiFi信息并储存在EEPROM中
// 联系：yaoyuwl@gmail.com yaoyuwl@qq.com

#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <DNSServer.h>

#define SSID_LENGTH 32  // 名称最大长度
#define PAWD_LENGTH 64  // 密码最大长度
#define INITADDRESS 0   // 起始位置
#define ONLOGS      1   // 0关闭输出日志 1打开输出日志

WebServer webServer;
DNSServer dnsServer;

void setup() {
  if (ONLOGS) Serial.begin(9600);
  int length = INITADDRESS + SSID_LENGTH + PAWD_LENGTH;
  EEPROM.begin(length);
  // for (int i=INITADDRESS; i<length; i++) 
  //   EEPROM.write(i, '\0');  // 清除EEPROM数据 需要的请打开注释
  WiFi.begin();
  logs("\n初始化设置完成");
}
void loop() {
  wifi();
}

void wifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (WiFi.getMode() == WIFI_STA) {
    String ssid, pawd;
    readWiFiConfig(ssid, pawd);
    if (ssid == "") getWiFiConfig();  // 若EEPROM无WiFi配置则开启配网模式
    else connectToWiFi();  // 存在WiFi配置信息 连接WiFi
  }
  else if (WiFi.getMode() == WIFI_AP) {  // 配网时回调所需的DNS服务和WEB服务
    webServer.handleClient();
    dnsServer.processNextRequest();
  }
}

void connectToWiFi() {  // 连接WiFi
  const int outtime  = 1000 * 60;
  static long connouttime = millis() - outtime - 1;
  if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED && millis()-connouttime > outtime) {
    String ssid, pawd;
    readWiFiConfig(ssid, pawd);
    logs("开始连接WiFi至"+ssid+pawd);
    WiFi.begin(ssid, pawd);
    connouttime = millis();
  }
  if (connouttime > millis()) connouttime = millis();
}
void getWiFiConfig() {  // 配网模式
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,1,4), IPAddress(192,168,1,4), IPAddress(255,255,255,0));
  WiFi.softAP("YAO-IOT");  // 配网热点名称
  dnsServer.start(53, "*", IPAddress(192,168,1,4));
  webServer.on("/", HTTP_GET, handleRoot);
  webServer.on("/", HTTP_POST, handleRootPost);
  webServer.onNotFound(handleRoot);
  webServer.begin(80);
  logs("已经入配网模式\n请用手机或电脑连接热点\""+WiFi.softAPSSID()+"\"进行配网\n如果没有自动跳转到配网页面\n请在浏览器访问 "+WiFi.softAPIP().toString()+" 进行配网");
}
void handleRoot() {
  const String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>YAO-IOT</title><style>*{padding:0;margin:0;outline:0;font-size:20px}body{width300px;margin:0 auto}main{height:calc(100vh - 40px);border-left:10px #444693 solid;border-right:10px #444693 solid;padding:20px;display:flex;gap:20px;flex-direction:column;justify-content:space-between}form{display:flex;flex-direction:column;gap:20px}h1{color:#444693;font-size:30px;text-align:center;cursor:pointer}label{color:#444693;cursor:pointer}input{border-width:0 0 2px 0}input:hover{border-color:#444693;background:#44469310}button{line-height:40px;border-radius:20px;color:#444693;font-weight:500;border:#444693 5px solid;margin-top:15px}button:hover{background:#444693;color:#FFF;font-weight:800;cursor:pointer}footer *{font-size:15px}p:hover{background:#44469310;cursor:pointer}span{color:#444693}</style></head><body><main><form action=\"/\" method=\"post\"><h1>YAO-IOT</h1><label for=\"ssid\">WiFi账户</label><input type=\"text\" name=\"ssid\" id=\"ssid\" /><label for=\"pawd\">WiFi密码</label><input type=\"text\" name=\"pawd\" id=\"pawd\" /><button type=\"submit\">同意某某协议并上传设备</button></form><footer><p><span>联系</span></p><p><span>yaoyuwl</span>@qq.com</p><p><span>yaoyuwl</span>@gmail.com</p></footer></main></body></html>";
  webServer.send(200, "text/html", html);
}
void handleRootPost() {
  String ssid, pawd;
  ssid = webServer.arg("ssid");
  pawd = webServer.arg("pawd");
  if (ssid.length() > 0) {
    webServer.send(200, "text/html", "<script>alert('上传成功，若60秒内设备仍未连接上WiFi则需要重新配网')</script>");
    logs("成功，已获取WiFi账密信息，如果180秒内未连接成功则需要重新配网\n");
    delay(1000);
  } else {
    webServer.send(200, "text/html", "<script>alert('错误，没有获取到WIFI账密信息，请重试！')</srcipt>");
    logs("错误，没有获取到WiFi账密信息\n");
    delay(1000);
  }
  webServer.stop();
  dnsServer.stop();
  if (ssid.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pawd);
    logs("开始连接WiFi，若180内未连接则需要重新配网");
    int m = millis();
    while(millis()-m < 1000*60*3 && WiFi.status()!=WL_CONNECTED);  // 若三分钟内连接失败则需要重新配网
    if (WiFi.status() != WL_CONNECTED) return;
    writeWiFiConfig(ssid, pawd);
    logs("连接成功 配网完成");
  }
}

void readWiFiConfig(String& ssid, String& pawd) {  // 读取WiFi配置信息
  readEEPROM(INITADDRESS, INITADDRESS+SSID_LENGTH, ssid);
  readEEPROM(INITADDRESS+SSID_LENGTH, INITADDRESS+SSID_LENGTH+PAWD_LENGTH, pawd);
}
void writeWiFiConfig(const String ssid, const String pawd) {  // 写入WiFi配置信息
  writeEEPROM(INITADDRESS, INITADDRESS+SSID_LENGTH, ssid);
  writeEEPROM(INITADDRESS+SSID_LENGTH, INITADDRESS+SSID_LENGTH+PAWD_LENGTH, pawd);
}
void readEEPROM(int startAddr, int endAddr, String& date) {  // 从EEPROM读取数据  
  date = '\0';
  while (startAddr<endAddr) {
    char c = EEPROM.read(startAddr++);
    if (c == '\0') break;
    date += c;
  }
}
void writeEEPROM(int startAddr, int endAddr, const String date) {  // 写入数据到EEPROM
  if (date.length() < endAddr-startAddr) endAddr = startAddr+date.length();
  for (int i=0; startAddr<endAddr; i++)
    EEPROM.write(startAddr++, date[i]);
  EEPROM.commit();
}

void logs(const String log) {  // 输出日志
  if (!ONLOGS) return;
  Serial.println(log);
}