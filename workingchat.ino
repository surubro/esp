#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define MAX_USERS 10
#define MAX_MESSAGES 20

const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

String users[MAX_USERS];
String messages[MAX_MESSAGES];
int userCount = 0;
int msgCount = 0;

const char* ssid = "ESP_Chat";
const char* password = "";

// === HTML PAGES FIRST ===
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Welcome</title></head><body style="font-family:sans-serif;text-align:center;">
<h2>Enter Your Info</h2>
<form action="/submit" method="POST">
<input name="name" placeholder="Name" required><br><br>
<input name="surname" placeholder="Surname" required><br><br>
<input type="submit" value="Join Chat">
</form>
</body></html>
)rawliteral";

const char chat_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP Chat</title>
<script>
function loadMessages(){
  fetch('/messages').then(r=>r.text()).then(t=>{
    document.getElementById('chat').innerText=t;
    window.scrollTo(0,document.body.scrollHeight);
  });
}
function sendMsg(){
  let msg=document.getElementById('msg').value;
  if(!msg) return;
  fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(msg)});
  document.getElementById('msg').value='';
  loadMessages();
}
setInterval(loadMessages,2000);
</script></head>
<body style="font-family:sans-serif;text-align:center;">
<h2>ESP Chat Room</h2>
<div id="chat" style="border:1px solid #999;height:300px;overflow:auto;text-align:left;padding:5px;"></div>
<br>
<input id="msg" placeholder="Type message"><button onclick="sendMsg()">Send</button>
</body></html>
)rawliteral";

const char info_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Connected Users</title>
<script>
function loadUsers(){
  fetch('/users').then(r=>r.text()).then(t=>{
    document.getElementById('list').innerText=t;
  });
}
setInterval(loadUsers,3000);
</script></head>
<body style="font-family:sans-serif;text-align:center;">
<h2>Connected Users</h2>
<pre id="list"></pre>
</body></html>
)rawliteral";

// === SETUP & SERVER LOGIC ===
void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Catch-all redirect
  server.onNotFound([]() {
    server.sendHeader("Location", "/index.html", true);
    server.send(302, "text/plain", "");
  });

  server.on("/", []() {
    server.sendHeader("Location", "/index.html", true);
    server.send(302, "text/plain", "");
  });

  // Serve pages
  server.on("/index.html", HTTP_GET, []() {
    server.send(200, "text/html", index_html);
  });
  server.on("/chat.html", HTTP_GET, []() {
    server.send(200, "text/html", chat_html);
  });
  server.on("/info.html", HTTP_GET, []() {
    server.send(200, "text/html", info_html);
  });

  // Handle form submit
  server.on("/submit", HTTP_POST, []() {
    String name = server.arg("name");
    String surname = server.arg("surname");
    if (userCount < MAX_USERS) {
      users[userCount++] = name + " " + surname;
    }
    server.sendHeader("Location", "/chat.html", true);
    server.send(302, "text/plain", "");
  });

  // Handle chat message
  server.on("/send", HTTP_POST, []() {
    String msg = server.arg("msg");
    if (msgCount < MAX_MESSAGES) {
      messages[msgCount++] = msg;
    } else {
      for (int i = 1; i < MAX_MESSAGES; i++) messages[i - 1] = messages[i];
      messages[MAX_MESSAGES - 1] = msg;
    }
    server.send(200, "text/plain", "ok");
  });

  // Send messages
  server.on("/messages", HTTP_GET, []() {
    String response;
    for (int i = 0; i < msgCount; i++) {
      response += messages[i] + "\n";
    }
    server.send(200, "text/plain", response);
  });

  // Send users
  server.on("/users", HTTP_GET, []() {
    String response;
    for (int i = 0; i < userCount; i++) {
      response += users[i] + "\n";
    }
    server.send(200, "text/plain", response);
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
