#ifndef MOCKHTTPHANDLER_H
#define MOCKHTTPHANDLER_H

#include <string>
#include <map>
#include <iostream>

// Mock String class for testing
class String
{
private:
    std::string data;

public:
    String() = default;
    String(const char *str) : data(str ? str : "") {}
    String(const std::string &str) : data(str) {}
    String &operator=(const char *str)
    {
        data = str ? str : "";
        return *this;
    }
    String &operator=(const std::string &str)
    {
        data = str;
        return *this;
    }
    bool operator==(const char *str) const { return data == (str ? str : ""); }
    bool operator==(const String &other) const { return data == other.data; }
    bool operator!=(const char *str) const { return data != (str ? str : ""); }
    const char *c_str() const { return data.c_str(); }
    friend std::ostream &operator<<(std::ostream &os, const String &str)
    {
        return os << str.data;
    }
};

// Mock HTTP status codes
#define HTTP_CODE_OK 200
#define HTTP_CODE_MOVED_PERMANENTLY 301

enum class HTTPStatus
{
    SUCCESS,
    CONNECTION_FAILED,
    REQUEST_FAILED,
    INVALID_RESPONSE
};

// Abstract interfaces for HTTP operations (for mocking)
class IHTTPClient
{
public:
    virtual ~IHTTPClient() = default;
    virtual bool begin(const String &url) = 0;
    virtual int GET() = 0;
    virtual String getString() = 0;
    virtual void end() = 0;
};

class IWiFiClient
{
public:
    virtual ~IWiFiClient() = default;
    virtual void setInsecure() = 0;
};

// Mock implementations
class MockHTTPClient : public IHTTPClient
{
private:
    std::map<std::string, std::string> mockResponses;
    std::map<std::string, int> mockStatusCodes;
    bool shouldFailConnection;
    std::string lastUrl;

public:
    MockHTTPClient() : shouldFailConnection(false) {}

    bool begin(const String &url) override
    {
        lastUrl = url.c_str();
        std::cout << "MockHTTPClient: begin(" << lastUrl << ") -> " << (!shouldFailConnection ? "success" : "failure") << std::endl;
        return !shouldFailConnection;
    }

    int GET() override
    {
        if (shouldFailConnection)
        {
            std::cout << "MockHTTPClient: GET() -> -1 (connection failed)" << std::endl;
            return -1;
        }

        auto it = mockStatusCodes.find(lastUrl);
        int code = (it != mockStatusCodes.end()) ? it->second : HTTP_CODE_OK;
        std::cout << "MockHTTPClient: GET() -> " << code << std::endl;
        return code;
    }

    String getString() override
    {
        auto it = mockResponses.find(lastUrl);
        std::string response = (it != mockResponses.end()) ? it->second : "{\"default\": \"response\"}";
        std::cout << "MockHTTPClient: getString() -> " << response << std::endl;
        return String(response.c_str());
    }

    void end() override
    {
        std::cout << "MockHTTPClient: end()" << std::endl;
    }

    // Test helpers
    void setMockResponse(const std::string &url, const std::string &response)
    {
        mockResponses[url] = response;
    }

    void setMockStatusCode(const std::string &url, int code)
    {
        mockStatusCodes[url] = code;
    }

    void setConnectionFailure(bool shouldFail)
    {
        shouldFailConnection = shouldFail;
    }

    void clearMocks()
    {
        mockResponses.clear();
        mockStatusCodes.clear();
        shouldFailConnection = false;
    }
};

class MockWiFiClient : public IWiFiClient
{
public:
    void setInsecure() override
    {
        std::cout << "MockWiFiClient: setInsecure()" << std::endl;
    }
};

#endif