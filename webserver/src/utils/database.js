const sqlite3 = require('sqlite3').verbose();
const path = require('path');
const fs = require('fs');
const logger = require('./logger');

class Database {
    constructor() {
        this.dbPath = path.join(__dirname, '../../data/aircon_control.db');
        this.db = null;
    }

    async initialize() {
        try {
            // 데이터 디렉토리 생성
            const dataDir = path.dirname(this.dbPath);
            if (!fs.existsSync(dataDir)) {
                fs.mkdirSync(dataDir, { recursive: true });
                logger.info('데이터 디렉토리 생성됨:', dataDir);
            }

            // 데이터베이스 연결
            this.db = new sqlite3.Database(this.dbPath, (err) => {
                if (err) {
                    logger.error('데이터베이스 연결 실패:', err);
                    throw err;
                }
                logger.info('SQLite 데이터베이스 연결됨:', this.dbPath);
            });

            // 테이블 생성
            await this.createTables();
            
            // 기본 데이터 삽입
            await this.insertDefaultData();
            
        } catch (error) {
            logger.error('데이터베이스 초기화 실패:', error);
            throw error;
        }
    }

    async createTables() {
        const tables = [
            // 사용자 테이블
            `CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username VARCHAR(50) UNIQUE NOT NULL,
                password_hash VARCHAR(255) NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )`,
            
            // 디바이스 테이블
            `CREATE TABLE IF NOT EXISTS devices (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name VARCHAR(100) NOT NULL,
                ip_address VARCHAR(15) NOT NULL,
                port INTEGER DEFAULT 80,
                api_key VARCHAR(255) NOT NULL,
                status VARCHAR(20) DEFAULT 'offline',
                last_seen DATETIME,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )`,
            
            // 설정 테이블
            `CREATE TABLE IF NOT EXISTS settings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key VARCHAR(100) UNIQUE NOT NULL,
                value TEXT NOT NULL,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )`,
            
            // 제어 히스토리 테이블
            `CREATE TABLE IF NOT EXISTS control_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                device_id INTEGER,
                command VARCHAR(50) NOT NULL,
                parameters TEXT,
                user_id INTEGER,
                executed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (device_id) REFERENCES devices(id),
                FOREIGN KEY (user_id) REFERENCES users(id)
            )`
        ];

        for (const table of tables) {
            await this.run(table);
        }
        
        logger.info('데이터베이스 테이블 생성 완료');
    }

    async insertDefaultData() {
        try {
            // 기본 설정 삽입
            const defaultSettings = [
                ['server_name', 'Aircon Control Server'],
                ['server_version', '1.0.0'],
                ['max_devices', '10'],
                ['log_level', 'info']
            ];

            for (const [key, value] of defaultSettings) {
                await this.run(
                    'INSERT OR IGNORE INTO settings (key, value) VALUES (?, ?)',
                    [key, value]
                );
            }

            // 기본 ESP32 디바이스 추가 (설정에서 수정 가능)
            await this.run(
                `INSERT OR IGNORE INTO devices (name, ip_address, port, api_key, status) 
                 VALUES (?, ?, ?, ?, ?)`,
                ['ESP32 Aircon Controller', '192.168.1.100', 80, 'aircon_control_2024', 'offline']
            );

            logger.info('기본 데이터 삽입 완료');
        } catch (error) {
            logger.error('기본 데이터 삽입 실패:', error);
        }
    }

    // 쿼리 실행 (INSERT, UPDATE, DELETE)
    run(sql, params = []) {
        return new Promise((resolve, reject) => {
            this.db.run(sql, params, function(err) {
                if (err) {
                    logger.error('SQL 실행 실패:', err);
                    reject(err);
                } else {
                    resolve({ id: this.lastID, changes: this.changes });
                }
            });
        });
    }

    // 단일 행 조회
    get(sql, params = []) {
        return new Promise((resolve, reject) => {
            this.db.get(sql, params, (err, row) => {
                if (err) {
                    logger.error('SQL 조회 실패:', err);
                    reject(err);
                } else {
                    resolve(row);
                }
            });
        });
    }

    // 여러 행 조회
    all(sql, params = []) {
        return new Promise((resolve, reject) => {
            this.db.all(sql, params, (err, rows) => {
                if (err) {
                    logger.error('SQL 조회 실패:', err);
                    reject(err);
                } else {
                    resolve(rows);
                }
            });
        });
    }

    // 트랜잭션 실행
    async transaction(callback) {
        return new Promise((resolve, reject) => {
            this.db.serialize(() => {
                this.db.run('BEGIN TRANSACTION');
                
                try {
                    callback(this);
                    this.db.run('COMMIT', (err) => {
                        if (err) {
                            this.db.run('ROLLBACK');
                            reject(err);
                        } else {
                            resolve();
                        }
                    });
                } catch (error) {
                    this.db.run('ROLLBACK');
                    reject(error);
                }
            });
        });
    }

    // 데이터베이스 닫기
    close() {
        return new Promise((resolve, reject) => {
            if (this.db) {
                this.db.close((err) => {
                    if (err) {
                        logger.error('데이터베이스 닫기 실패:', err);
                        reject(err);
                    } else {
                        logger.info('데이터베이스 연결 종료');
                        resolve();
                    }
                });
            } else {
                resolve();
            }
        });
    }

    // 백업 생성
    async backup(backupPath) {
        return new Promise((resolve, reject) => {
            const backupDb = new sqlite3.Database(backupPath);
            this.db.backup(backupDb, (err) => {
                backupDb.close();
                if (err) {
                    logger.error('백업 생성 실패:', err);
                    reject(err);
                } else {
                    logger.info('백업 생성 완료:', backupPath);
                    resolve();
                }
            });
        });
    }
}

module.exports = new Database(); 