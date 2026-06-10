#include "alarm_geoproj.h"

#include "customconfig.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void AlarmLatLongToMetres(const SPxLatLong_t* ll, double* mx, double* my)
{
    if (!ll || !mx || !my) {
        return;
    }
    CustomConfig* c = CustomConfig::getInstance();
    const double lat0 = c->m_struBasicConfig.m_dBasePointLat;
    const double lon0 = c->m_struBasicConfig.m_dBasePointLon;
    const double metersPerDegLat = 111320.0;
    const double metersPerDegLon = 111320.0 * std::cos(lat0 * M_PI / 180.0);
    *mx = (ll->longDegs - lon0) * metersPerDegLon;
    *my = (ll->latDegs - lat0) * metersPerDegLat;
}

void AlarmGetViewPixelsPerMetre(double* ppm)
{
    if (ppm) {
        *ppm = 1.0;
    }
}
