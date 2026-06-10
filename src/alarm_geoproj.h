#pragma once

/* 与 SPxViewControl.h 中定义一致，避免在无 UI / 无 SPx 库时链接显示层 */
typedef struct SPxLatLong_tag {
    double latDegs;
    double longDegs;
} SPxLatLong_t;

/** 以 BasicConfig 中基准点为原点的局部平面近似（米），替代 SPxViewControl::LatLongToMetres */
void AlarmLatLongToMetres(const SPxLatLong_t* ll, double* mx, double* my);

/** 无显示设备时返回 1.0，保持与原逻辑中变量存在性兼容 */
void AlarmGetViewPixelsPerMetre(double* ppm);
