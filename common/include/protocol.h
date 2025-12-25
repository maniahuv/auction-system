#ifndef PROTOCOL_H
#define PROTOCOL_H

// Buoc gia toi thieu
#define MIN_BID_STEP 10000 

// Client -> Server (C2S)
typedef enum {
	// 1.Xac thuc
    C2S_REGISTER = 101,	// (Auctioneer, bidder)
    C2S_LOGIN = 102,	// (Admin, bidder, auctioneer)
    
    // 2.Phong dau gia 
    C2S_LIST_ROOMS = 201,	// (Bidder)
    C2S_CREATE_ROOM = 202,	// (Auctioneer)
    C2S_JOIN_ROOM = 203,	// (Bidder)
    C2S_LEAVE_ROOM = 204,	// (Bidder)
    
    // 3.Vat pham do nguoi dau gia tao 
    C2S_CREATE_ITEM = 301,	// (Auctioneer)
    // Liet ke vat pham dau gia 
    C2S_LIST_MY_ITEMS = 302,// (Auctioneer)
    
    // 4.Nguoi mua dau gia 
    C2S_BID = 401,		// (Bidder)
    C2S_BUY_NOW = 402,	// (Bidder)
} ClientMessageType;

typedef enum {
    // 8.Phan hoi thanh cong/ khong thanh cong
    S2C_GENERIC_OK = 800,
    S2C_GENERIC_ERROR = 801,
    
    // 8.Phan hoi xac thuc
    S2C_LOGIN_SUCCESS = 802,
    
    // 8.Phan hoi danh sach phong
    S2C_ROOM_LIST = 810,
    
    // 9.Cap nhat thong tin trong phong dau gia, Thong bao broadcast cho moi nguoi
    S2C_JOIN_ROOM_SUCCESS = 901,	// Thong bao co nguoi join phong
    S2C_NEW_ITEM_PENDING = 902, 	// Thong bao phien moi dang dien ra
    S2C_AUCTION_STARTED = 903, 		// Thong bao bat dau phien dau gia
    S2C_NEW_BID = 904,         		// Thong bao gia moi duoc cap nhat
    S2C_TIME_ALERT = 905,      		// Canh bao con 30s
    S2C_AUCTION_ENDED = 906,   		// Thong bao ket thuc phien dau gia
    
} ServerMessageType;

typedef enum {
    ERR_NONE = 0,
    ERR_UNKNOWN = 1,
    ERR_INVALID_MESSAGE = 2,
    ERR_WRONG_PASSWORD = 3,
    ERR_USER_EXISTS = 4,
    ERR_USER_NOT_FOUND = 5,
    ERR_ROOM_NOT_FOUND = 6,
    ERR_ALREADY_IN_ROOM = 7,
    ERR_BID_TOO_LOW = 8,
    // ... 
} ErrorCode;

#endif

