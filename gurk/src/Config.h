#ifndef CONFIG_H
#define CONFIG_H

extern const int pinLED;
extern const int pinInput;
extern const int waitThreshold;
extern const unsigned long maxOnDuration;
extern const unsigned long reconnectInterval;
extern const unsigned long reconnectTimeout;
extern const unsigned long standbyDuration;

extern const char *ssid;
extern const char *password;
extern const char *serverUrl;
extern const char *noButtonSignalUrl;
extern const char *logLightStatusUrl;

#endif
