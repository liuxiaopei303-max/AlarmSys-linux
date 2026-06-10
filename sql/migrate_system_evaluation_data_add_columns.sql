-- 从仅含 4 列的旧表升级到完整 8 列（在目标库执行一次即可）

ALTER TABLE system_evaluation_data
  ADD COLUMN IF NOT EXISTS rwgl_recv_time BIGINT,
  ADD COLUMN IF NOT EXISTS rwgl_scheme_time BIGINT,
  ADD COLUMN IF NOT EXISTS rwgl_task_time BIGINT,
  ADD COLUMN IF NOT EXISTS rwgl_task_id VARCHAR(256),
  ADD COLUMN IF NOT EXISTS wrjgl_recv_time BIGINT;
