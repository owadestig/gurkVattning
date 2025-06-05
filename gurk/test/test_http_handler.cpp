#include <iostream>
#include <cassert>
#include <string>

// Mock Arduino functions
void delay(unsigned long /*ms*/) {}
unsigned long millis()
{
    static unsigned long mockTime = 0;
    mockTime += 100;
    return mockTime;
}

class Serial_Mock
{
public:
    template <typename T>
    void print(T value) { std::cout << value; }

    template <typename T>
    void println(T value) { std::cout << value << std::endl; }

    void println() { std::cout << std::endl; }
} Serial;

// Include mock interface
#include "mock/MockHTTPHandler.h"

// Simplified HTTPHandler for testing (without Arduino dependencies)
class HTTPHandler
{
private:
    static HTTPHandler *instance;
    IHTTPClient *httpClient;
    IWiFiClient *wifiClient;
    bool ownsClients;
    HTTPStatus lastStatus;

    HTTPHandler() : httpClient(nullptr), wifiClient(nullptr), ownsClients(false), lastStatus(HTTPStatus::SUCCESS) {}

public:
    static HTTPHandler &getInstance()
    {
        if (instance == nullptr)
        {
            instance = new HTTPHandler();
        }
        return *instance;
    }

    static void setClients(IHTTPClient *http, IWiFiClient *wifi)
    {
        HTTPHandler &handler = getInstance();
        if (handler.ownsClients)
        {
            delete handler.httpClient;
            delete handler.wifiClient;
        }
        handler.httpClient = http;
        handler.wifiClient = wifi;
        handler.ownsClients = false;
    }

    String sendRequest(const String &url)
    {
        if (!httpClient || !wifiClient)
        {
            lastStatus = HTTPStatus::CONNECTION_FAILED;
            return "error";
        }

        wifiClient->setInsecure();

        if (!httpClient->begin(url))
        {
            lastStatus = HTTPStatus::CONNECTION_FAILED;
            return "error";
        }

        int httpCode = httpClient->GET();
        String payload = "error";

        if (httpCode > 0)
        {
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                payload = httpClient->getString();
                lastStatus = HTTPStatus::SUCCESS;
            }
            else
            {
                lastStatus = HTTPStatus::INVALID_RESPONSE;
            }
        }
        else
        {
            lastStatus = HTTPStatus::REQUEST_FAILED;
        }

        httpClient->end();
        return payload;
    }

    HTTPStatus getLastStatus() const
    {
        return lastStatus;
    }

    String getDeviceVariables(const String &serverUrl)
    {
        return sendRequest(serverUrl);
    }

    bool sendNoButtonSignal(const String &url)
    {
        String response = sendRequest(url);
        return response != "error" && lastStatus == HTTPStatus::SUCCESS;
    }

    bool setIsWatering(const String &url, bool /*isWatering*/)
    {
        String response = sendRequest(url);
        return response != "error" && lastStatus == HTTPStatus::SUCCESS;
    }

    // Reset for testing
    static void reset()
    {
        if (instance)
        {
            if (instance->ownsClients)
            {
                delete instance->httpClient;
                delete instance->wifiClient;
            }
            delete instance;
            instance = nullptr;
        }
    }
};

HTTPHandler *HTTPHandler::instance = nullptr;

// Test functions
void test_http_request_success()
{
    std::cout << "=== Testing successful HTTP request ===" << std::endl;

    HTTPHandler::reset();

    MockHTTPClient *mockHTTP = new MockHTTPClient();
    MockWiFiClient *mockWiFi = new MockWiFiClient();

    mockHTTP->setMockResponse("https://test.com/api", "{\"status\": \"ok\", \"data\": \"test\"}");
    mockHTTP->setMockStatusCode("https://test.com/api", HTTP_CODE_OK);

    HTTPHandler::setClients(mockHTTP, mockWiFi);
    HTTPHandler &handler = HTTPHandler::getInstance();

    String response = handler.sendRequest("https://test.com/api");

    assert(response == "{\"status\": \"ok\", \"data\": \"test\"}");
    assert(handler.getLastStatus() == HTTPStatus::SUCCESS);

    std::cout << "✓ HTTP success test passed" << std::endl;

    delete mockHTTP;
    delete mockWiFi;
}

