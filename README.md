# cathook bot panel

> Web panel สำหรับจัดการ cathook bots ผ่าน IPC — ควบคุม เริ่ม หยุด และมอนิเตอร์ bots ทั้งหมดได้จากเบราว์เซอร์

**Nullware 2026** · [GitHub](https://github.com/NullwareN)

---

## 📋 สารบัญ

- [ความต้องการของระบบ](#-ความต้องการของระบบ)
- [โครงสร้างโปรเจกต์](#-โครงสร้างโปรเจกต์)
- [การติดตั้ง](#-การติดตั้ง)
- [การใช้งาน](#-การใช้งาน)
- [ตัวเลือก Environment Variables](#-environment-variables)
- [หน้าแผงควบคุม (Web Panel)](#-หน้าแผงควบคุม-web-panel)
- [การตั้งค่า Bot](#-การตั้งค่า-bot)
- [การแก้ปัญหาเบื้องต้น](#-การแก้ปัญหาเบื้องต้น)

---

## ⚙️ ความต้องการของระบบ

| สิ่งที่ต้องมี | เวอร์ชัน |
|---|---|
| Linux (Ubuntu/Debian แนะนำ) | — |
| Node.js หรือ nodejs | ≥ 14 |
| cathook (ติดตั้งแล้ว) | — |
| สิทธิ์ root | จำเป็น |

> [!IMPORTANT]
> Script ทั้งหมดต้องรันด้วยสิทธิ์ **root** (`sudo`) เท่านั้น

---

## 📁 โครงสร้างโปรเจกต์

```
cathook-botpanel/
├── catbot-ipc-web-panel-main/   ← Web Panel (Node.js)
│   ├── public/                  ← ไฟล์ HTML/CSS (UI)
│   ├── forever/                 ← Bot manager, ban tracker
│   ├── app.js                   ← Entry point (Express server)
│   ├── auth.js                  ← ระบบ login / API key
│   ├── run.sh                   ← Script สำหรับสตาร์ท panel
│   └── package.json
├── catbot-ipc-server-main/      ← IPC Server (C++)
├── configs/
│   └── bot.conf                 ← ค่าคอนฟิก cathook สำหรับ bots
└── install-catbots              ← Script ติดตั้งอัตโนมัติ
```

---

## 🚀 การติดตั้ง

### 1. Clone โปรเจกต์

```bash
git clone https://github.com/NullwareN/cathook-botpanel.git
cd cathook-botpanel
```

### 2. ติดตั้ง Node.js dependencies

```bash
cd catbot-ipc-web-panel-main
npm install
```

### 3. ติดตั้ง bots (ครั้งแรก)

```bash
sudo bash install-catbots
```

---

## ▶️ การใช้งาน

### เริ่ม Panel

```bash
sudo bash start
```

หรือรันตรงๆ:

```bash
cd catbot-ipc-web-panel-main
sudo bash run.sh
```

Panel จะเปิดที่ **`http://localhost:7655`** (หรือ IP เครื่อง:7655)

### หยุด Panel

```bash
sudo bash stop
```

### อัปเดต

```bash
sudo bash update
```

---

## 🔐 การ Login

| สถานการณ์ | วิธีการ |
|---|---|
| เข้าจาก **LAN / localhost** | ไม่ต้อง login อัตโนมัติ ✅ |
| เข้าจาก **ภายนอก** | ต้องใส่รหัสผ่าน |

**รหัสผ่าน** ถูกสร้างอัตโนมัติทุกครั้งที่รัน panel และบันทึกไว้ที่:

```
/opt/cathook/run/cat-webpanel-password
```

อ่านรหัสผ่านด้วย:

```bash
cat /opt/cathook/run/cat-webpanel-password
```

กำหนดรหัสผ่านเองได้โดยตั้ง environment variable:

```bash
export CAT_WEB_PASSWORD="รหัสผ่านของคุณ"
sudo bash start
```

---

## 🌐 หน้าแผงควบคุม (Web Panel)

### ภาพรวม UI

```
[ cathook ]
─────────────────────────────────────────────────────
[ Command for every client _________________ ] [Submit]

Status: Connected

[Restart all] [Terminate all] [Refresh bot list]
Quota: [3] [Apply quota]   Max concurrent: [3] [Apply]
─────────────────────────────────────────────────────
| Actions | Restarts | Bot Name | State | Steam | ... |
|---------|----------|----------|-------|-------|-----|
| [R][T]  |    2     | bot_001  |  ...  |  OK   | ... |
─────────────────────────────────────────────────────
☐ Auto-restart nonresponsive bots
☐ Account ban tracker   Auto-restart Steam if not logged within: [70]s [Apply]
```

### ปุ่มและฟังก์ชัน

| ปุ่ม / ฟิลด์ | หน้าที่ |
|---|---|
| **Submit** | ส่งคำสั่งไปยัง bots ทุกตัวพร้อมกัน |
| **Restart all** | รีสตาร์ท bots ทุกตัว |
| **Terminate all** | ปิด bots ทุกตัวทันที |
| **Refresh bot list** | โหลดรายชื่อ bot ใหม่ |
| **Quota** | จำนวน bot ที่ต้องการรัน (สูงสุด 254) |
| **Max concurrent** | จำนวน bot ที่เริ่มพร้อมกันได้สูงสุด |
| **Apply quota** | บันทึกจำนวน bot |

### คอลัมน์ในตาราง Bot

| คอลัมน์ | ความหมาย |
|---|---|
| **Actions** | ปุ่มควบคุมรายตัว (Restart / Terminate) |
| **Restarts** | จำนวนครั้งที่ restart แล้ว |
| **Bot Name** | ชื่อ user account |
| **State** | สถานะปัจจุบัน |
| **Steam** | สถานะ Steam login |
| **Ban Tracker** | ตรวจสอบการโดน ban |
| **Uptime** | เวลาที่รันมาแล้ว |
| **IPC Status** | สถานะการเชื่อมต่อ IPC |
| **Score / Shots / Hit %** | สถิติการเล่น |
| **Server IP / Map / Players** | ข้อมูลเซิร์ฟเวอร์ปัจจุบัน |

### Checkboxes

| Checkbox | หน้าที่ |
|---|---|
| ☐ **Auto-restart nonresponsive bots** | รีสตาร์ท bot อัตโนมัติถ้าไม่ตอบสนอง |
| ☐ **Account ban tracker** | เปิดการตรวจสอบ ban ของ account |
| **Auto-restart Steam if not logged within** | รีสตาร์ท Steam ถ้า login ไม่สำเร็จใน X วินาที |

---

## 🔧 Environment Variables

ปรับแต่งการทำงานผ่าน environment variables ก่อนรัน:

```bash
export VARIABLE=value
sudo bash start
```

| Variable | ค่าเริ่มต้น | ความหมาย |
|---|---|---|
| `CAT_IPC_PORT` | `7655` | Port ของ web panel |
| `CAT_IPC_BIND` | `0.0.0.0` | IP ที่ panel รับการเชื่อมต่อ |
| `CAT_WEB_PASSWORD` | (สุ่ม) | รหัสผ่าน login |
| `CAT_RUNTIME_DIR` | `/opt/cathook/run` | โฟลเดอร์ runtime |
| `CAT_PANEL_LOG` | `panel.log` | ไฟล์ log |
| `CAT_PER_BOT_X_DISPLAY` | `1` | แต่ละ bot ใช้ X display แยก |
| `CAT_TEXTMODE_GAME` | `1` | รันเกมใน textmode |
| `CAT_STM_WEBHELPER_NOSANDBOX` | `1` | ปิด Steam sandbox |

---

## 🗂️ การตั้งค่า Bot

ไฟล์คอนฟิก cathook สำหรับ bots อยู่ที่ [`configs/bot.conf`](configs/bot.conf)

ตัวอย่างค่าสำคัญ:

```ini
# การ Queue เข้าเกมอัตโนมัติ
autojoin.auto-accept-q=true
autojoin.auto-queue=true
autojoin.class=1          # 1 = Scout

# Bot behavior
cat-bot.enable=true
cat-bot.abandon-if.humans-lte=10   # ออกถ้าเหลือคนจริงน้อยกว่า 10
cat-bot.always-crouch=true

# ป้องกัน MOTD / anti-report bypass
cat-bot.anti-motd=true
cat-bot.autoqueue-report=true
misc.cheats-bypass=true
misc.insecure-bypass=true
```

> [!NOTE]
> แก้ไข `bot.conf` แล้ว Apply quota ใหม่เพื่อให้ bots โหลดค่าใหม่

---

## 🛠️ การแก้ปัญหาเบื้องต้น

### Panel ไม่สตาร์ท

```bash
# ตรวจสอบว่ารันด้วย root
sudo bash run.sh

# ตรวจสอบ node ติดตั้งแล้ว
node --version || nodejs --version

# ดู log
cat catbot-ipc-web-panel-main/panel.log
```

### Panel รันอยู่แล้ว (already running)

```bash
# ลบ PID file แล้วรันใหม่
rm -f /tmp/ncat-cathook-webpanel.pid
sudo bash start
```

### Bot ไม่เชื่อมต่อ IPC

```bash
# ตรวจสอบ IPC server ทำงานอยู่
ps aux | grep catbot-ipc

# รัน IPC server setup ใหม่
sudo bash catbot-ipc-web-panel-main/scripts/ensure-ipc-server.sh
```

### ดู Crash Log

```bash
cat catbot-ipc-web-panel-main/logs/main.crash.log
```

---

## 📜 License

ISC — nullifiedcat / Nullware 2026
