// Erzeugt von tools/wetter_icons.py. Nicht von Hand aendern.
#include "wetter.h"
#include <string.h>

#include "assets/icons/wetter/wx_clear_day.inc"
#include "assets/icons/wetter/wx_clear_night.inc"
#include "assets/icons/wetter/wx_extreme_day_rain.inc"
#include "assets/icons/wetter/wx_extreme_day_snow.inc"
#include "assets/icons/wetter/wx_extreme_night_rain.inc"
#include "assets/icons/wetter/wx_extreme_night_snow.inc"
#include "assets/icons/wetter/wx_fog_day.inc"
#include "assets/icons/wetter/wx_fog_night.inc"
#include "assets/icons/wetter/wx_mostly_clear_day.inc"
#include "assets/icons/wetter/wx_mostly_clear_night.inc"
#include "assets/icons/wetter/wx_overcast_day.inc"
#include "assets/icons/wetter/wx_overcast_day_drizzle.inc"
#include "assets/icons/wetter/wx_overcast_day_rain.inc"
#include "assets/icons/wetter/wx_overcast_day_sleet.inc"
#include "assets/icons/wetter/wx_overcast_day_snow.inc"
#include "assets/icons/wetter/wx_overcast_night.inc"
#include "assets/icons/wetter/wx_overcast_night_drizzle.inc"
#include "assets/icons/wetter/wx_overcast_night_rain.inc"
#include "assets/icons/wetter/wx_overcast_night_sleet.inc"
#include "assets/icons/wetter/wx_overcast_night_snow.inc"
#include "assets/icons/wetter/wx_partly_cloudy_day.inc"
#include "assets/icons/wetter/wx_partly_cloudy_day_drizzle.inc"
#include "assets/icons/wetter/wx_partly_cloudy_day_rain.inc"
#include "assets/icons/wetter/wx_partly_cloudy_day_snow.inc"
#include "assets/icons/wetter/wx_partly_cloudy_night.inc"
#include "assets/icons/wetter/wx_partly_cloudy_night_drizzle.inc"
#include "assets/icons/wetter/wx_partly_cloudy_night_rain.inc"
#include "assets/icons/wetter/wx_partly_cloudy_night_snow.inc"
#include "assets/icons/wetter/wx_thunderstorms_day_hail.inc"
#include "assets/icons/wetter/wx_thunderstorms_day_rain.inc"
#include "assets/icons/wetter/wx_thunderstorms_night_hail.inc"
#include "assets/icons/wetter/wx_thunderstorms_night_rain.inc"
#include "assets/icons/wetter/wxk_clear_day.inc"
#include "assets/icons/wetter/wxk_clear_night.inc"
#include "assets/icons/wetter/wxk_extreme_day_rain.inc"
#include "assets/icons/wetter/wxk_extreme_day_snow.inc"
#include "assets/icons/wetter/wxk_extreme_night_rain.inc"
#include "assets/icons/wetter/wxk_extreme_night_snow.inc"
#include "assets/icons/wetter/wxk_fog_day.inc"
#include "assets/icons/wetter/wxk_fog_night.inc"
#include "assets/icons/wetter/wxk_mostly_clear_day.inc"
#include "assets/icons/wetter/wxk_mostly_clear_night.inc"
#include "assets/icons/wetter/wxk_overcast_day.inc"
#include "assets/icons/wetter/wxk_overcast_day_drizzle.inc"
#include "assets/icons/wetter/wxk_overcast_day_rain.inc"
#include "assets/icons/wetter/wxk_overcast_day_sleet.inc"
#include "assets/icons/wetter/wxk_overcast_day_snow.inc"
#include "assets/icons/wetter/wxk_overcast_night.inc"
#include "assets/icons/wetter/wxk_overcast_night_drizzle.inc"
#include "assets/icons/wetter/wxk_overcast_night_rain.inc"
#include "assets/icons/wetter/wxk_overcast_night_sleet.inc"
#include "assets/icons/wetter/wxk_overcast_night_snow.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_day.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_day_drizzle.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_day_rain.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_day_snow.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_night.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_night_drizzle.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_night_rain.inc"
#include "assets/icons/wetter/wxk_partly_cloudy_night_snow.inc"
#include "assets/icons/wetter/wxk_thunderstorms_day_hail.inc"
#include "assets/icons/wetter/wxk_thunderstorms_day_rain.inc"
#include "assets/icons/wetter/wxk_thunderstorms_night_hail.inc"
#include "assets/icons/wetter/wxk_thunderstorms_night_rain.inc"

