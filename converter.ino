#include "settings.h"

const float r = rotation_angle * M_PI / 180;

// meters in Meridian
const float m_in_mer = 20004274 / 180;

// length Degree at equator 111321
// Meters in parallel
const float m_in_par = 111321 * cos(baselat * M_PI / 180);

struct coord_nmea {
  // Gradusy i minuty
  int full;
  // 1 / 1 000 000 minuty
  long ostatok;
};

void setup() {
  // initialize serial:
  Serial.begin(serial_speed);
}

String stringFloat8 (long number) {
  String result = "";
  if (number < 10000000) result += "0";
  if (number < 1000000) result += "0";
  if (number < 100000) result += "0";
  if (number < 10000) result += "0";
  if (number < 1000) result += "0";
  if (number < 100) result += "0";
  if (number < 10) result += "0";
  result += String(number);
  return result;
}

String stringFloat6 (long number) {
  String result = "";
  if (number < 100000) result += "0";
  if (number < 10000) result += "0";
  if (number < 1000) result += "0";
  if (number < 100) result += "0";
  if (number < 10) result += "0";
  result += String(number);
  return result;
}

coord_nmea coordinate_to_nmea (int full_part, long ost_part) {
  coord_nmea new_coordinate;

  new_coordinate.full = full_part * 100;

  int add_to_full_part = round(ost_part * 6 / 10000000);

  ost_part -= round(add_to_full_part * 10000000 / 6);

  new_coordinate.ostatok = round(ost_part * 6 / 10);

  if (new_coordinate.ostatok > (1000000 - 1) ) {
    new_coordinate.ostatok = 0;
    if (add_to_full_part < (60 - 1)) {
      new_coordinate.full++;
    } else {
      add_to_full_part = 0;
      new_coordinate.full += 100;
    }
  }

  new_coordinate.full += add_to_full_part;

  return new_coordinate;
}

String getTime() {
  String time = "";

  uint32_t curTime = millis();

  int hours = trunc(curTime / 3600000);
  curTime -= hours * 3600000;
  hours = hours % 24;

  int minutes = trunc(curTime / 60000);
  curTime -= minutes * 60000;
  minutes = minutes % 60;

  int seconds = trunc(curTime / 1000);
  curTime -= seconds * 1000;
  seconds = seconds % 60;

  int huseconds = trunc(curTime / 10);
  huseconds = huseconds % 100;

  // Add Hours
  if (hours < 10) {
    time += "0" + String(hours);
  } else {
    time += String(hours);
  }
  // Add mins
  if (minutes < 10) {
    time += "0" + String(minutes);
  } else {
    time += String(minutes);
  }
  // Add seconds
  if (seconds < 10) {
    time += "0" + String(seconds);
  } else {
    time += String(seconds);
  }
  time += ".";

  // Add hund seconds
  if (huseconds < 10) {
    time += "0" + String(huseconds);
  } else {
    time += String(huseconds);
  }

  return time;
}

String getChecksum(String message) {

  byte sum = 0;

  int stringLength = message.length() + 1;

  char mes[stringLength];

  message.toCharArray(mes, stringLength);

  for (int i = 0; i < stringLength - 1; i++) {
    sum ^= mes[i];
  }


  String prevResult = String(sum, HEX);

  prevResult.toUpperCase();

  if (prevResult.length() == 1) {
    prevResult = "0" + prevResult;
  }

  return prevResult;
}

