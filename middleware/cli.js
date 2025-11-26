const { spawn } = require('child_process');
const path = require('path');
const readline = require('readline');

// --- CẤU HÌNH ---
const CLIENT_PATH = path.join(__dirname, '../bin/clientd');

// --- GIAO DIỆN NHẬP LIỆU ---
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    prompt: 'YOU> '
});

// --- KHỞI CHẠY CLIENT C ---
console.log(`Dang khoi dong C Client tu: ${CLIENT_PATH}...`);
const client = spawn(CLIENT_PATH, ['127.0.0.1']);
client.stdin.setEncoding('utf-8');

// --- XỬ LÝ PHẢN HỒI TỪ SERVER ---
client.stdout.on('data', (data) => {
    const lines = data.toString().split('\n');
    lines.forEach(line => {
        if (!line.trim()) return;
        try {
            const json = JSON.parse(line);
            formatServerMessage(json);
        } catch (e) {
            console.log("RAW:", line);
        }
    });
    rl.prompt();
});

client.stderr.on('data', (data) => {
    console.error(`[CLIENT-ERR]: ${data}`);
});

client.on('close', (code) => {
    console.log(`Client da thoat (Code ${code})`);
    process.exit(0);
});

// --- HÀM IN KẾT QUẢ ĐẸP MẮT ---
function formatServerMessage(json) {
    console.log("\r"); // Xóa dòng prompt hiện tại
    switch (json.type) {
        case 800: console.log(`✅ OK: ${json.message}`); break;
        case 801: console.log(`❌ LỖI: ${json.message} (Code: ${json.error_code})`); break;
        case 802: console.log(`🔓 ĐĂNG NHẬP THÀNH CÔNG!`); break;
        
        case 810: // List Room
            console.log(`📋 DANH SÁCH PHÒNG:`);
            if (json.rooms) {
                json.rooms.forEach(r => console.log(`   - [ID ${r.id}] ${r.title} | Giá: ${r.price}$`));
            }
            break;

        case 901: console.log(`👋 User mới đã vào phòng: Room ${json.room_id}`); break;
        
        case 904: // New Bid
            console.log(`\n🔥 [GIÁ MỚI] ${json.bidder} vừa trả: ${json.current_price} VND`);
            break;
            
        case 906: // Auction Ended
            console.log(`\n🛑 [KẾT THÚC] Người thắng: ${json.winner} - Giá chốt: ${json.final_price} VND`);
            break;

        default:
            console.log("📩 SERVER:", JSON.stringify(json));
    }
}

// --- XỬ LÝ LỆNH NGƯỜI DÙNG NHẬP ---
console.log("=== HỆ THỐNG ĐẤU GIÁ (CLI MODE) ===");
console.log("Các lệnh hỗ trợ:");
console.log("  login <user> <pass>      : Đăng nhập");
console.log("  create <tên> <giá>       : Tạo phòng (VD: create iPhone 1000)");
console.log("  list                     : Xem danh sách phòng");
console.log("  join <id>                : Vào phòng");
console.log("  bid <giá>                : Đấu giá");
console.log("  exit                     : Thoát");
console.log("======================================");

rl.prompt();

rl.on('line', (line) => {
    const args = line.trim().split(' ');
    const cmd = args[0].toLowerCase();
    let payload = null;

    switch (cmd) {
        case 'login':
            if (args.length < 3) console.log("Thiếu user/pass!");
            else payload = { type: 102, user: args[1], pass: args[2] };
            break;
        
        case 'create':
            if (args.length < 3) console.log("Thiếu tên hoặc giá!");
            else payload = { type: 202, title: args[1], start_price: parseInt(args[2]) };
            break;

        case 'list':
            payload = { type: 201 };
            break;

        case 'join':
            if (args.length < 2) console.log("Thiếu ID phòng!");
            else payload = { type: 203, room_id: parseInt(args[1]) };
            break;

        case 'bid':
            if (args.length < 2) console.log("Thiếu giá tiền!");
            else payload = { type: 401, price: parseInt(args[1]) };
            break;

        case 'exit':
            rl.close();
            process.exit(0);
            break;

        default:
            console.log("Lệnh không hợp lệ!");
    }

    if (payload) {
        // Gửi JSON xuống Client C
        client.stdin.write(JSON.stringify(payload) + "\n");
    }
    
    // rl.prompt() sẽ được gọi sau khi server phản hồi để prompt không bị trôi
});