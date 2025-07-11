const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const path = require('path');
require('dotenv').config();

const logger = require('./utils/logger');
const database = require('./utils/database');

// 라우터 임포트
const authRoutes = require('./routes/auth');
const deviceRoutes = require('./routes/device');
const settingsRoutes = require('./routes/settings');

const app = express();
const PORT = process.env.PORT || 3000;

// 보안 미들웨어
app.use(helmet());

// CORS 설정
app.use(cors({
    origin: process.env.ALLOWED_ORIGINS ? process.env.ALLOWED_ORIGINS.split(',') : '*',
    credentials: true
}));

// Rate limiting
const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15분
    max: 100, // IP당 최대 100개 요청
    message: {
        error: '너무 많은 요청이 발생했습니다. 잠시 후 다시 시도해주세요.'
    }
});
app.use('/api/', limiter);

// Body parser
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true }));

// 정적 파일 서빙
app.use(express.static(path.join(__dirname, '../public')));

// 로깅 미들웨어
app.use((req, res, next) => {
    logger.info(`${req.method} ${req.url} - ${req.ip}`);
    next();
});

// API 라우트
app.use('/api/auth', authRoutes);
app.use('/api/device', deviceRoutes);
app.use('/api/settings', settingsRoutes);

// 기본 라우트
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, '../public/index.html'));
});

// 설정 페이지
app.get('/settings', (req, res) => {
    res.sendFile(path.join(__dirname, '../public/settings.html'));
});

// 404 핸들러
app.use((req, res) => {
    res.status(404).json({
        error: '요청한 리소스를 찾을 수 없습니다.',
        path: req.url
    });
});

// 에러 핸들러
app.use((err, req, res, next) => {
    logger.error('서버 에러:', err);
    
    res.status(err.status || 500).json({
        error: process.env.NODE_ENV === 'production' 
            ? '서버 내부 오류가 발생했습니다.' 
            : err.message,
        ...(process.env.NODE_ENV !== 'production' && { stack: err.stack })
    });
});

// 서버 시작
async function startServer() {
    try {
        // 데이터베이스 초기화
        await database.initialize();
        logger.info('데이터베이스 초기화 완료');
        
        // 서버 시작
        app.listen(PORT, () => {
            logger.info(`서버가 포트 ${PORT}에서 실행 중입니다.`);
            logger.info(`환경: ${process.env.NODE_ENV || 'development'}`);
            
            if (process.env.NODE_ENV !== 'production') {
                logger.info(`설정 페이지: http://localhost:${PORT}/settings`);
                logger.info(`API 문서: http://localhost:${PORT}/api/status`);
            }
        });
    } catch (error) {
        logger.error('서버 시작 실패:', error);
        process.exit(1);
    }
}

// Graceful shutdown
process.on('SIGTERM', () => {
    logger.info('SIGTERM 신호 수신, 서버 종료 중...');
    process.exit(0);
});

process.on('SIGINT', () => {
    logger.info('SIGINT 신호 수신, 서버 종료 중...');
    process.exit(0);
});

// 예상치 못한 에러 처리
process.on('uncaughtException', (err) => {
    logger.error('예상치 못한 에러:', err);
    process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
    logger.error('처리되지 않은 Promise 거부:', reason);
    process.exit(1);
});

startServer(); 