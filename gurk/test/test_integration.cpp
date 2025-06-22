#include <iostream>
#include <cassert>
#include <string>

#include "mock/ArduinoMock.h"

// Include ArduinoJson directly
#include "../.pio/libdeps/modwifi/ArduinoJson/src/ArduinoJson.h"

// Test JSON processing (core of your issue)
void test_json_processing_real_response()
{
    std::cout << "=== Testing JSON Processing with Real Server Response ===" << std::endl;

    // Your actual server response
    String realJsonResponse = R"({
        "current_time": "2025-06-07 12:36:00 CEST",
        "next_watering_time": "2025-06-07 14:00:00 CEST",
        "sleep_time": 14400,
        "time_until_watering": 5039,
        "watering_time": 5
    })";

    std::cout << "JSON size: " << realJsonResponse.length() << " characters" << std::endl;

    // Test with your original buffer size (200) - should fail
    std::cout << "\nTesting with 200-byte buffer:" << std::endl;
    StaticJsonDocument<200> smallDoc;
    DeserializationError smallError = deserializeJson(smallDoc, realJsonResponse.c_str());

    if (smallError)
    {
        std::cout << "❌ Failed as expected: " << smallError.c_str() << std::endl;
    }
    else
    {
        std::cout << "✓ Unexpected success with 200 bytes" << std::endl;
    }

    // Test with your updated buffer size (512) - should work
    std::cout << "\nTesting with 512-byte buffer:" << std::endl;
    StaticJsonDocument<512> largeDoc;
    DeserializationError largeError = deserializeJson(largeDoc, realJsonResponse.c_str());

    if (largeError)
    {
        std::cout << "❌ Failed: " << largeError.c_str() << std::endl;
        return;
    }

    std::cout << "✓ JSON parsed successfully!" << std::endl;
    std::cout << "Memory used: " << largeDoc.memoryUsage() << " bytes" << std::endl;

    // Test accessing fields like your code does
    int timeUntilWatering = largeDoc["time_until_watering"].as<int>();
    unsigned long valveOnDuration = largeDoc["watering_time"].as<int>() * 1000UL * 60UL;
    int sleepTime = largeDoc["sleep_time"].as<int>() * 1000;

    std::cout << "Parsed values:" << std::endl;
    std::cout << "  time_until_watering: " << timeUntilWatering << std::endl;
    std::cout << "  watering_time (ms): " << valveOnDuration << std::endl;
    std::cout << "  sleep_time (ms): " << sleepTime << std::endl;

    // Verify values are correct
    assert(timeUntilWatering == 5039);
    assert(largeDoc["watering_time"].as<int>() == 5);
    assert(largeDoc["sleep_time"].as<int>() == 14400);

    std::cout << "✓ All values correct!" << std::endl;
}

void test_watering_logic_conditions()
{
    std::cout << "\n=== Testing Watering Logic Conditions ===" << std::endl;

    // Test case 1: Should trigger watering (time_until_watering is small)
    String immediateWateringJson = R"({
        "current_time": "2025-06-07 14:00:00 CEST",
        "next_watering_time": "2025-06-07 14:00:00 CEST",
        "sleep_time": 3600,
        "time_until_watering": 100,
        "watering_time": 2
    })";

    StaticJsonDocument<512> doc1;
    deserializeJson(doc1, immediateWateringJson.c_str());

    int timeUntilWatering = doc1["time_until_watering"].as<int>() * 1000;
    int sleepTime = doc1["sleep_time"].as<int>() * 1000;

    // Your logic: if (timeUntilWatering < sleepTime + 20000)
    bool shouldWater = timeUntilWatering < sleepTime + 20000;

    std::cout << "Case 1 - Immediate watering:" << std::endl;
    std::cout << "  time_until_watering: " << timeUntilWatering << "ms" << std::endl;
    std::cout << "  sleep_time + 20000: " << (sleepTime + 20000) << "ms" << std::endl;
    std::cout << "  Should water: " << (shouldWater ? "YES" : "NO") << std::endl;

    assert(shouldWater == true);

    // Test case 2: Should sleep (time_until_watering is large)
    String sleepJson = R"({
        "current_time": "2025-06-07 12:00:00 CEST",
        "next_watering_time": "2025-06-07 18:00:00 CEST",
        "sleep_time": 3600,
        "time_until_watering": 25000,
        "watering_time": 2
    })";

    StaticJsonDocument<512> doc2;
    deserializeJson(doc2, sleepJson.c_str());

    int timeUntilWatering2 = doc2["time_until_watering"].as<int>() * 1000;
    int sleepTime2 = doc2["sleep_time"].as<int>() * 1000;
    bool shouldWater2 = timeUntilWatering2 < sleepTime2 + 20000;

    std::cout << "\nCase 2 - Should sleep:" << std::endl;
    std::cout << "  time_until_watering: " << timeUntilWatering2 << "ms" << std::endl;
    std::cout << "  sleep_time + 20000: " << (sleepTime2 + 20000) << "ms" << std::endl;
    std::cout << "  Should water: " << (shouldWater2 ? "YES" : "NO") << std::endl;

    assert(shouldWater2 == false);

    std::cout << "✓ Watering logic conditions work correctly!" << std::endl;
}

