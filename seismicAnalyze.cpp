// seismicAnalyze.cpp

#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
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

static const int version = 5;
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
  int           argIndex = 1, readingCount = 0;
  struct tm    *utcTime;
  double        prevReadingTime, t, fractionalSeconds, sx, sy, sz;
  double        vx, vy, vz;
  time_t        ut;
  bool          first = true, useCommas = false;

  if ((argc < 2) || (strcmp(argv[1], "-h") == 0)) {
    std::cout << "seismicAnalyse version " << version << std::endl
              << "Usage:" << std::endl
              << "  seismicAnalyse [-c] <data-file>" << std::endl
              << "where:" << std::endl
              << "  -c = use commas instead of spaces to separate columns"
              << std::endl;
    return 1;
  }

  for (int i = 1; i < argc; i++)
    if (strcmp(argv[i], "-c") == 0) {
      useCommas = true;
      argIndex = i + 1;
    }

  input.open(argv[argIndex], std::ios::binary);
  if (input.fail()) {
    std::cout << "Failed to open " << argv[argIndex] << std::endl;
    return 1;
  }
  time_t fileTime = getFileTime(argv[argIndex]);
  sx = sy = sz = vx = vy = vz = 0.0;

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
        // Calculate the velocity.
        vx += ax * dt;
        vy += ay * dt;
        vz += az * dt;
        // Calculate the displacements.
        sx += vx * dt;
        sy += vy * dt;
        sz += vz * dt;
        // Correct for drift.
        /*
        double c = 2.8e-6;
        sx < 0.0 ? sx += c : sx -= c;
        sy < 0.0 ? sy += c : sy -= c;
        sz < 0.0 ? sz += c : sz -= c;
        */
        if (useCommas) {
          std::cout << readingCount << std::setprecision(6)
                    << "," << sx << "," << sy << "," << sz << std::endl;
        } else {
          std::cout << readingCount << std::setprecision(6)
                    << " " << sx << " " << sy << " " << sz << std::endl;
        }
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

