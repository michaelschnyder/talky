#include "Contacts.h"

bool Contacts::load() {

    logger.verbose(F("Attempting to load contacts from '%s'"), filename.c_str());

    if (!SD.exists(filename)) {
        
        logger.warning(F("Contacts file '%s' does not exist."), filename.c_str());
        return false;
    }

    File jsonFile = SD.open(filename, "r");

    if (!jsonFile) {
        logger.error(F("Cannot open contacts file '%s'"), filename.c_str());
        return false;
    }

    // Allocate a buffer to store contents of the file.
    StaticJsonBuffer<512> jsonBuffer;
    JsonArray &root = jsonBuffer.parseArray(jsonFile);
    jsonFile.close();

    if (!root.success()) {
        logger.error(F("failed to load contacts."));
        return false;
    }

    int elements = root.size();

    logger.verbose(F("Found %i contacts..."), elements);
    
    delete[] contacts;
    contacts = new ContactElement[elements];

    int i = 0;

    for (auto& element : root) {

        const char* name = element["name"];
        const char* userId = element["userId"];

        contacts[i].name = String(name);
        contacts[i].userId = String(userId);
        i++;
    }

    logger.trace(F("Contacts successfully loaded."));
    return true;
}
