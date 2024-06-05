#ifndef CONFIG_H
#define CONFIG_H

extern int pinLED;
extern int pinInput;
extern int waitThreshold;
extern unsigned long maxOnDuration;
extern unsigned long reconnectInterval;
extern unsigned long reconnectTimeout;
extern unsigned long standbyDuration;

extern const char *ssid;
extern const char *password;
extern const char *serverUrl;
extern const char *noButtonSignalUrl;
extern const char *logLightStatusUrl;
extern const char *constantsUrl;

#endif
