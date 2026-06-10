-- system_evaluation_data：完整 9 列（时间均为毫秒，除 rwgl_task_id 为文本）
-- 应用当前仅写入 alarm_id、unique_id、threat_time、alarm_time；其余列留 NULL 供后续流程填充
-- 与 alarm_data 同库（见 bin/db.ini）。已有旧版仅 4 列的表时，请先执行同目录 migrate_system_evaluation_data_add_columns.sql

CREATE TABLE IF NOT EXISTS system_evaluation_data (
    alarm_id VARCHAR(128) PRIMARY KEY,
    unique_id BIGINT NOT NULL,
    threat_time BIGINT NOT NULL,
    alarm_time BIGINT NULL,
    rwgl_recv_time BIGINT NULL,
    rwgl_scheme_time BIGINT NULL,
    rwgl_task_time BIGINT NULL,
    rwgl_task_id VARCHAR(256) NULL,
    wrjgl_recv_time BIGINT NULL
);

COMMENT ON TABLE system_evaluation_data IS '评估链路时刻：当前程序写 threat_time/alarm_time；rwgl/wrjgl 列预留';
COMMENT ON COLUMN system_evaluation_data.threat_time IS '毫秒：首次落 alarm_data 时威胁时刻';
COMMENT ON COLUMN system_evaluation_data.alarm_time IS '毫秒：首次查证成功，仅写一次';
