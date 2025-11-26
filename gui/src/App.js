import React, { useState, useEffect, useRef } from 'react';
import io from 'socket.io-client';
import './App.css'; // Bạn có thể tự style thêm cho đẹp

// Kết nối tới NodeJS Middleware đang chạy ở port 3000
const socket = io('http://localhost:3000');

function App() {
  const [isConnected, setIsConnected] = useState(socket.connected);
  const [user, setUser] = useState(null); // Lưu thông tin user sau khi login
  const [currentRoom, setCurrentRoom] = useState(null); // Phòng đang tham gia
  const [rooms, setRooms] = useState([]); // Danh sách phòng
  const [logs, setLogs] = useState([]); // Log chat/đấu giá

  // Form inputs
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [bidAmount, setBidAmount] = useState(0);
  const [newRoomTitle, setNewRoomTitle] = useState('');
  const [newRoomPrice, setNewRoomPrice] = useState(0);

  useEffect(() => {
    socket.on('connect', () => setIsConnected(true));
    socket.on('disconnect', () => setIsConnected(false));

    // Lắng nghe tin nhắn từ Server gửi về
    socket.on('server_message', (data) => {
      console.log("Server Msg:", data);
      
      // Xử lý phản hồi Login
      if (data.type === 802) { 
        alert("Đăng nhập thành công!");
        setUser(username);
        // Sau khi login, xin danh sách phòng ngay
        socket.emit('action', { type: 201 }); 
      }
      
      // Xử lý phản hồi Lỗi
      if (data.type === 801) {
        alert("Lỗi: " + data.message);
      }

      // Xử lý danh sách phòng (S2C_ROOM_LIST)
      if (data.type === 810) {
        setRooms(data.rooms || []);
      }

      // Xử lý vào phòng thành công (S2C_JOIN_ROOM_SUCCESS)
      if (data.type === 901) {
        // Server báo user khác vào, hoặc chính mình vào
        // Để đơn giản, ta chỉ set currentRoom khi nhận phản hồi trực tiếp
      }
    });

    // Sự kiện Bid mới (Realtime)
    socket.on('new_bid', (data) => {
      addLog(`🔥 ${data.bidder} vừa trả giá: ${data.current_price} VND`);
      // Cập nhật lại giá trên UI nếu đang ở đúng phòng
      if (currentRoom && currentRoom.id === data.room_id) {
        setCurrentRoom(prev => ({ ...prev, price: data.current_price }));
      }
    });

    // Sự kiện Hết giờ (Realtime)
    socket.on('auction_ended', (data) => {
      addLog(`🛑 PHIÊN ĐẤU GIÁ KẾT THÚC! Người thắng: ${data.winner} - Giá: ${data.final_price}`);
      alert(`Phòng ${data.room_id} đã đóng. Người thắng: ${data.winner}`);
    });

    return () => {
      socket.off('connect');
      socket.off('server_message');
      socket.off('new_bid');
      socket.off('auction_ended');
    };
  }, [username, currentRoom]);

  const addLog = (msg) => {
    setLogs(prev => [msg, ...prev]);
  };

  // --- ACTIONS ---

  const handleLogin = () => {
    // Gửi lệnh Login xuống C (Type 102)
    socket.emit('action', { type: 102, user: username, pass: password });
  };

  const handleCreateRoom = () => {
    // Type 202: Create Room
    socket.emit('action', { type: 202, title: newRoomTitle, start_price: parseInt(newRoomPrice) });
    // Refresh list sau 1s
    setTimeout(() => socket.emit('action', { type: 201 }), 1000);
  };

  const handleJoinRoom = (room) => {
    // Type 203: Join Room
    socket.emit('action', { type: 203, room_id: room.id });
    setCurrentRoom(room);
    setLogs([]); // Clear log cũ
    addLog(`Bạn đã vào phòng: ${room.title}`);
  };

  const handleBid = () => {
    if (!currentRoom) return;
    // Type 401: Bid
    socket.emit('action', { type: 401, price: parseInt(bidAmount) });
  };

  const handleRefreshRooms = () => {
    socket.emit('action', { type: 201 });
  }

  // --- RENDER ---

  if (!isConnected) return <div>Đang kết nối tới Server...</div>;

  // MÀN HÌNH 1: LOGIN
  if (!user) {
    return (
      <div className="container">
        <h2>Đăng nhập Hệ thống Đấu giá</h2>
        <input placeholder="Username" onChange={e => setUsername(e.target.value)} />
        <input placeholder="Password" type="password" onChange={e => setPassword(e.target.value)} />
        <button onClick={handleLogin}>Login</button>
      </div>
    );
  }

  // MÀN HÌNH 2: TRONG PHÒNG ĐẤU GIÁ
  if (currentRoom) {
    return (
      <div className="container">
        <button onClick={() => setCurrentRoom(null)}>⬅ Quay lại sảnh</button>
        <h1>Phòng: {currentRoom.title}</h1>
        <h2 style={{color: 'red'}}>Giá hiện tại: {currentRoom.price} VND</h2>
        
        <div className="control-panel">
          <input 
            type="number" 
            placeholder="Nhập giá muốn mua" 
            onChange={e => setBidAmount(e.target.value)} 
          />
          <button onClick={handleBid} className="bid-btn">ĐẶT GIÁ NGAY</button>
        </div>

        <div className="logs">
          <h3>Diễn biến phiên đấu giá:</h3>
          {logs.map((log, idx) => <p key={idx}>{log}</p>)}
        </div>
      </div>
    );
  }

  // MÀN HÌNH 3: SẢNH CHỜ (DANH SÁCH PHÒNG)
  return (
    <div className="container">
      <h1>Xin chào, {user}</h1>
      
      <div className="create-room">
        <h3>Tạo phòng mới</h3>
        <input placeholder="Tên vật phẩm" onChange={e => setNewRoomTitle(e.target.value)} />
        <input placeholder="Giá khởi điểm" type="number" onChange={e => setNewRoomPrice(e.target.value)} />
        <button onClick={handleCreateRoom}>Tạo phòng</button>
      </div>

      <hr />
      <div className="room-list">
        <h3>Danh sách phòng đang mở <button onClick={handleRefreshRooms}>🔃</button></h3>
        {rooms.length === 0 ? <p>Chưa có phòng nào.</p> : null}
        
        {rooms.map(room => (
          <div key={room.id} className="room-card">
            <b>{room.title}</b> - Giá: {room.price} VND
            <button onClick={() => handleJoinRoom(room)}>Tham gia</button>
          </div>
        ))}
      </div>
    </div>
  );
}

export default App;