// middleware/bridge.js
const { spawn } = require('child_process');
const path = require('path');
const EventEmitter = require('events');

class AuctionCore extends EventEmitter {
    constructor() {
        super();
        this.process = null;
        this.buffer = ''; // Dùng để gom các mảnh data bị cắt rời
    }

    start() {
        // Đường dẫn tới file clientd đã compile
        const exePath = path.join(__dirname, '../bin/clientd');
        
        console.log(`[Bridge] Starting Core Client: ${exePath}`);
        this.process = spawn(exePath, ['127.0.0.1']);

        this.process.stdin.setEncoding('utf-8');

        // Xử lý dữ liệu từ C gửi lên
        this.process.stdout.on('data', (data) => {
            this.buffer += data.toString();
            this.processBuffer();
        });

        this.process.stderr.on('data', (data) => {
            console.error(`[C-ERR]: ${data}`);
        });

        this.process.on('close', (code) => {
            console.log(`[Bridge] Core process exited with code ${code}`);
            this.emit('exit', code);
        });
    }

    // Hàm xử lý buffer để tách từng dòng JSON
    processBuffer() {
        let boundary = this.buffer.indexOf('\n');
        while (boundary !== -1) {
            const line = this.buffer.substring(0, boundary).trim();
            this.buffer = this.buffer.substring(boundary + 1);
            
            if (line) {
                try {
                    const json = JSON.parse(line);
                    this.emit('message', json); // Bắn sự kiện cho Node xử lý
                } catch (e) {
                    console.error("[Bridge] JSON Parse Error:", line);
                }
            }
            boundary = this.buffer.indexOf('\n');
        }
    }

    // Gửi lệnh xuống C
    send(command) {
        if (this.process) {
            this.process.stdin.write(JSON.stringify(command) + '\n');
        }
    }
}

// --- PHẦN SERVER NODEJS (Express + Socket.io) ---
const express = require('express');
const http = require('http');
const { Server } = require("socket.io");

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
    cors: { origin: "*" } // Cho phép React connect
});

// Khởi tạo cầu nối C
const auctionCore = new AuctionCore();
auctionCore.start();

// Khi C gửi tin về (VD: Giá mới, Hết giờ...) -> Bắn ra Socket cho React
auctionCore.on('message', (json) => {
    console.log("[Core -> Node]:", json);
    
    // Tùy loại tin mà bắn event khác nhau
    if (json.type === 904) { // S2C_NEW_BID
        io.to("room_" + json.room_id).emit("new_bid", json);
    } 
    else if (json.type === 906) { // S2C_AUCTION_ENDED
        io.to("room_" + json.room_id).emit("auction_ended", json);
    }
    else {
        // Các tin khác (Login success, Join success...)
        // Logic này cần mapping với socket.id của user (hơi phức tạp xíu)
        // Tạm thời bắn broadcast để test
        io.emit("server_message", json);
    }
});

io.on('connection', (socket) => {
    console.log('User connected:', socket.id);

    // React gửi lệnh -> Node chuyển xuống C
    socket.on('action', (data) => {
        console.log("[Node -> Core]:", data);
        auctionCore.send(data);
        
        // Nếu là lệnh Join Room, ta cho socket join room ảo của socket.io luôn
        if (data.type === 203) { // JOIN_ROOM
             socket.join("room_" + data.room_id);
        }
    });
});

server.listen(3000, () => {
    console.log('NodeJS Middleware running on port 3000');
});