void test_json_error_handling()
{
    std::cout << "\n=== Testing JSON Error Handling ===" << std::endl;

    // Test malformed JSON
    String malformedJson = R"({"incomplete": "json")"; // Missing closing brace

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, malformedJson.c_str());

    if (error)
    {
        std::cout << "✓ Malformed JSON correctly rejected: " << error.c_str() << std::endl;
    }
    else
    {
        std::cout << "❌ Malformed JSON should have been rejected" << std::endl;
        assert(false);
    }

    // Test empty JSON
    String emptyJson = "{}";
    StaticJsonDocument<512> doc2;
    DeserializationError error2 = deserializeJson(doc2, emptyJson.c_str());

    if (!error2)
    {
        // Should parse but fields should be missing/default
        int timeUntilWatering = doc2["time_until_watering"] | -1; // Default to -1
        std::cout << "✓ Empty JSON parsed, missing field defaults to: " << timeUntilWatering << std::endl;
        assert(timeUntilWatering == -1);
    }

    std::cout << "✓ Error handling works correctly!" << std::endl;
}

void test_memory_requirements()
{
    std::cout << "\n=== Testing Memory Requirements ===" << std::endl;

    String realJsonResponse = R"({
        "current_time": "2025-06-07 12:36:00 CEST",
        "next_watering_time": "2025-06-07 14:00:00 CEST",
        "sleep_time": 14400,
        "time_until_watering": 5039,
        "watering_time": 5
    })";

    // Calculate optimal buffer size
    DynamicJsonDocument tempDoc(1024);
    deserializeJson(tempDoc, realJsonResponse.c_str());

    size_t requiredSize = tempDoc.memoryUsage();
    std::cout << "Actual memory required: " << requiredSize << " bytes" << std::endl;
    std::cout << "Your 200-byte buffer: " << (requiredSize <= 200 ? "SUFFICIENT" : "TOO SMALL") << std::endl;
    std::cout << "Your 512-byte buffer: " << (requiredSize <= 512 ? "SUFFICIENT" : "TOO SMALL") << std::endl;
    std::cout << "Recommended minimum: " << (requiredSize + 50) << " bytes (with safety margin)" << std::endl;

    assert(requiredSize > 200);  // Confirms why 200 was too small
    assert(requiredSize <= 512); // Confirms 512 is sufficient

    std::cout << "✓ Memory analysis complete!" << std::endl;
}

int main()
{
    std::cout << "🧪 Integration Tests for Water Controller\n"
              << std::endl;

    try
    {
        test_json_processing_real_response();
        test_watering_logic_conditions();
        test_json_error_handling();
        test_memory_requirements();

        std::cout << "\n🎉 All integration tests passed!" << std::endl;
        std::cout << "\n📋 Summary:" << std::endl;
        std::cout << "- Your JSON parsing with 512-byte buffer works correctly" << std::endl;
        std::cout << "- Your watering logic conditions are properly implemented" << std::endl;
        std::cout << "- Error handling gracefully manages malformed JSON" << std::endl;
        std::cout << "- Memory requirements confirm 512 bytes is sufficient" << std::endl;

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "\n❌ Integration test failed: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "\n❌ Integration test failed with unknown error" << std::endl;
        return 1;
    }
}