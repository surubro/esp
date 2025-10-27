#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

String users = "";
String chatMessages = "";

const char* ssid = "ESP8266_Chat";
const char* password = "";

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width,initial-scale=1.0">
    <title>Join Chat</title>
    <style>
      body {
        font-family: Arial;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
        background: linear-gradient(135deg,#74ABE2,#5563DE);
        color: #fff;
      }
      .box {
        background: rgba(255,255,255,0.15);
        padding: 20px;
        border-radius: 15px;
        width: 90%;
        max-width: 320px;
        text-align: center;
      }
      input {
        width: 90%;
        padding: 10px;
        border: none;
        border-radius: 10px;
        margin-bottom: 10px;
        text-align: center;
      }
      button {
        width: 95%;
        padding: 10px;
        border: none;
        background: #00ffb3;
        color: #000;
        border-radius: 10px;
        font-weight: bold;
      }
      button:hover { background: #00e0a3; }
    </style>
  </head>
  <body>
    <div class="box">
      <h2>Join Chat</h2>
      <form action="/join" method="POST">
        <input type="text" name="name" placeholder="First name" required><br>
        <input type="text" name="surname" placeholder="Surname" required><br>
        <button type="submit"></button>
      </form>
      <br>
      <button onclick="window.open('http://192.168.4.1/chat','_blank')">Submit</button>
    </div>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleJoin() {
  String name = server.arg("name");
  String surname = server.arg("surname");
  if (name != "" && surname != "") {
    users += name + " " + surname + "<br>";
  }
  server.sendHeader("Location", "/chat");
  server.send(303);
}

void handleChat() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width,initial-scale=1.0">
    <title>Chat Room</title>
    <style>
      body {
        font-family: Arial;
        margin: 0;
        display: flex;
        flex-direction: column;
        height: 100vh;
        background: #f1f1f1;
      }
      #messages {
        flex: 1;
        overflow-y: auto;
        padding: 10px;
      }
      #chatbox {
        display: flex;
        background: #fff;
        padding: 8px;
      }
      input[type=text] {
        flex: 1;
        padding: 10px;
        border: 1px solid #ccc;
        border-radius: 15px;
        outline: none;
      }
      button {
        margin-left: 5px;
        padding: 10px 15px;
        background: #4CAF50;
        color: white;
        border: none;
        border-radius: 15px;
      }
      .msg {
        margin: 5px 0;
        background: #dcf8c6;
        padding: 8px 12px;
        border-radius: 10px;
        display: inline-block;
      }
    </style>
  </head>
  <body>
    <div id="messages"></div>
    <div id="chatbox">
      <input type="text" id="msg" placeholder="Type a message..." />
      <button onclick="sendMessage()">Send</button>
    </div>
    <script>
      function fetchMessages() {
        fetch('/getMessages')
        .then(r => r.text())
        .then(t => {
          document.getElementById('messages').innerHTML = t;
          document.getElementById('messages').scrollTop = document.getElementById('messages').scrollHeight;
        });
      }
      function sendMessage() {
        var text = document.getElementById('msg').value.trim();
        if (!text) return;
        fetch('/sendMessage', {
          method: 'POST',
          headers: {'Content-Type': 'application/x-www-form-urlencoded'},
          body: 'msg=' + encodeURIComponent(text)
        }).then(() => {
          document.getElementById('msg').value = '';
          fetchMessages();
        });
      }
      setInterval(fetchMessages, 2000);
      fetchMessages();
    </script>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSendMessage() {
  String msg = server.arg("msg");
  if (msg != "") {
    chatMessages += "<div class='msg'>" + msg + "</div><br>";
  }
  server.send(200, "text/plain", "OK");
}

void handleGetMessages() {
  server.send(200, "text/html", chatMessages);
}

void handleInfo() {
  String html = "<html><body><h3>Connected Users:</h3>" + users + "<br><h3>Chat Log:</h3>" + chatMessages + "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(500);

  IPAddress myIP = WiFi.softAPIP();
  Serial.println("AP Started: " + myIP.toString());

  dnsServer.start(DNS_PORT, "*", myIP);

  // Captive portal detection routes
  server.on("/generate_204", [myIP]() {
    server.sendHeader("Location", String("http://") + myIP.toString(), true);
    server.send(302, "text/plain", "");
  });
  server.on("/hotspot-detect.html", [myIP]() {
    server.sendHeader("Location", String("http://") + myIP.toString(), true);
    server.send(302, "text/plain", "");
  });
  server.on("/fwlink", [myIP]() {
    server.sendHeader("Location", String("http://") + myIP.toString(), true);
    server.send(302, "text/plain", "");
  });

  // Normal routes
  server.on("/", handleRoot);
  server.on("/join", HTTP_POST, handleJoin);
  server.on("/chat", handleChat);
  server.on("/sendMessage", HTTP_POST, handleSendMessage);
  server.on("/getMessages", handleGetMessages);
  server.on("/info", handleInfo);

  server.begin();
  Serial.println("Server started!");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
