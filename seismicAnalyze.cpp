// seismicAnalyze.cpp

#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// Data types and globals

struct Values {
  double x;
  double y;
  double z;
};

struct Reading {
  double time;
  Values values;
};

struct Record {
  uint32_t time;
  int16_t  x;
  int16_t  y;
  int16_t  z;
};

static const int version = 3;
static const double G = 9.80665;

/////////////////////////////////////////////////////////////////////////////
// Functions

time_t getFileTime(char *path) {
  char *pt, fileName[200];
  struct tm ts;
  time_t t;

  char *f = strrchr(path, '/');
  if (NULL == f) 
    strcpy(fileName, path);
  else
    strcpy(fileName, f+1);
  char *d = strchr(fileName, '.');
  if (NULL != d)
    *d = '\0';
  pt = strtok(fileName, "-");
  ts.tm_year = atoi(pt) - 1900;
  pt = strtok(NULL, "-\0");
  ts.tm_mon = atoi(pt) - 1;
  pt = strtok(NULL, "-\0");
  ts.tm_mday = atoi(pt);
  ts.tm_hour = ts.tm_min = ts.tm_sec = 0;

  t = mktime(&ts);
  return t;
}

void getRecord(std::ifstream *input, Record *record) {
  uint8_t buffer[11];

  input->read((char*) buffer, 10);
  record->time = buffer[0] + ((uint32_t)buffer[1] << 8)
                          + ((uint32_t)buffer[2] << 16)
                          + ((uint32_t)buffer[3] << 24);
  record->x = buffer[4] + ((int16_t)buffer[5] << 8);
  record->y = buffer[6] + ((int16_t)buffer[7] << 8);
  record->z = buffer[8] + ((int16_t)buffer[9] << 8);
}

// Read the next record from input, compute the time and scale the readings.
void getValues(std::ifstream *input, Reading *reading, time_t fileTime) {
  Record record;
  double seconds;

  getRecord(input, &record);
  seconds = (double)record.time / 1000.0;
  reading->time = (double)(uint32_t)fileTime + seconds;
  reading->values.x = (double)record.x * 0.24375;
  reading->values.y = (double)record.y * 0.24375;
  reading->values.z = (double)record.z * 0.24375;
}

/////////////////////////////////////////////////////////////////////////////
// Main program

int main(int argc, char **argv) {
  std::ifstream input;
  Reading       reading;
  int           readingCount = 0;
  struct tm    *utcTime;
  double        prevReadingTime, t, fractionalSeconds, sx, sy, sz;
  time_t        ut;
  bool          first = true;

  if (argc < 2) {
    std::cout << "seismicAnalyse version " << version << std::endl
              << "Missing file argument" << std::endl
              << "Usage:" << std::endl
              << "  seismicAnalyse <data-file>" << std::endl;
    return 1;
  }

  input.open(argv[1], std::ios::binary);
  if (input.fail()) {
    std::cout << "Failed to open " << argv[1] << std::endl;
    return 1;
  }
  time_t fileTime = getFileTime(argv[1]);
  sx = sy = sz = 0.0;

  while (!input.eof()) {
    getValues(&input, &reading, fileTime);
    if (!input.eof()) {
      if (!first) {
        // Calculate displacements sx, sy, sz. Acceleration in reading is in
        // milli-g. Calculate acceleration in meters/sec2.
        double dt = reading.time - prevReadingTime;
        double ax = G * reading.values.x / 1000.0;
        double ay = G * reading.values.y / 1000.0;
        double az = G * reading.values.z / 1000.0;
        // Calculate the displacements.
        double dx = (ax * pow(dt, 2)) / 2.0;
        double dy = (ay * pow(dt, 2)) / 2.0;
        double dz = (az * pow(dt, 2)) / 2.0;
        sx += dx;
        sy += dy;
        sz += dz;
        // Correct for drift.
        double c = 2.8e-6;
        sx < 0.0 ? sx += c : sx -= c;
        sy < 0.0 ? sy += c : sy -= c;
        sz < 0.0 ? sz += c : sz -= c;
        std::cout << readingCount << ": sx = " << sx << " sy = "
                  << sy << " sz = " << sz
                  << std::endl;
      }
    }
    prevReadingTime = reading.time;
    ++readingCount;
    first = false;
    /*
    ut = (time_t)(uint32_t)reading.time;
    utcTime = localtime(&ut);
    fractionalSeconds = modf(reading.time, &t);
    std::cout << utcTime->tm_year + 1900 << "-"
              << utcTime->tm_mon + 1 << "-"
              << utcTime->tm_mday << " "
              << utcTime->tm_hour << ":"
              << utcTime->tm_min << ":"
              << utcTime->tm_sec << "." << fractionalSeconds << " "
              << reading.values.x << " "
              << reading.values.y << " "
              << reading.values.z << std::endl;
    */
  }

  input.close();
  return 0;
}

