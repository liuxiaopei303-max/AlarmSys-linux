# 系统告警 gRPC 接口设计（草案）

> 用途：外部系统向 AlarmSys **上报告警 / 取消告警 / 查询当前告警**。  
> 与现有航迹告警快照接口 `AlarmResultService` **独立**，面向「系统级事项告警」。

---

## 1. 概述

| 项 | 说明 |
|---|---|
| 服务名 | `SystemAlarmService` |
| 包名建议 | `alarmsys.grpc.system_alarm` |
| 端口 | **默认 `25071`**（`Config.ini` → `[GrpcSystemAlarm]`，Listen=`192.168.18.141`） |
| 语义 | 增量上报 + 按 `alarm_id` 取消；查询返回当前仍有效（未取消）的告警 |
| 存储 | 当前实现：**进程内内存**；取消后从活跃池移除（暂不落库、不对外发布） |

---

## 2. 系统 ID 分配（`system_id`）

| system_id | 系统名称 | 日常端口 | 建议短名（用于 alarm_id） |
|-----------|----------|----------|---------------------------|
| `sys-001` | 融控（NexusUI） | 21911 | `fusion` |
| `sys-002` | 作战管理 | 21912 | `combat` |
| `sys-003` | 实体管理 | 21913 | `entity` |
| `sys-004` | 任务管理 | 21914 | `task` |
| `sys-005` | 相机管理 | 21915 | `camera` |
| `sys-006` | 无人机管理 | 25316（演示） | `uav` |
| `sys-007` | 告警管理 | 21917 | `alarmsys` |
| `sys-008` | 数据管理 | 21918 | `data` |
| `sys-009` | 雷达管理 | 21919 | `radar` |
| `sys-010` | 航迹管理 | 21920 | `track` |
| `sys-011` | 数据转发 | — | `forward` |


## 3. 枚举

### 3.1 告警类型 `AlarmKind`（六种）

| 值 | 名称 | 说明 |
|----|------|------|
| `EQUIPMENT` = 1 | 装备状态 | 设备/传感器/平台自身状态异常 |
| `DATA_COMM` = 2 | 数据通信 | 链路中断、超时、协议失败等 |
| `TASK` = 3 | 任务 | 任务失败、超时、冲突等 |
| `ENVIRONMENT` = 4 | 环境 | 环境条件相关告警 |
| `COMPREHENSIVE` = 5 | 综合 | 跨类或无法归入以上四类 |
| `SYSTEM_AUTH` = 6 | 系统权鉴 | 鉴权/授权失败、权限变更、非法访问等 |

`UNSPECIFIED = 0` 仅作默认非法值，上报时必须填 1–6。

### 3.2 告警级别 `AlarmLevel`

| 值 | 名称 | 说明 |
|----|------|------|
| `LOW` = 0 | 低 | **默认**（未填或未指定时按低） |
| `MEDIUM` = 1 | 中 | |
| `HIGH` = 2 | 高 | |

---

## 4. 字段约定

### 4.1 `alarm_id`（唯一 ID）

- **必填**（上报、取消均按此关联）
- 建议格式：`alarm_{短名}_{毫秒时间戳}`
- 示例：`alarm_camera_1721020800123`
- 同一 `alarm_id` 重复上报：视为**更新**该条告警（以最后一次为准）

### 4.2 告警条目字段

| 字段 | 必填 | 类型 | 说明 |
|------|------|------|------|
| `alarm_id` | ✓ | string | 唯一 ID |
| `system_id` | ✓ | string | 如 `sys-005` |
| `description` | ✓ | string | 告警描述 |
| `timestamp_ms` | ✓ | int64 | 本次消息时间戳（毫秒） |
| `raised_time_ms` | ✓ | int64 | 告警产生时间（毫秒） |
| `canceled_time_ms` | | int64 | 告警取消时间（毫秒）；未取消为 0 |
| `alarm_kind` | ✓ | AlarmKind | 六种类型之一 |
| `entity_id` | | string | 关联实体，可空 |
| `level` | | AlarmLevel | 可空，默认 `LOW` |
| `reserved1` | | string | 保留字段 1 |
| `reserved2` | | string | 保留字段 2 |
| `reserved3` | | string | 保留字段 3 |

---

## 5. 三个接口

### 5.1 上报告警 `ReportAlarm`

- 请求：一条完整告警条目（见 4.2）
- 响应：`ok` / `message` / `server_time_ms`
- 行为：写入（或更新）当前告警池；同 `alarm_id` 覆盖

### 5.2 取消告警 `CancelAlarm`

| 字段 | 必填 | 说明 |
|------|------|------|
| `alarm_id` | ✓ | 要取消的告警 |
| `system_id` | ✓ | 取消方系统 ID（建议与上报一致） |
| `timestamp_ms` | ✓ | 取消请求消息时间戳（毫秒） |

