#pragma once
#include "Policy.h"
#include "Announcement.h"
#include <unordered_map>
#include <vector>
#include <string>

class BGP : public Policy {
private:
    std::unordered_map<std::string, Announcement> local_rib;
    std::unordered_map<std::string, std::vector<Announcement>> received_queue;

public:
    BGP() = default;

    void receiveAnnouncement(const Announcement& ann) override { received_queue[ann.prefix].push_back(ann); };
    void processAnnouncements() override;
    void processAnnouncementsFor(uint32_t my_asn) override;
    const std::unordered_map<std::string, Announcement>& getLocalRIB() const override { return local_rib; };
};
