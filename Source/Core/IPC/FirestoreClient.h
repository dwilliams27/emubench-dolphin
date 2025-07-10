#include <curl/curl.h>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Common/Logging/Log.h"

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
  size_t totalSize = size * nmemb;
  userp->append((char*)contents, totalSize);
  return totalSize;
}

class FirestoreClient {
private:
  std::string projectId;
  std::string accessToken;
  
  std::string getAccessToken() {
    // Use metadata server to get access token
    CURL* curl = curl_easy_init();
    std::string response;
    
    if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL, 
        "http://metadata.google.internal/computeMetadata/v1/instance/service-accounts/default/token");
      
      struct curl_slist* headers = nullptr;
      headers = curl_slist_append(headers, "Metadata-Flavor: Google");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      
      // Set callback to capture response
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      
      curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);
    }
    
    return extractAccessToken(response);
  }

  std::string extractAccessToken(const std::string& response) {
    try {
      // Parse the JSON response
      nlohmann::json jsonResponse = nlohmann::json::parse(response);
      
      // Extract the access_token field
      if (jsonResponse.contains("access_token")) {
        return jsonResponse["access_token"].get<std::string>();
      } else {
        std::cerr << "Error: access_token not found in response" << std::endl;
        return "";
      }
    } catch (const nlohmann::json::exception& e) {
      std::cerr << "JSON parsing error: " << e.what() << std::endl;
      return "";
    }
  }
  
public:
  FirestoreClient(const std::string& project_id) : projectId(project_id) {
    accessToken = getAccessToken();
    NOTICE_LOG_FMT(CORE, "IPC: Firestore client initialized");
  }
  
  std::string readDocument(const std::string& collection, const std::string& testId,
                           const std::string& subCollection, const std::string& docId) {
    NOTICE_LOG_FMT(CORE, "IPC: Reading document from Firestore");
    CURL* curl = curl_easy_init();
    std::string response;
    
    if(curl) {
      std::string url = "https://firestore.googleapis.com/v1/projects/" + 
                        projectId + "/databases/(default)/documents/" + 
                        collection + "/" + testId + "/" + subCollection + "/" + docId;
      NOTICE_LOG_FMT(CORE, "IPC: Firestore URL: {}", url);
      
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      
      struct curl_slist* headers = nullptr;
      std::string authHeader = "Authorization: Bearer " + accessToken;
      headers = curl_slist_append(headers, authHeader.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
      
      curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);
    }
    
    return response;
  }
  
  bool writeDocument(const std::string& collection, const std::string& testId,
                     const std::string& subCollection, const std::string& docId, 
                     const std::string& jsonData) {
    NOTICE_LOG_FMT(CORE, "IPC: Writing document to Firestore");
    CURL* curl = curl_easy_init();
    
    if(curl) {
      std::string url = "https://firestore.googleapis.com/v1/projects/" + 
                        projectId + "/databases/(default)/documents/" + 
                        collection + "/" + testId + "/" + subCollection + "/" + docId;
      NOTICE_LOG_FMT(CORE, "IPC: Firestore URL: {}", url);
      
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
      
      struct curl_slist* headers = nullptr;
      std::string authHeader = "Authorization: Bearer " + accessToken;
      headers = curl_slist_append(headers, authHeader.c_str());
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
      
      curl_easy_perform(curl);
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);
    } else {
      NOTICE_LOG_FMT(CORE, "IPC: Unable to init curl");
    }

    return true;
  }

  nlohmann::json convertToFirestoreValue(const nlohmann::json& value) {
    nlohmann::json firestoreValue;
    
    if (value.is_string()) {
      firestoreValue["stringValue"] = value.get<std::string>();
    } else if (value.is_number_integer()) {
      firestoreValue["integerValue"] = std::to_string(value.get<int64_t>());
    } else if (value.is_number_float()) {
      firestoreValue["doubleValue"] = value.get<double>();
    } else if (value.is_boolean()) {
      firestoreValue["booleanValue"] = value.get<bool>();
    } else if (value.is_object()) {
      firestoreValue["mapValue"]["fields"] = convertToFirestoreFields(value);
    } else if (value.is_array()) {
      nlohmann::json arrayValues;
      for (const auto& item : value) {
        arrayValues["values"].push_back(convertToFirestoreValue(item));
      }
      firestoreValue["arrayValue"] = arrayValues;
    } else if (value.is_null()) {
      firestoreValue["nullValue"] = nullptr;
    }
    
    return firestoreValue;
  }

  nlohmann::json convertToFirestoreFields(const nlohmann::json& obj) {
    nlohmann::json fields;
    
    for (auto& [key, value] : obj.items()) {
      fields[key] = convertToFirestoreValue(value);
    }
    
    return fields;
  }

  std::string createFirestorePayload(const nlohmann::json& data) {
    nlohmann::json firestoreDoc;
    firestoreDoc["fields"] = convertToFirestoreFields(data);
    return firestoreDoc.dump();
  }
};