- 行为：按 `alarm_id` 写入 `canceled_time_ms`，从当前告警池移除（可记历史，实现阶段定）
- 不存在的 `alarm_id`：返回失败或 `ok=false` + 原因（建议 `not_found`）

### 5.3 获取当前告警 `GetActiveAlarms`

| 请求字段 | 必填 | 说明 |
|----------|------|------|
| `system_id` | | 按系统过滤，空=全部 |
| `alarm_kind` | | 按类型过滤；`UNSPECIFIED`=不按类型滤 |
| `level` | | 按级别过滤；可不传表示不过滤（见 proto 注释） |

- 响应：当前仍有效告警列表（条目结构同 4.2）

---

## 6. Proto 草案

```protobuf
syntax = "proto3";

package alarmsys.grpc.system_alarm;

option cc_enable_arenas = true;

enum AlarmKind {
  ALARM_KIND_UNSPECIFIED = 0;
  EQUIPMENT = 1;       // 装备状态
  DATA_COMM = 2;       // 数据通信
  TASK = 3;            // 任务
  ENVIRONMENT = 4;     // 环境
  COMPREHENSIVE = 5;   // 综合
  SYSTEM_AUTH = 6;     // 系统权鉴
}

enum AlarmLevel {
  LOW = 0;             // 默认
  MEDIUM = 1;
  HIGH = 2;
}

message SystemAlarmItem {
  string alarm_id = 1;          // 必填，建议 alarm_{短名}_{ms}
  string system_id = 2;         // 必填，如 sys-005
  string description = 3;       // 必填
  int64 timestamp_ms = 4;       // 必填，消息时间戳 ms
  int64 raised_time_ms = 5;     // 必填，告警产生时间 ms
  int64 canceled_time_ms = 6;   // 告警取消时间 ms；未取消为 0
  AlarmKind alarm_kind = 7;     // 必填，1~6
  string entity_id = 8;         // 可空
  AlarmLevel level = 9;         // 可空，默认 LOW
  string reserved1 = 10;
  string reserved2 = 11;
  string reserved3 = 12;
}

message CommonAck {
  bool ok = 1;
  string message = 2;           // ok / invalid_param / not_found / ...
  int64 server_time_ms = 3;
}

message ReportAlarmRequest {
  SystemAlarmItem alarm = 1;
}

message CancelAlarmRequest {
  string alarm_id = 1;
  string system_id = 2;
  int64 timestamp_ms = 3;       // 取消请求消息时间戳 ms
  int64 canceled_time_ms = 4;   // 必填，告警取消时间 ms
  string reason = 5;            // 可空
}

message GetActiveAlarmsRequest {
  string system_id = 1;         // 空=不过滤
  AlarmKind alarm_kind = 2;     // UNSPECIFIED=不过滤
  // level_filter_set=false 时不过滤级别；true 时按 level 过滤
  bool level_filter_set = 3;
  AlarmLevel level = 4;
}

message GetActiveAlarmsResponse {
  repeated SystemAlarmItem alarms = 1;
  int64 server_time_ms = 2;
}

service SystemAlarmService {
  rpc ReportAlarm(ReportAlarmRequest) returns (CommonAck);
  rpc CancelAlarm(CancelAlarmRequest) returns (CommonAck);
  rpc GetActiveAlarms(GetActiveAlarmsRequest) returns (GetActiveAlarmsResponse);
}
```

---

## 7. 调用示例

**上报（相机管理，装备状态，高级）：**

```text
alarm_id      = alarm_camera_1721020800123
system_id     = sys-005
description   = 相机 camera_004 掉线
timestamp_ms  = 1721020800123
raised_time_ms= 1721020799000
alarm_kind    = EQUIPMENT
entity_id     = camera_004
level         = HIGH
```

**取消：**

```text
alarm_id          = alarm_camera_1721020800123
system_id         = sys-005
timestamp_ms      = 1721020900000
canceled_time_ms  = 1721020898000
reason            = 已恢复
```

**查询：** `system_id=sys-005`，`alarm_kind=EQUIPMENT` → 返回该系统当前装备类未取消告警。

---

## 8. 实现状态 / 待确认

**已实现（AlarmSys-linux）：**
- proto：`third_party/grpc_system_alarm/proto/system_alarm.proto`
- 内存收集：`ReportAlarm` / `CancelAlarm` / `GetActiveAlarms`
- 监听：`[GrpcSystemAlarm] Listen/Port`，默认 `0.0.0.0:25071`

**后续：**
1. 取消后是否落库保留历史（当前仅从内存活跃池移除）
2. 系统告警对外发布（DDS/前端）
3. `sys-011`（数据转发）日常/演示端口号

确认后可继续做发布链路。