struct WxEintrag { const char* name; const lv_img_dsc_t* gross; const lv_img_dsc_t* klein; };
static const WxEintrag s_wx[] = {
  { "clear_day", &wx_clear_day, &wxk_clear_day },
  { "clear_night", &wx_clear_night, &wxk_clear_night },
  { "extreme_day_rain", &wx_extreme_day_rain, &wxk_extreme_day_rain },
  { "extreme_day_snow", &wx_extreme_day_snow, &wxk_extreme_day_snow },
  { "extreme_night_rain", &wx_extreme_night_rain, &wxk_extreme_night_rain },
  { "extreme_night_snow", &wx_extreme_night_snow, &wxk_extreme_night_snow },
  { "fog_day", &wx_fog_day, &wxk_fog_day },
  { "fog_night", &wx_fog_night, &wxk_fog_night },
  { "mostly_clear_day", &wx_mostly_clear_day, &wxk_mostly_clear_day },
  { "mostly_clear_night", &wx_mostly_clear_night, &wxk_mostly_clear_night },
  { "overcast_day", &wx_overcast_day, &wxk_overcast_day },
  { "overcast_day_drizzle", &wx_overcast_day_drizzle, &wxk_overcast_day_drizzle },
  { "overcast_day_rain", &wx_overcast_day_rain, &wxk_overcast_day_rain },
  { "overcast_day_sleet", &wx_overcast_day_sleet, &wxk_overcast_day_sleet },
  { "overcast_day_snow", &wx_overcast_day_snow, &wxk_overcast_day_snow },
  { "overcast_night", &wx_overcast_night, &wxk_overcast_night },
  { "overcast_night_drizzle", &wx_overcast_night_drizzle, &wxk_overcast_night_drizzle },
  { "overcast_night_rain", &wx_overcast_night_rain, &wxk_overcast_night_rain },
  { "overcast_night_sleet", &wx_overcast_night_sleet, &wxk_overcast_night_sleet },
  { "overcast_night_snow", &wx_overcast_night_snow, &wxk_overcast_night_snow },
  { "partly_cloudy_day", &wx_partly_cloudy_day, &wxk_partly_cloudy_day },
  { "partly_cloudy_day_drizzle", &wx_partly_cloudy_day_drizzle, &wxk_partly_cloudy_day_drizzle },
  { "partly_cloudy_day_rain", &wx_partly_cloudy_day_rain, &wxk_partly_cloudy_day_rain },
  { "partly_cloudy_day_snow", &wx_partly_cloudy_day_snow, &wxk_partly_cloudy_day_snow },
  { "partly_cloudy_night", &wx_partly_cloudy_night, &wxk_partly_cloudy_night },
  { "partly_cloudy_night_drizzle", &wx_partly_cloudy_night_drizzle, &wxk_partly_cloudy_night_drizzle },
  { "partly_cloudy_night_rain", &wx_partly_cloudy_night_rain, &wxk_partly_cloudy_night_rain },
  { "partly_cloudy_night_snow", &wx_partly_cloudy_night_snow, &wxk_partly_cloudy_night_snow },
  { "thunderstorms_day_hail", &wx_thunderstorms_day_hail, &wxk_thunderstorms_day_hail },
  { "thunderstorms_day_rain", &wx_thunderstorms_day_rain, &wxk_thunderstorms_day_rain },
  { "thunderstorms_night_hail", &wx_thunderstorms_night_hail, &wxk_thunderstorms_night_hail },
  { "thunderstorms_night_rain", &wx_thunderstorms_night_rain, &wxk_thunderstorms_night_rain },
};
const lv_img_dsc_t* wxSuche(const char* name, bool klein) {
  for (const WxEintrag& e : s_wx) if (strcmp(e.name, name) == 0) return klein ? e.klein : e.gross;
  return nullptr;
}