void test_http_request_failure()
{
    std::cout << "=== Testing HTTP request failure ===" << std::endl;

    HTTPHandler::reset();

    MockHTTPClient *mockHTTP = new MockHTTPClient();
    MockWiFiClient *mockWiFi = new MockWiFiClient();

    mockHTTP->setConnectionFailure(true);

    HTTPHandler::setClients(mockHTTP, mockWiFi);
    HTTPHandler &handler = HTTPHandler::getInstance();

    String response = handler.sendRequest("https://test.com/api");

    assert(response == "error");
    assert(handler.getLastStatus() == HTTPStatus::CONNECTION_FAILED);

    std::cout << "✓ HTTP failure test passed" << std::endl;

    delete mockHTTP;
    delete mockWiFi;
}

void test_http_invalid_status_code()
{
    std::cout << "=== Testing HTTP invalid status code ===" << std::endl;

    HTTPHandler::reset();

    MockHTTPClient *mockHTTP = new MockHTTPClient();
    MockWiFiClient *mockWiFi = new MockWiFiClient();

    mockHTTP->setMockStatusCode("https://test.com/api", 404);

    HTTPHandler::setClients(mockHTTP, mockWiFi);
    HTTPHandler &handler = HTTPHandler::getInstance();

    String response = handler.sendRequest("https://test.com/api");

    assert(response == "error");
    assert(handler.getLastStatus() == HTTPStatus::INVALID_RESPONSE);

    std::cout << "✓ HTTP invalid status test passed" << std::endl;

    delete mockHTTP;
    delete mockWiFi;
}

void test_device_variables_endpoint()
{
    std::cout << "=== Testing device variables endpoint ===" << std::endl;

    HTTPHandler::reset();

    MockHTTPClient *mockHTTP = new MockHTTPClient();
    MockWiFiClient *mockWiFi = new MockWiFiClient();

    mockHTTP->setMockResponse("https://gurkvattning.onrender.com/get_device_variables",
                              "{\"watering_duration\": 5000, \"check_interval\": 1000}");

    HTTPHandler::setClients(mockHTTP, mockWiFi);
    HTTPHandler &handler = HTTPHandler::getInstance();

    String response = handler.getDeviceVariables("https://gurkvattning.onrender.com/get_device_variables");

    assert(response == "{\"watering_duration\": 5000, \"check_interval\": 1000}");
    assert(handler.getLastStatus() == HTTPStatus::SUCCESS);

    std::cout << "✓ Device variables test passed" << std::endl;

    delete mockHTTP;
    delete mockWiFi;
}

void test_no_button_signal()
{
    std::cout << "=== Testing no button signal endpoint ===" << std::endl;

    HTTPHandler::reset();

    MockHTTPClient *mockHTTP = new MockHTTPClient();
    MockWiFiClient *mockWiFi = new MockWiFiClient();

    mockHTTP->setMockResponse("https://gurkvattning.onrender.com/no_button_signal",
                              "{\"acknowledged\": true}");

    HTTPHandler::setClients(mockHTTP, mockWiFi);
    HTTPHandler &handler = HTTPHandler::getInstance();

    bool success = handler.sendNoButtonSignal("https://gurkvattning.onrender.com/no_button_signal");

    assert(success == true);
    assert(handler.getLastStatus() == HTTPStatus::SUCCESS);

    std::cout << "✓ No button signal test passed" << std::endl;

    delete mockHTTP;
    delete mockWiFi;
}

int main()
{
    std::cout << "Running HTTPHandler Tests..." << std::endl
              << std::endl;

    try
    {
        test_http_request_success();
        test_http_request_failure();
        test_http_invalid_status_code();
        test_device_variables_endpoint();
        test_no_button_signal();

        std::cout << std::endl
                  << "🎉 All HTTP tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}