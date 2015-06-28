#ifndef __MACMANAGER_H_INCLUDED__
#define __MACMANAGER_H_INCLUDED__

#include <string>
#include <iostream>
#include <fstream>

class MACManager {
    private :
        const std::string m_key;

    protected :
        /**
         * Returns the current time. Refactored to be able to override for testing.
         * @return string of current time in milliseconds
         */
        virtual std::string getTime() const;

    public : 
        /**
         * Initializes MACManager with the supplied key string.
         * @param key const string of supplied vendor key
         */
        MACManager(const std::string& key);

        /**
         * Generates the HMAC and in turn returns the final url
         * with msgpad and HMAC appended at the end as parameters.
         * @param url const string of the initial url
         * @return a string of the final url with msgpad and HMAC
         */
        std::string getEncryptUrl(const std::string& url) const;

        /**
         * Reads the key from file and returns the key as a string.
         * @return a string of the supplied vendor key
         */
        static const std::string getKeyFromFile();

        /**
         * Reads the key from the given file and returns it as a string.
         * @param filePath const string of the full path to file
         * @return a strong of the supplied vendor key
         */
        static const std::string getKeyFromFile(const std::string& filePath);
};

#endif
