/*
    Copyright 2016-2020 Arisotura, WaluigiWare64
    This file is part of melonDS.
    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.
    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <iostream>
#include <string>
#include <vector>
#include <cstring> 
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdio>

namespace Updater
{
    using json = nlohmann::json;
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
    {
        size_t written = fwrite(ptr, size, nmemb, stream);
        return written;
    }
    std::vector<std::string> getCURL(std::string webURL, const char* authFlag="", bool fwrite=false, const char* fname="") 
    {
        CURL *curl;
        CURLcode res;
        FILE *fp;
      
        std::string readBuffer = "";

        curl = curl_easy_init();
        if(curl) 
        {
            curl_easy_setopt(curl, CURLOPT_URL, webURL.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, (std::string("curl/") + curl_version_info_data(*curl_version_info(CURLVERSION_NOW)).version).c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, authFlag);
            if (fwrite)
            {
                fp = fopen(fname,"wb");
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            } 
            else
            {
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            }
        
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (fwrite) {
                fclose(fp);
            }
            return std::vector<std::string> {readBuffer, curl_easy_strerror(res)};
        
         }
    }
    std::vector<std::string> checkForUpdates(std::string currentVer, const char* authFlag)
    {
        #if defined(_WIN32)
            const char* platform = "melonDS-win-x86_64";
        #elif defined(__linux__) && !defined(__aarch64__)
            const char* platform = "melonDS-ubuntu-x86_64";
        #elif defined(__linux__) && defined(__aarch64__)
            const char* platform = "melonDS-ubuntu-aarch64";	
        #endif
        std::string repo = "Arisotura/melonDS";
        std::string latestArtifact, latestArtifactVer, commitMessage;
        
        std::vector<std::string> test = getCURL("https://api.github.com/user", authFlag);
        std::cout << test[0] << test[1];
        if (test[1] != curl_easy_strerror(CURLcode(0))) {
            return std::vector<std::string> {"Err", 
            "There was an error while trying to test with cURL:" + test[1] + " This may happen because you are not connected to the internet, or your access token is not correct."};
        }
        try {
            json(json::parse(std::string(test[0])));
        }
        catch(json::parse_error) {
            return std::vector<std::string> {"Err", "There was an error while parsing the JSON response."};
        }
        printf("Network connectivity + JSON Parse Test succeeded. Trying to check for updates");
        
        json jsonWorkflows = json::parse(std::string(getCURL("https://api.github.com/repos/" + repo + "/actions/runs", authFlag)[0]));
        for (const auto& obj : jsonWorkflows["workflow_runs"]) 
        {
            if (obj["head_repository"]["name"] == repo && obj["conclusion"] == "success") 
            {
                json jsonArtifact = json::parse(getCURL(obj["artifacts_url"])[0]);
                if (jsonArtifact["artifacts"][0]["name"] == platform) {
                    latestArtifact = jsonArtifact["artifacts"][0]["archive_download_url"];
                    latestArtifactVer = jsonArtifact["artifacts"][0]["url"];
                    latestArtifactVer = latestArtifactVer.substr(latestArtifactVer.find_last_of("/")+1);
                    commitMessage = obj["head_commit"]["message"];
                    break;
                }
            }
        }
        if (latestArtifactVer == currentVer)
        {
            return std::vector<std::string> {"N", ""};
        }
        else
        {
            return std::vector<std::string> {"Y", commitMessage};
        }
    }
    int installUpdate()
    {
        //getCURL(latestArtifact.c_str(), authFlag, true, "melonDS.zip");
    }
}
