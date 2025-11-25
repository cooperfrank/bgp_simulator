#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Announcement.h"

class Policy {
public:
    virtual ~Policy() = default;

    // Insert a new announcement into the received queue
    virtual void receiveAnnouncement(const Announcement& ann) = 0;

    // Process the received announcements and update the local RIB
    virtual void processAnnouncements() = 0;

    // Process the received announcements and update the local RIB, using
    // `my_asn` as the ASN of the AS doing the processing. Implementations
    // should prepend `my_asn` to any stored AS-paths when storing.
    virtual void processAnnouncementsFor(uint32_t my_asn) = 0;

    // Access the local RIB
    virtual const std::unordered_map<std::string, Announcement>& getLocalRIB() const = 0;
};