void printNmea(coord_nmea lat, coord_nmea lon, float height) {
  String nmea_message = "GPGGA,";
  nmea_message += getTime();
  nmea_message += ",";
  if (abs(lat.full) < 1000) nmea_message += "0";
  nmea_message += String(lat.full);
  nmea_message += ".";
  nmea_message += stringFloat6(lat.ostatok);
  nmea_message += ",";

  // Add Sign
  if (lat.full < 0) {
    nmea_message += "S";
  } else {
    nmea_message += "N";
  }
  nmea_message += ",";
  if (abs(lon.full) < 10000) nmea_message += "0";
  if (abs(lon.full) < 1000) nmea_message += "0";
  nmea_message += String(lon.full);
  nmea_message += ".";
  nmea_message += stringFloat6(lon.ostatok);
  nmea_message += ",";

  // Add Sign
  if (lon.full < 0) {
    nmea_message += "W";
  } else {
    nmea_message += "E";
  }
  // Add Sat info
  // 4 - RTK Quality
  // 20 - 20 satellites
  // 1 - HDOP
  nmea_message += ",4,20,1,";
  nmea_message += String(height);

  // Height in Meters
  nmea_message += ",M,";
  // Add height of Geoid
  nmea_message += heightGeoid;
  // Height in Meters
  // Blank - Time since last DGPS update
  // Blank - DGPS reference station id
  nmea_message += ",M,,";

  Serial.print("$");
  Serial.print(nmea_message);
  Serial.print("*");
  Serial.print(getChecksum(nmea_message));
  Serial.println();
}

void loop() {
  // if there's any serial available, read it:
  while (Serial.available() > 0) {

    // READ PART

    float x = Serial.parseFloat();
    float y = Serial.parseFloat();

    float height = Serial.parseFloat();

    // look for the newline. That's the end of your sentence:
    if (Serial.read() == '\n') {
      // Serial.println("INIT STATE:");

      // Serial.print("meters in 1 degree Meridian: ");
      // Serial.println(m_in_mer, 2);

      // Serial.print("meters in 1 degree Parallel: ");
      // Serial.println(m_in_par, 2);

      // START CONVERTION

      // x1 = x0cos(θ) – y0sin(θ)
      // y1 = x0sin(θ) + y0cos(θ)

      // LATITUDE

      // FLOAT PART
      // *100 - convert Meters to CM

      long addlat = long((x * sin(r) + y * cos(r)) * 100000000 / m_in_mer);

      // Serial.print("Add to Latitude in 10^-7 degree: ");
      // Serial.println(addlat);

      long newlat_fl = baselat_fl + addlat;

      int addbase = 0;
      // Check Big numbers
      if (newlat_fl > 100000000) {
        newlat_fl -= 100000000;
        addbase = 1;
      }

      if (newlat_fl < -100000000) {
        newlat_fl += 100000000;
        addbase = -1;
      }

      // INT PART
      int newlat_int = baselat_int + addbase;

      // LONGTITUDE ===================

      // FLOAT PART
      long addlon = long((x * cos(r) - y * sin(r)) * 100000000 / m_in_par);

      long newlon_fl = baselon_fl + addlon;

      // Serial.print("Add to Longitude in 10^-7 degree: ");
      // Serial.println(addlon);

      addbase = 0;
      // Check Big numbers
      if (newlon_fl > 100000000) {
        newlon_fl -= 100000000;
        addbase = 1;
      }

      if (newlon_fl < -100000000) {
        newlon_fl += 100000000;
        addbase = -1;
      }

      // INT PART
      int newlon_int = baselon_int + addbase;

      // HEIGHT
      float newheight = base_height + height;

      // OUTPUT PART

      // Corrected output
      /*
        Serial.print("Corrected Latitude: ");
        Serial.print(newlat_int);
        Serial.print(".");
        Serial.print(stringFloat8(newlat_fl));
        Serial.print(", ");
        Serial.print("Corrected Longitude: ");
        Serial.print(newlon_int);
        Serial.print(".");
        Serial.print(stringFloat8(newlon_fl));
        Serial.print(" at height ");
        Serial.print(newheight);
        Serial.println();
      */

      // NMEA Format
      coord_nmea coord1 = coordinate_to_nmea(newlat_int, newlat_fl);
      coord_nmea coord2 = coordinate_to_nmea(newlon_int, newlon_fl);

      /*
        Serial.print("NMEA Latitude: ");
        Serial.print(coord1.full);
        Serial.print(".");
        Serial.print(stringFloat6(coord1.ostatok));
        Serial.print(", ");
        Serial.print("NMEA Longitude: ");
        Serial.print(coord2.full);
        Serial.print(".");
        Serial.print(stringFloat6(coord2.ostatok));
        Serial.print(" at ");
        Serial.print(newheight);
        Serial.println();
      */

      // Serial.print("Current time: ");
      // Serial.println(getTime());

      printNmea(coord1, coord2, newheight);
    }
  }
}
