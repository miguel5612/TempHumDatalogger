#include "onmotica.h"
#include "configuration.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h> 

const long utcOffsetInSeconds = -18000; //GMT -5:00 For UTC -5.00 : -5 * 60 * 60 : -18000
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "south-america.pool.ntp.org", utcOffsetInSeconds);

void onmotica::init()
{
  timeClient.begin();
}

String onmotica::getTime()
{
   timeClient.update();
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);

   uint8_t hours = ti->tm_hour;
   String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

   uint8_t minutes = ti->tm_min;
   String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

   uint8_t seconds = ti->tm_sec;
   String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

   return dayStr + "/" + monthStr + "/" + yearStr + " - " +
          hoursStr + ":" + minuteStr + ":" + secondStr;

}
